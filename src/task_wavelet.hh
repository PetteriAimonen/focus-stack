// Performs forward or inverse wavelet transformation on an image.

#pragma once
#include "worker.hh"

namespace focusstack {

class Task_Wavelet: public ImgTask
{
public:
  Task_Wavelet(std::shared_ptr<ImgTask> input, bool inverse);

private:
  virtual void task();

  std::shared_ptr<ImgTask> m_input;
  bool m_inverse;
};

}
