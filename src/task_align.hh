// Aligns one image against a reference image.
// Algorithm is based on "A Pyramid Approach to Subpixel Registration Based on Intensity" by
//                       P. Thévenaz, U.E. Ruttimann, M. Unser, January 1998
//
// Alignment is calculated on grayscale image, but final transformation is applied to the color image.

#pragma once
#include "worker.hh"

namespace focusstack {

class Task_Align: public ImgTask
{
public:
  Task_Align(std::shared_ptr<ImgTask> reference, std::shared_ptr<ImgTask> grayscale, std::shared_ptr<ImgTask> color);

  virtual const cv::Mat &img() const { return m_result; }

  virtual std::string filename() const { return m_color.filename(); }
  virtual std::string name() const { return "Align " + filename(); }

private:
  virtual void task();

  std::shared_ptr<ImgTask> m_reference;
  std::shared_ptr<ImgTask> m_grayscale;
  std::shared_ptr<ImgTask> m_color;
  cv::Mat m_result;
};

}
