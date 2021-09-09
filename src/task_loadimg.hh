// Handles loading of images

#pragma once
#include "worker.hh"

namespace focusstack {

class Task_LoadImg: public ImgTask
{
public:
  Task_LoadImg(std::string filename);
  Task_LoadImg(std::string name, const cv::Mat &img);

  cv::Size orig_size() const { return m_orig_size; }

private:
  virtual void task();

  bool m_memimg;
  cv::Size m_orig_size;
};


}
