#include "focusstack.hh"
#include "worker.hh"
#include "task_loadimg.hh"
#include "task_grayscale.hh"
#include "task_align.hh"
#include "task_wavelet.hh"
#include "task_merge.hh"
#include "task_denoise.hh"
#include "task_reassign.hh"
#include "task_saveimg.hh"
#include <thread>

using namespace focusstack;

FocusStack::FocusStack():
  m_output("output.jpg"), m_save_steps(false), m_verbose(false),
  m_threads(std::thread::hardware_concurrency()),
  m_reference(-1),
  m_consistency(0),
  m_denoise(0)
{
}

void FocusStack::run()
{
  Worker worker(m_threads, m_verbose);

  {
    // Use middle image as reference if no option is given
    if (m_reference < 0 || m_reference >= m_inputs.size())
      m_reference = m_inputs.size() / 2;

    // Load the reference image
    // This is needed when processing the other images, so load it first.
    std::shared_ptr<Task_LoadImg> refcolor = std::make_shared<Task_LoadImg>(m_inputs.at(m_reference));
    std::shared_ptr<Task_Grayscale> refgray = std::make_shared<Task_Grayscale>(refcolor);
    worker.add(refcolor);
    worker.add(refgray);

    // This loop goes through input images and processes them up to the merge step.
    // Merging is done in batches to reduce memory usage.
    std::vector<std::shared_ptr<ImgTask> > input_imgs;
    std::vector<std::shared_ptr<ImgTask> > aligned_imgs;
    std::vector<std::shared_ptr<ImgTask> > aligned_grayscales;
    std::vector<std::shared_ptr<ImgTask> > merge_batch;
    for (int i = 0; i < m_inputs.size(); i++)
    {
      std::shared_ptr<Task_LoadImg> color;
      std::shared_ptr<ImgTask> grayscale;

      if (i == m_reference)
      {
        input_imgs.push_back(refcolor);
        color = refcolor;
        grayscale = refgray;
      }
      else
      {
        color = std::make_shared<Task_LoadImg>(m_inputs.at(i));
        worker.add(color);
        input_imgs.push_back(color);

        // Convert image to grayscale
        // The reference image is used to calculate the best mapping, which is then used for all images.
        grayscale = std::make_shared<Task_Grayscale>(color, refgray);
        worker.add(grayscale);
      }

      if (m_save_steps)
      {
        worker.add(std::make_shared<Task_SaveImg>("grayscale_" + grayscale->basename(), grayscale));
      }

      // Align image with respect to the reference
      std::shared_ptr<ImgTask> aligned = std::make_shared<Task_Align>(refgray, refcolor, grayscale, color);
      aligned_imgs.push_back(aligned);
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
      aligned_grayscales.push_back(aligned_grayscale);

      // Wavelet transform the image
      std::shared_ptr<ImgTask> wavelet = std::make_shared<Task_Wavelet>(aligned_grayscale, false);
      worker.add(wavelet);
      merge_batch.push_back(wavelet);

      if (merge_batch.size() >= m_threads)
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
    std::shared_ptr<ImgTask> merged_gray = std::make_shared<Task_Wavelet>(denoised, true);
    worker.add(merged_gray);

    if (m_save_steps)
    {
      worker.add(std::make_shared<Task_SaveImg>(merged_gray->filename(), merged_gray));
    }

    // Reassign pixel values
    std::shared_ptr<ImgTask> reassigntask = std::make_shared<Task_Reassign>(aligned_grayscales, aligned_imgs, merged_gray);
    std::shared_ptr<ImgTask> reassigned = reassigntask;
    worker.add(std::move(reassigntask));

    // Save result image
    worker.add(std::make_shared<Task_SaveImg>(m_output, reassigned, refcolor));

  } // Close scope to avoid holding onto the shared_ptr's in local variables

  worker.wait_all();
}

