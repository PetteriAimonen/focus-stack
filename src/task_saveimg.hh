// Handles cropping and conversion of images for saving

#pragma once
#include "worker.hh"
#include "task_loadimg.hh"

namespace focusstack {

class Task_SaveImg: public ImgTask
{
public:
  Task_SaveImg(std::string filename, std::shared_ptr<ImgTask> input, int jpgquality = 99, bool nocrop = false);

private:
  virtual void task();

  std::shared_ptr<ImgTask> m_input;
  bool m_nocrop;
};

}
