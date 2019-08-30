// Performs forward or inverse wavelet transformation on an image.

#pragma once
#include "worker.hh"

namespace focusstack {

class Task_Wavelet
{
public:
  Task_Wavelet(std::shared_ptr<ImgTask> input, bool inverse);

  virtual const cv::Mat &img() const { return m_result; }

  virtual std::string filename() const { return m_input.filename(); }
  virtual std::string name() const;

private:
  virtual void task();

  std::shared_ptr<ImgTask> m_input;
  bool m_inverse;
  cv::Mat m_result;
};

}
