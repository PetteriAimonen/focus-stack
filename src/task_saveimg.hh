// Handles saving of images

#pragma once
#include "worker.hh"

namespace focusstack {

class Task_SaveImg: public Task
{
public:
  Task_SaveImg(std::string filename, ImgTask &input);

  virtual void run();

  virtual std::string filename() const { return m_filename; }
  virtual std::string name() const { return "Save " + filename(); }

private:
  std::string m_filename;
};

}
