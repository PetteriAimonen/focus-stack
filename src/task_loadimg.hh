// Handles loading of images

#pragma once
#include "worker.hh"

namespace focusstack {

class Task_LoadImg: public ImgTask
{
public:
  Task_LoadImg(std::string filename, float wait_images = 0.0f);
  Task_LoadImg(std::string name, const cv::Mat &img);

  virtual bool ready_to_run();

  cv::Size orig_size() const { return m_orig_size; }

private:
  virtual void task();

  float m_wait_images;
  std::chrono::system_clock::time_point m_wait_images_until;
  bool m_memimg;
  cv::Size m_orig_size;
};


}
