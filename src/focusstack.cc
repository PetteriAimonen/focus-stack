#include "focusstack.hh"
#include "worker.hh"
#include "logger.hh"
#include "task_loadimg.hh"
#include "task_grayscale.hh"
#include "task_align.hh"
#include "task_wavelet.hh"
#include "task_wavelet_opencl.hh"
#include "task_merge.hh"
#include "task_denoise.hh"
#include "task_reassign.hh"
#include "task_saveimg.hh"
#include "task_focusmeasure.hh"
#include "task_depthmap.hh"
#include "task_depthmap_inpaint.hh"
#include "task_3dpreview.hh"
#include <thread>
#include <opencv2/core/ocl.hpp>

using namespace focusstack;

FocusStack::FocusStack():
  m_output(""),
  m_depthmap_threshold(16),
  m_depthmap_smooth_xy(32),
  m_depthmap_smooth_z(64),
  m_disable_opencl(false),
  m_save_steps(false),
  m_align_only(false),
  m_align_flags(ALIGN_DEFAULT),
  m_3dviewpoint(1,1,1),
  m_3dzscale(1),
  m_threads(std::thread::hardware_concurrency() + 1), // +1 to have extra thread to give tasks for GPU
  m_batchsize(8),
  m_reference(-1),
  m_consistency(0),
  m_jpgquality(95),
  m_denoise(0)
{
  m_logger = std::make_shared<Logger>();

  reset();
}

FocusStack::~FocusStack()
{
}

void FocusStack::set_verbose(bool verbose)
{
  m_logger->set_level(verbose ? LOG_VERBOSE : LOG_PROGRESS);
}

void FocusStack::set_log_callback(std::function<void(log_level_t level, std::string)> callback)
{
  m_logger->set_callback(callback);
}

bool FocusStack::run()
{
  reset();
  start();
  do_final_merge();

  bool status;
  std::string errmsg;
  wait_done(status, errmsg);
  return status;
}

void FocusStack::start()
{
  m_worker = std::make_unique<Worker>(m_threads, m_logger);

  m_have_opencl = false;
  if (m_disable_opencl)
  {
    m_logger->verbose("OpenCL disabled\n");
    cv::ocl::setUseOpenCL(false);
  }
  else
  {
    if (!cv::ocl::haveOpenCL())
    {
      m_logger->verbose("OpenCL not available\n");
    }
    else
    {
      cv::ocl::setUseOpenCL(true);

      cv::ocl::Context context = cv::ocl::Context::getDefault();
      if (context.ndevices() > 0)
      {
        cv::ocl::Device dev = context.device(0);

        m_logger->verbose("OpenCL device: %s %s %s\n",
                          dev.vendorName().c_str(),
                          dev.name().c_str(),
                          dev.version().c_str());
        m_have_opencl = true;
      }
      else
      {
        m_logger->verbose("OpenCL: no devices available\n");
      }
    }
  }

  // Add any images that have been added as filenames
  for (const std::string &input: m_inputs)
  {
    m_input_images.push_back(std::make_shared<Task_LoadImg>(input));
  }

  schedule_queue_processing();
}

void FocusStack::add_image(std::string filename)
{
  m_input_images.push_back(std::make_shared<Task_LoadImg>(filename));

  if (m_worker)
  {
    schedule_queue_processing();
  }
}

void FocusStack::add_image(const cv::Mat &image)
{
  std::string name = "memimg-" + std::to_string(m_input_images.size()) + ".jpg";
  m_input_images.push_back(std::make_shared<Task_LoadImg>(name, image));

  if (m_worker)
  {
    schedule_queue_processing();
  }
}

void FocusStack::do_final_merge()
{
  schedule_queue_processing();
  schedule_final_merge();

  // All temporaries except results can be released now.
  // Anything that is needed is held on by shared_ptrs in the tasks.
  reset(true);
}

void FocusStack::get_status(int &total_tasks, int &completed_tasks)
{
  if (!m_worker)
  {
    completed_tasks = total_tasks = 0;
  }
  else
  {
    m_worker->get_status(total_tasks, completed_tasks);
  }
}

bool FocusStack::wait_done(bool &status, std::string &errmsg, int timeout_ms)
{
  if (!m_worker)
  {
    status = false;
    errmsg = "Process is not running";
    return true;
  }

  if (m_worker->wait_all(timeout_ms))
  {
    status = !m_worker->failed();
    if (!status)
    {
      errmsg = m_worker->error();
    }
    m_worker.reset();

    return true;
  }

  // Not done yet
  return false;
}

