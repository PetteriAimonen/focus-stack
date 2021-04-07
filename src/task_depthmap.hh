// Construct spatial depthmap from the wavelet depthmap.

#pragma once
#include "worker.hh"
#include "task_merge.hh"

namespace focusstack {

class Task_Depthmap: public ImgTask
{
public:
  Task_Depthmap(std::shared_ptr<Task_Merge> merged_wavelet, float smoothing, int max_depth);

private:
  virtual void task();

  std::shared_ptr<Task_Merge> m_merged_wavelet;
  float m_smoothing;
  int m_max_depth;
};

}
