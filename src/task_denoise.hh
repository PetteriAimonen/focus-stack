// Performs image denoising, using the nonlinear wavelet denoising algorithm.

#pragma once
#include "worker.hh"

namespace focusstack {

class Task_Denoise: public ImgTask
{
public:
  Task_Denoise(std::shared_ptr<ImgTask> input, float level);

private:
  virtual void task();

  std::shared_ptr<ImgTask> m_input;
  float m_level;
};


}