void FocusStack::reset(bool keep_results)
{
  m_scheduled_image_count = 0;
  m_refidx = -1;
  m_input_images.clear();
  m_grayscale_imgs.clear();
  m_aligned_imgs.clear();
  m_aligned_grayscales.clear();
  m_refcolor.reset();
  m_refgray.reset();
  m_prev_merge.reset();
  m_merge_batch.clear();
  m_reassign_batch_grays.clear();
  m_reassign_batch_colors.clear();
  m_reassign_map.reset();

  if (!keep_results)
  {
    m_worker.reset();
    m_result_image.reset();
    m_result_depthmap.reset();
    m_result_3dview.reset();
  }
}

const cv::Mat &FocusStack::get_result_image() const
{
  if (m_result_image)
  {
    return m_result_image->img();
  }
  else
  {
    throw std::runtime_error("No result image available");
  }
}

const cv::Mat &FocusStack::get_result_depthmap() const
{
  if (m_result_depthmap)
  {
    return m_result_depthmap->img();
  }
  else
  {
    throw std::runtime_error("No result depthmap available");
  }
}

const cv::Mat &FocusStack::get_result_3dview() const
{
  if (m_result_3dview)
  {
    return m_result_3dview->img();
  }
  else
  {
    throw std::runtime_error("No result 3dview available");
  }
}

void FocusStack::schedule_queue_processing()
{
  const int count = m_input_images.size();
  if (count <= m_scheduled_image_count) return; // No new images

  m_grayscale_imgs.resize(count);
  m_aligned_imgs.resize(count);
  m_aligned_grayscales.resize(count);
  m_focusmeasures.resize(count);

  if (!m_refcolor)
  {
    // Load the reference image
    // This is needed when processing the other images, so load it first.

    // Use middle image as reference if no option is given
    m_refidx = m_reference;
    if (m_refidx < 0 || m_refidx >= count)
      m_refidx = count / 2;

    m_refcolor = m_input_images.at(m_refidx);
    m_refgray = std::make_shared<Task_Grayscale>(m_refcolor);
    m_worker->add(m_refcolor);
    m_worker->add(m_refgray);

    m_grayscale_imgs.at(m_refidx) = m_refgray;
  }

  // Construct list of indexes. Perform alignment from reference image outwards.
  std::vector<int> indexes;
  if (m_refidx >= m_scheduled_image_count) indexes.push_back(m_refidx);
  for (int i = 1; i < count; i++)
  {
    if (m_refidx - i >= m_scheduled_image_count && m_refidx - i < count) indexes.push_back(m_refidx - i);
    if (m_refidx + i >= m_scheduled_image_count && m_refidx + i < count) indexes.push_back(m_refidx + i);
  }

  for (int i : indexes)
  {
    if (i != m_refidx)
    {
      // Schedule image loading
      m_worker->add(m_input_images.at(i));

      // Convert image to grayscale
      // The reference image is used to calculate the best mapping, which is then used for all images.
      m_grayscale_imgs.at(i) = std::make_shared<Task_Grayscale>(m_input_images.at(i), m_refgray);
      m_worker->add(m_grayscale_imgs.at(i));
    }

    // Track the indexes for depthmap
    m_input_images.at(i)->set_index(i);
    m_grayscale_imgs.at(i)->set_index(i);

    if (m_save_steps)
    {
      m_worker->add(std::make_shared<Task_SaveImg>("grayscale_" + m_grayscale_imgs.at(i)->basename(),
                                                   m_grayscale_imgs.at(i), m_jpgquality, true));
    }

    schedule_alignment(i);

    if (m_align_only)
    {
      m_worker->add(std::make_shared<Task_SaveImg>(m_output + m_input_images.at(i)->basename(),
                                                   m_aligned_imgs.at(i), m_jpgquality, true));
    }
    else
    {
      if (m_save_steps)
      {
        // Task_Align adds "aligned_" prefix to the filename, so just use that name for saving also.
        m_worker->add(std::make_shared<Task_SaveImg>(m_aligned_imgs.at(i)->filename(),
                                                     m_aligned_imgs.at(i), m_jpgquality, true));
      }

      schedule_single_image_processing(i);
      schedule_depthmap_processing(i, false);

      if (m_merge_batch.size() >= m_batchsize)
      {
        schedule_batch_merge();
      }
    }
  }

  m_scheduled_image_count = m_input_images.size();
  release_temporaries();
}

