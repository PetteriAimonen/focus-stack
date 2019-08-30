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

  virtual const cv::Mat &img() const { return m_result; }

  virtual std::string filename() const { return m_input->filename(); }
  virtual std::string name() const { return "Grayscale " + filename(); }

private:
  virtual void task();

  std::shared_ptr<ImgTask> m_input;
  std::shared_ptr<Task_Grayscale> m_reference;
  cv::Mat m_result;
};

}
