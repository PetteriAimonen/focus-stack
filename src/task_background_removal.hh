// Perform histogram-based segmentation of image.
// This can be used for background removal when there is a clear brightness difference.

#pragma once
#include "worker.hh"

namespace focusstack {

class Task_BackgroundRemoval: public ImgTask
{
public:
  Task_BackgroundRemoval(std::shared_ptr<ImgTask> input, int threshold = 32, int gapsize = 100);

private:
  virtual void task();

  std::shared_ptr<ImgTask> m_input;
  int m_threshold;
  int m_gapsize;
};


}