void FocusStack::schedule_alignment(int i)
{
  // Perform image alignment, against either the reference image or the neighbor image.
  // In very thick stacks, it is difficult to align outermost images directly
  // against the reference image because of heavy blurring.
  std::shared_ptr<Task_Align> aligned;
  int neighbour = m_refidx;
  if (i < m_refidx) neighbour = i + 1;
  if (i > m_refidx) neighbour = i - 1;
  if (i != m_refidx)
  {
    if (m_align_flags & ALIGN_GLOBAL)
    {
      // Align directly against the global reference, but use neighbour as a guess.
      // This can give slightly better alignment in shallow stacks with little blur.
      aligned = std::make_shared<Task_Align>(m_aligned_grayscales.at(m_refidx),
                                              m_aligned_imgs.at(m_refidx),
                                              m_grayscale_imgs.at(i),
                                              m_input_images.at(i),
                                              m_aligned_imgs.at(neighbour),
                                              nullptr,
                                              m_align_flags);
    }
    else
    {
      // Align against the neighbour image.
      // This usually works better on deep stacks that have blur at the extremes, and
      // works equally well as global alignment for almost all cases.
      // This also allows us to align against the original source image and stacking
      // the transforms later, which gives better parallelism while benefiting from
      // the similarity in alignment between neighbour images.
      aligned = std::make_shared<Task_Align>(m_grayscale_imgs.at(neighbour),
                                              m_input_images.at(neighbour),
                                              m_grayscale_imgs.at(i),
                                              m_input_images.at(i),
                                              nullptr,
                                              m_aligned_imgs.at(neighbour),
                                              m_align_flags);
    }
  }
  else
  {
    // Nothing to be done for the global reference image, but we run it through Task_Align
    // to make the types match.
    aligned = std::make_shared<Task_Align>(m_refgray, m_refcolor, m_refgray, m_refcolor);
  }

  m_aligned_imgs.at(i) = aligned;
  m_worker->add(aligned);
}

void FocusStack::schedule_single_image_processing(int i)
{
  // Convert aligned image to grayscale again.
  // We could also transform the grayscale images directly, but a new grayscale conversion is faster
  // and results in less difference between the color and grayscale versions.
  m_aligned_grayscales.at(i) = std::make_shared<Task_Grayscale>(m_aligned_imgs.at(i), m_refgray);
  m_worker->add(m_aligned_grayscales.at(i));
  m_reassign_batch_grays.push_back(m_aligned_grayscales.at(i));
  m_reassign_batch_colors.push_back(m_aligned_imgs.at(i));

  // Wavelet transform the image
  std::shared_ptr<ImgTask> wavelet;
  if (m_have_opencl)
  {
    wavelet = std::make_shared<Task_Wavelet_OpenCL>(m_aligned_grayscales.at(i), false);
  }
  else
  {
    wavelet = std::make_shared<Task_Wavelet>(m_aligned_grayscales.at(i), false);
  }
  m_worker->add(wavelet);
  m_merge_batch.push_back(wavelet);
}

void FocusStack::schedule_batch_merge()
{
  // Merge wavelet images accumulated so far
  m_prev_merge = std::make_shared<Task_Merge>(m_prev_merge, m_merge_batch, m_consistency);
  m_worker->add(m_prev_merge);
  m_merge_batch.clear();

  // And update reassignment map.
  // After this, the aligned images can be unloaded from RAM.
  m_reassign_map = std::make_shared<Task_Reassign_Map>(m_reassign_batch_grays,
                                                       m_reassign_batch_colors,
                                                       m_reassign_map);
  m_worker->add(m_reassign_map);
  m_reassign_batch_colors.clear();
  m_reassign_batch_grays.clear();
}

void FocusStack::schedule_depthmap_processing(int i, bool is_final)
{
  if (m_depthmap != "" || m_filename_3dview != "")
  {
    if (i >= 0)
    {
      m_focusmeasures.at(i) = std::make_shared<Task_FocusMeasure>(m_aligned_grayscales.at(i));
      m_worker->add(m_focusmeasures.at(i));

      if (m_save_steps)
      {
        m_worker->add(std::make_shared<Task_SaveImg>(m_focusmeasures.at(i)->filename(),
                              m_focusmeasures.at(i), m_jpgquality, true));
      }
    }

    // Check when neighbours are available so that we can process the depthmap layer
    if (m_focusmeasures.size() > 1)
    {
      for (int j = 0; j < m_focusmeasures.size(); j++)
      {
        if (m_focusmeasures.at(j) && !m_depthmap_processed.count(j))
        {
          if (j == m_focusmeasures.size() - 1)
          {
            // The last image can be scheduled once we know no more images are coming.
            if (is_final && m_focusmeasures.at(j - 1))
            {
              std::vector<std::shared_ptr<ImgTask> > neighbors = {m_focusmeasures.at(j - 1)};
              m_latest_depthmap = std::make_shared<Task_Depthmap>(m_focusmeasures.at(j), neighbors, j, m_latest_depthmap);
              m_worker->add(m_latest_depthmap);
              m_depthmap_processed.insert(j);
            }
          }
          else if (j == 0 && m_focusmeasures.at(j + 1))
          {
            // First image, one neighbour
            std::vector<std::shared_ptr<ImgTask> > neighbors = {m_focusmeasures.at(j + 1)};
            m_latest_depthmap = std::make_shared<Task_Depthmap>(m_focusmeasures.at(j), neighbors, j, m_latest_depthmap);
            m_worker->add(m_latest_depthmap);
            m_depthmap_processed.insert(j);
          }
          else if (m_focusmeasures.at(j - 1) && m_focusmeasures.at(j + 1))
          {
            // Two neighbours
            std::vector<std::shared_ptr<ImgTask> > neighbors = {m_focusmeasures.at(j - 1), m_focusmeasures.at(j + 1)};
            m_latest_depthmap = std::make_shared<Task_Depthmap>(m_focusmeasures.at(j), neighbors, j, m_latest_depthmap);
            m_worker->add(m_latest_depthmap);
            m_depthmap_processed.insert(j);
          }
        }
      }
    }
  }
}

