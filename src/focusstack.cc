#include "focusstack.hh"
#include "worker.hh"
#include "task_loadimg.hh"
#include "task_align.hh"
#include "task_wavelet.hh"
#include "task_merge.hh"
#include "task_reassign.hh"
#include "task_saveimg.hh"
#include <thread>

using namespace focusstack;

FocusStack::FocusStack():
  m_output("output.jpg"), m_save_aligned(false), m_verbose(false), m_threads(std::thread::hardware_concurrency())
{
}

void FocusStack::run()
{
  Worker worker(m_threads, m_verbose);

  // Load input images
  std::vector<ImgTask&> input_imgs;
  for (std::string filename: m_inputs)
  {
    std::unique_ptr<ImgTask> task = std::make_unique<Task_LoadImg>(filename);
    input_imgs.push_back(*task);
    worker.add(std::move(task));
  }

  // Align other images with respect to the middle one
  int refidx = input_imgs.size() / 2;
  std::vector<ImgTask&> aligned_imgs;
  for (int i = 0; i < input_imgs.size(); i++)
  {
    if (i == refidx)
    {
      aligned_imgs.push_back(input_imgs.at(i));
    }
    else
    {
      std::unique_ptr<ImgTask> task = std::make_unique<Task_Align>(input_imgs.at(refidx), input_imgs.at(i));
      aligned_imgs.push_back(*task);
      worker.add(std::move(task));
    }
  }

  // Save aligned images if requested
  if (m_save_aligned)
  {
    for (ImgTask &img: aligned_imgs)
    {
      worker.add(std::make_unique<Task_SaveImg>("aligned_" + img.filename(), img));
    }
  }

  // Wavelet transform all images
  std::vector<ImgTask&> wavelets;
  for (int i = 0; i < aligned_imgs.size(); i++)
  {
    std::unique_ptr<ImgTask> task = std::make_unique<Task_Wavelet>(aligned_imgs.at(i), false);
    wavelets.push_back(*task);
    worker.add(std::move(task));
  }

  // Merge images
  std::unique_ptr<ImgTask> mergetask = std::make_unique<Task_Merge>(wavelets);
  ImgTask &merged = *mergetask;
  worker.add(std::move(mergetask));

  // Reassign pixel values
  std::unique_ptr<ImgTask> reassigntask = std::make_unique<Task_Reassign>(aligned_imgs, merged);
  ImgTask &reassigned = *reassigntask;
  worker.add(std::move(reassigntask));

  // Save result image
  worker.add(std::make_unique<Task_SaveImg>(m_output, reassigned));

  worker.runall();
}

