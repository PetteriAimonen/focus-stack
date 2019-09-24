#include "focusstack.hh"
#include "worker.hh"
#include "task_loadimg.hh"
#include "task_grayscale.hh"
#include "task_align.hh"
#include "task_wavelet.hh"
#include "task_wavelet_opencl.hh"
#include "task_merge.hh"
#include "task_denoise.hh"
#include "task_reassign.hh"
#include "task_saveimg.hh"
#include <thread>
#include <opencv2/core/ocl.hpp>

using namespace focusstack;

FocusStack::FocusStack():
  m_output("output.jpg"),
  m_disable_opencl(false),
  m_save_steps(false),
  m_verbose(false),
  m_align_flags(ALIGN_DEFAULT),
  m_threads(std::thread::hardware_concurrency() + 1), // +1 to have extra thread to give tasks for GPU
  m_reference(-1),
  m_consistency(0),
  m_denoise(0)
{
}

bool FocusStack::run()
{
  Worker worker(m_threads, m_verbose);

  bool have_opencl = false;
  if (m_disable_opencl)
  {
    printf("OpenCL disabled\n");
    cv::ocl::setUseOpenCL(false);
  }
  else
  {
    if (!cv::ocl::haveOpenCL())
    {
      printf("OpenCL not available\n");
    }
    else
    {
      cv::ocl::setUseOpenCL(true);

      cv::ocl::Context context = cv::ocl::Context::getDefault();
      if (context.ndevices() > 0)
      {
        cv::ocl::Device dev = context.device(0);
        printf("OpenCL device: %s %s %s\n",
              dev.vendorName().c_str(),
              dev.name().c_str(),
              dev.version().c_str());
        have_opencl = true;
      }
      else
      {
        printf("OpenCL: no devices available\n");
      }
    }
  }

  {
    const int count = m_inputs.size();
    int refidx = m_reference;

    // Use middle image as reference if no option is given
    if (refidx < 0 || refidx >= count)
      refidx = count / 2;

    // Load the reference image
    // This is needed when processing the other images, so load it first.
    std::shared_ptr<Task_LoadImg> refcolor = std::make_shared<Task_LoadImg>(m_inputs.at(refidx));
    std::shared_ptr<Task_Grayscale> refgray = std::make_shared<Task_Grayscale>(refcolor);
    worker.add(refcolor);
    worker.add(refgray);

    // Construct list of indexes. Perform alignment from reference image outwards.
    // In very thick stacks, it is difficult to align outermost images directly
    // against the reference image because of heavy blurring. Instead this aligns
    // each image with its neighbour.
    std::vector<int> indexes;
    indexes.push_back(refidx);
    for (int i = 1; i < count; i++)
    {
      if (refidx - i >= 0) indexes.push_back(refidx - i);
      if (refidx + i < count) indexes.push_back(refidx + i);
    }

    // This loop goes through input images and processes them up to the merge step.
    // Merging is done in batches to reduce memory usage.
    const int batch_size = 8;
    std::vector<std::shared_ptr<Task_LoadImg> > input_imgs(count);
    std::vector<std::shared_ptr<ImgTask> > grayscale_imgs(count);
    std::vector<std::shared_ptr<Task_Align> > aligned_imgs(count);
    std::vector<std::shared_ptr<ImgTask> > aligned_grayscales(count);
    std::vector<std::shared_ptr<ImgTask> > merge_batch;
    for (int i : indexes)
    {
      std::shared_ptr<Task_LoadImg> color;
      std::shared_ptr<ImgTask> grayscale;

      if (i == refidx)
      {
        input_imgs.at(i) = refcolor;
        color = refcolor;
        grayscale = refgray;
        grayscale_imgs.at(i) = grayscale;
      }
      else
      {
        color = std::make_shared<Task_LoadImg>(m_inputs.at(i));
        worker.add(color);
        input_imgs.at(i) = color;

        // Convert image to grayscale
        // The reference image is used to calculate the best mapping, which is then used for all images.
        grayscale = std::make_shared<Task_Grayscale>(color, refgray);
        worker.add(grayscale);
        grayscale_imgs.at(i) = grayscale;
      }

      if (m_save_steps)
      {
        worker.add(std::make_shared<Task_SaveImg>("grayscale_" + grayscale->basename(), grayscale));
      }

      // Perform image alignment, using the neighbour image as initial guess or reference
      std::shared_ptr<Task_Align> aligned;
      int neighbour = refidx;
      if (i < refidx) neighbour = i + 1;
      if (i > refidx) neighbour = i - 1;
      if (i != refidx)
      {
        if (m_align_flags & ALIGN_GLOBAL)
        {
          // Align directly against the global reference, but use neighbour as a guess.
          // This can give slightly better alignment in shallow stacks with little blur.
          aligned = std::make_shared<Task_Align>(aligned_grayscales.at(refidx),
                                                 aligned_imgs.at(refidx),
                                                 grayscale, color,
                                                 aligned_imgs.at(neighbour),
                                                 nullptr,
                                                 input_imgs.at(i),
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
          aligned = std::make_shared<Task_Align>(grayscale_imgs.at(neighbour),
                                                 input_imgs.at(neighbour),
                                                 grayscale, color,
                                                 nullptr,
                                                 aligned_imgs.at(neighbour),
                                                 input_imgs.at(i),
                                                 m_align_flags);
        }
      }
      else
      {
        // Nothing to be done for the global reference image, but we run it through Task_Align
        // to make the types match.
        aligned = std::make_shared<Task_Align>(refgray, refcolor, refgray, refcolor);
      }
      aligned_imgs.at(i) = aligned;
      worker.add(aligned);

      if (m_save_steps)
      {
        // Task_Align adds "aligned_" prefix to the filename, so just use that name for saving also.
        worker.add(std::make_shared<Task_SaveImg>(aligned->filename(), aligned));
      }

      // Convert aligned image to grayscale again.
      // We could also transform the grayscale images directly, but a new grayscale conversion is faster
      // and results in less difference between the color and grayscale versions.
      std::shared_ptr<ImgTask> aligned_grayscale = std::make_shared<Task_Grayscale>(aligned, refgray);
      worker.add(aligned_grayscale);
      aligned_grayscales.at(i) = aligned_grayscale;

      // Wavelet transform the image
      std::shared_ptr<ImgTask> wavelet;
      if (have_opencl)
      {
        wavelet = std::make_shared<Task_Wavelet_OpenCL>(aligned_grayscale, false);
      }
      else
      {
        wavelet = std::make_shared<Task_Wavelet>(aligned_grayscale, false);
      }
      worker.add(wavelet);
      merge_batch.push_back(wavelet);

      if (merge_batch.size() >= batch_size)
      {
        // Merge wavelet images accumulated so far
        std::shared_ptr<ImgTask> merged = std::make_shared<Task_Merge>(merge_batch, m_consistency);
        worker.add(merged);

        merge_batch.clear();
        merge_batch.push_back(merged);
      }
    }

    // Merge the final batch of images
    std::shared_ptr<ImgTask> merged_wavelet;
    if (merge_batch.size() == 1)
    {
      merged_wavelet = merge_batch.front();
    }
    else
    {
      merged_wavelet = std::make_shared<Task_Merge>(merge_batch, m_consistency);
      worker.add(merged_wavelet);
    }

    // Denoise merged image
    std::shared_ptr<ImgTask> denoised = std::make_shared<Task_Denoise>(merged_wavelet, m_denoise);
    worker.add(denoised);

    // Inverse-transform merged image
    std::shared_ptr<ImgTask> merged_gray;
    if (have_opencl)
    {
      merged_gray = std::make_shared<Task_Wavelet>(denoised, true);
    }
    else
    {
      merged_gray = std::make_shared<Task_Wavelet_OpenCL>(denoised, true);
    }
    worker.add(merged_gray);

    if (m_save_steps)
    {
      worker.add(std::make_shared<Task_SaveImg>(merged_gray->filename(), merged_gray));
    }

    // Reassign pixel values
    std::vector<std::shared_ptr<ImgTask>> tmp(aligned_imgs.begin(), aligned_imgs.end()); // Convert to base class pointer
    std::shared_ptr<ImgTask> reassigntask = std::make_shared<Task_Reassign>(aligned_grayscales, tmp, merged_gray);
    std::shared_ptr<ImgTask> reassigned = reassigntask;
    worker.add(std::move(reassigntask));

    // Save result image
    worker.add(std::make_shared<Task_SaveImg>(m_output, reassigned, refcolor));

  } // Close scope to avoid holding onto the shared_ptr's in local variables

  worker.wait_all();

  return !worker.failed();
}

