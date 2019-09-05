// Handles loading of images

#pragma once
#include "worker.hh"

namespace focusstack {

class Task_LoadImg: public ImgTask
{
public:
  Task_LoadImg(std::string filename);

  cv::Size orig_size() const { return m_orig_size; }

private:
  virtual void task();

  cv::Size m_orig_size;
};


}
