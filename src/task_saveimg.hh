// Handles saving of images

#pragma once
#include "worker.hh"
#include "task_loadimg.hh"

namespace focusstack {

class Task_SaveImg: public Task
{
public:
  Task_SaveImg(std::string filename, std::shared_ptr<ImgTask> input, int jpgquality, std::shared_ptr<Task_LoadImg> origsize = nullptr);

private:
  virtual void task();

  std::shared_ptr<ImgTask> m_input;
  std::shared_ptr<Task_LoadImg> m_origsize;
};

}
