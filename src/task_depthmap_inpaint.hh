// Interpolate areas of depthmap that couldn't be estimated from the focus data.
// This uses both the partial depthmap data and the merged grayscale image data.

#pragma once
#include "worker.hh"
#include "task_depthmap.hh"

namespace focusstack {

class Task_Depthmap_Inpaint: public ImgTask
{
public:
  Task_Depthmap_Inpaint(std::shared_ptr<Task_Depthmap> depthmap,
    int threshold = 16, int smooth_xy = 32, int smooth_z = 64,
    bool save_steps = false);

private:
  virtual void task();

  std::shared_ptr<Task_Depthmap> m_depthmap;
  int m_threshold;
  int m_smooth_xy;
  int m_smooth_z;
  bool m_save_steps;
};



}