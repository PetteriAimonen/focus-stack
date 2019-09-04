// Converts image to grayscale using principal component analysis.
// This provides best contrast for alignment tasks.

#pragma once
#include "worker.hh"

namespace focusstack {

class Task_Grayscale: public ImgTask
{
public:
  // If reference is given, this uses the same grayscale conversion map as that task.
  Task_Grayscale(std::shared_ptr<ImgTask> input, std::shared_ptr<Task_Grayscale> reference = nullptr);

  const cv::Mat &weights() const { return m_weights; };

private:
  virtual void task();

  void do_pca();

  std::shared_ptr<ImgTask> m_input;
  std::shared_ptr<Task_Grayscale> m_reference;

  cv::Mat m_weights;
};

}