void FocusStack::release_temporaries()
{
  for (int i = 0; i < m_scheduled_image_count; i++)
  {
    if (i == m_refidx)
    {
      // Need to keep the global reference
    }
    else if (i == m_scheduled_image_count - 1)
    {
      // Need to keep the latest neighbor for alignment
    }
    else
    {
      // Otherwise we can release our pointers.
      // The image will be released by shared_ptr as soon as the tasks are done.
      m_input_images.at(i).reset();
      m_grayscale_imgs.at(i).reset();
      m_aligned_imgs.at(i).reset();
      m_aligned_grayscales.at(i).reset();
    }

    // Focus measures can be released once their neighbours have been processed.
    if (m_focusmeasures.size() > i && m_focusmeasures.at(i))
    {
      if (i != m_scheduled_image_count - 1 &&
          m_depthmap_processed.count(i) &&
          m_depthmap_processed.count(i + 1) &&
          (i == 0 || m_depthmap_processed.count(i - 1)))
      {
        m_focusmeasures.at(i).reset();
      }
    }
  }
}

void FocusStack::schedule_final_merge()
{
  if (m_align_only) return;

  // Merge the final batch of images
  if (m_merge_batch.size() > 0 || m_reassign_batch_colors.size() > 0)
  {
    schedule_batch_merge();
  }

  // Save depth map if requested
  if (m_depthmap != "" || m_filename_3dview != "")
  {
    schedule_depthmap_processing(-1, true);

    std::shared_ptr<ImgTask> inpainted = std::make_shared<Task_Depthmap_Inpaint>(
      m_latest_depthmap, m_depthmap_threshold, m_depthmap_smooth_xy, m_depthmap_smooth_z, m_halo_radius, m_save_steps);
    m_worker->add(inpainted);

    m_result_depthmap = std::make_shared<Task_SaveImg>(m_depthmap, inpainted, m_jpgquality, m_nocrop);
    m_worker->add(m_result_depthmap);
  }

  // Denoise merged image
  std::shared_ptr<ImgTask> denoised = m_prev_merge;
  if (m_denoise > 0)
  {
    denoised = std::make_shared<Task_Denoise>(m_prev_merge, m_denoise);
    m_worker->add(denoised);
  }

  // Inverse-transform merged image
  std::shared_ptr<ImgTask> merged_gray;
  if (!m_have_opencl)
  {
    merged_gray = std::make_shared<Task_Wavelet>(denoised, true);
  }
  else
  {
    merged_gray = std::make_shared<Task_Wavelet_OpenCL>(denoised, true);
  }
  m_worker->add(merged_gray);

  if (m_save_steps)
  {
    m_worker->add(std::make_shared<Task_SaveImg>(merged_gray->filename(), merged_gray, m_jpgquality, m_nocrop));
  }

  // Reassign pixel values
  std::shared_ptr<Task_Reassign> colored = std::make_shared<Task_Reassign>(m_reassign_map, merged_gray);
  m_worker->add(colored);

  // Save result image
  m_result_image = std::make_shared<Task_SaveImg>(m_output, colored, m_jpgquality, m_nocrop);
  m_worker->add(m_result_image);

  // Save 3D preview
  if (m_filename_3dview != "")
  {
    std::shared_ptr<Task_3DPreview> preview = std::make_shared<Task_3DPreview>(
      m_result_depthmap, nullptr, m_result_image,
      m_3dviewpoint, m_3dzscale);
    m_worker->add(preview);
    m_result_3dview = std::make_shared<Task_SaveImg>(m_filename_3dview, preview, m_jpgquality, m_nocrop);
    m_worker->add(m_result_3dview);
  }
}
