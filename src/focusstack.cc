#include "focusstack.hh"
#include "worker.hh"
#include "task_loadimg.hh"
#include "task_grayscale.hh"
#include "task_align.hh"
#include "task_wavelet.hh"
#include "task_merge.hh"
#include "task_reassign.hh"
#include "task_saveimg.hh"
#include <thread>

using namespace focusstack;

FocusStack::FocusStack():
  m_output("output.jpg"), m_save_steps(false), m_verbose(false),
  m_threads(std::thread::hardware_concurrency()),
  m_reference(-1)
{
}

void FocusStack::run()
{
  Worker worker(m_threads, m_verbose);

  {
    // Use middle image as reference if no option is given
    if (m_reference < 0 || m_reference >= m_inputs.size())
      m_reference = m_inputs.size() / 2;

    // Load input images
    std::vector<std::shared_ptr<Task_LoadImg> > input_imgs;
    for (int i = 0; i < m_inputs.size(); i++)
    {
      std::shared_ptr<Task_LoadImg> task = std::make_shared<Task_LoadImg>(m_inputs.at(i));
      input_imgs.push_back(task);

      if (i == m_reference)
      {
        // Load reference first to have it available sooner
        worker.prepend(task);
      }
      else
      {
        worker.add(task);
      }
    }

    // Convert images to grayscale
    // The reference image is used to calculate the best mapping, which is then used for all images.
    // For that reason it is important that the tasks on reference image are prepended instead of waiting
    // behind all other load tasks.
    std::shared_ptr<Task_Grayscale> grayscale_reference = std::make_shared<Task_Grayscale>(input_imgs.at(m_reference));
    worker.prepend(grayscale_reference);

    std::vector<std::shared_ptr<ImgTask> > grayscale_imgs;
    for (int i = 0; i < input_imgs.size(); i++)
    {
      if (i != m_reference)
      {
        std::shared_ptr<ImgTask> task = std::make_shared<Task_Grayscale>(input_imgs.at(i), grayscale_reference);
        worker.add(task);
        grayscale_imgs.push_back(task);
      }
      else
      {
        grayscale_imgs.push_back(grayscale_reference);
      }
    }

    if (m_save_steps)
    {
      for (int i = 0; i < grayscale_imgs.size(); i++)
      {
        worker.add(std::make_shared<Task_SaveImg>("grayscale_" + grayscale_imgs.at(i)->basename(), grayscale_imgs.at(i)));
      }
    }

    // Align other images with respect to the reference
    std::vector<std::shared_ptr<ImgTask> > aligned_imgs;
    for (int i = 0; i < input_imgs.size(); i++)
    {
      std::shared_ptr<ImgTask> task = std::make_shared<Task_Align>(grayscale_reference, input_imgs.at(m_reference),
                                                                   grayscale_imgs.at(i), input_imgs.at(i));
      aligned_imgs.push_back(task);
      worker.add(std::move(task));
    }

    // Save aligned images if requested
    if (m_save_steps)
    {
      for (std::shared_ptr<ImgTask> img: aligned_imgs)
      {
        // Task_Align adds "aligned_" prefix to the filename, so just use that name for saving also.
        worker.add(std::make_shared<Task_SaveImg>(img->filename(), img));
      }
    }

    // Convert aligned images to grayscale again.
    // We could also transform the grayscale images directly, but a new grayscale conversion is faster
    // and results in less difference between the color and grayscale versions.
    std::vector<std::shared_ptr<ImgTask> > aligned_grayscale;
    for (int i = 0; i < aligned_imgs.size(); i++)
    {
      std::shared_ptr<ImgTask> task = std::make_shared<Task_Grayscale>(aligned_imgs.at(i), grayscale_reference);
      aligned_grayscale.push_back(task);
      worker.add(task);
    }

    // Wavelet transform all images
    std::vector<std::shared_ptr<ImgTask> > wavelets;
    for (int i = 0; i < aligned_imgs.size(); i++)
    {
      std::shared_ptr<ImgTask> task = std::make_shared<Task_Wavelet>(aligned_grayscale.at(i), false);
      wavelets.push_back(task);
      worker.add(std::move(task));
    }

    if (m_save_steps)
    {
      for (int i = 0; i < aligned_imgs.size(); i++)
      {
        worker.add(std::make_shared<Task_SaveImg>("wavelet_" + std::to_string(i) + ".png", wavelets.at(i)));
      }
    }

    // Merge images
    std::shared_ptr<ImgTask> merged_wavelet = std::make_shared<Task_Merge>(wavelets);
    worker.add(merged_wavelet);

    // Inverse-transform merged image
    std::shared_ptr<ImgTask> merged = std::make_shared<Task_Wavelet>(merged_wavelet, true);
    worker.add(merged);

    if (m_save_steps)
    {
      worker.add(std::make_shared<Task_SaveImg>(merged->filename(), merged));
    }

    // Reassign pixel values
    std::shared_ptr<ImgTask> reassigntask = std::make_shared<Task_Reassign>(aligned_grayscale, aligned_imgs, merged);
    std::shared_ptr<ImgTask> reassigned = reassigntask;
    worker.add(std::move(reassigntask));

    // Save result image
    worker.add(std::make_shared<Task_SaveImg>(m_output, reassigned, input_imgs.at(m_reference)));

  } // Close scope to avoid holding onto the shared_ptr's in local variables

  worker.wait_all();
}

