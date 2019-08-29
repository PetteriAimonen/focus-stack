// Performs forward or inverse wavelet transformation on an image.

#pragma once
#include "worker.hh"

namespace focusstack {

class Task_Wavelet
{
public:
  Task_Wavelet(ImgTask &input, bool inverse);

  virtual const cv::Mat &img_gray() const { return m_result; }

  virtual void run();

  virtual std::string filename() const { return m_input.filename(); }
  virtual std::string name() const;

private:
  ImgTask &m_input;
  bool m_inverse;
  cv::Mat m_result;
};

}
