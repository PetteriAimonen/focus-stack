// Aligns one image against a reference image.
// Uses OpenCV findTransformECC() as the algorithm.
//
// Alignment is calculated on grayscale image, but final transformation is applied to the color image.

#pragma once
#include "worker.hh"

namespace focusstack {

class Task_Align: public ImgTask
{
public:
  Task_Align(std::shared_ptr<ImgTask> reference, std::shared_ptr<ImgTask> grayscale, std::shared_ptr<ImgTask> color);

private:
  virtual void task();

  void match_contrast();
  void match_transform();

  void apply_transform(const cv::Mat &src, cv::Mat &dst, bool inverse);

  std::shared_ptr<ImgTask> m_reference;
  std::shared_ptr<ImgTask> m_grayscale;
  std::shared_ptr<ImgTask> m_color;

  cv::Mat m_transformation;
  float m_contrast;
};

}
