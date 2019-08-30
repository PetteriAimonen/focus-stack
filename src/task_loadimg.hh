// Handles loading of images

#pragma once
#include "worker.hh"

namespace focusstack {

class Task_LoadImg: public ImgTask
{
public:
  Task_LoadImg(std::string filename);

  virtual const cv::Mat &img() const { return m_result; }

  virtual std::string filename() const { return m_filename; }
  virtual std::string name() const { return "Load " + filename(); }

private:
  virtual void task();

  std::string m_filename;
  cv::Mat m_result;
};


}
