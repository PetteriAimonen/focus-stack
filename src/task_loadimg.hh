// Handles loading of images

#pragma once
#include "worker.hh"

namespace focusstack {

class Task_LoadImg: public ImgTask
{
public:
  Task_LoadImg(std::string filename);

private:
  virtual void task();
};


}
