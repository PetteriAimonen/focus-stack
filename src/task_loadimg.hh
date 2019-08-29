// Handles loading of images

#pragma once
#include "worker.hh"

namespace focusstack {

class Task_LoadImg: public ImgTask
{
public:
  Task_LoadImg(std::string filename);

  virtual const cv::Mat &img_rgb() const { return m_rgb; }
  virtual const cv::Mat &img_gray() const { return m_gray; }

  virtual void run();

  virtual std::string filename() const { return m_filename; }
  virtual std::string name() const { return "Load " + filename(); }

private:
  std::string m_filename;
  cv::Mat m_rgb;
  cv::Mat m_gray;
};


}
