// Handles saving of images

#pragma once
#include "worker.hh"

namespace focusstack {

class Task_SaveImg: public Task
{
public:
  Task_SaveImg(std::string filename, std::shared_ptr<ImgTask> input);

  virtual std::string filename() const { return m_filename; }
  virtual std::string name() const { return "Save " + filename(); }

private:
  virtual void task();

  std::string m_filename;
  std::shared_ptr<ImgTask> m_input;
};

}
