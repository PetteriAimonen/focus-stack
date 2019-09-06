// Aligns one image against a reference image.
// Uses OpenCV findTransformECC() as the algorithm.
//
// Alignment is calculated on grayscale image, but final transformation is applied to the color image.

#pragma once
#include "worker.hh"
#include "task_loadimg.hh"

namespace focusstack {

class Task_Align: public ImgTask
{
public:
  Task_Align(std::shared_ptr<ImgTask> refgray, std::shared_ptr<Task_LoadImg> refcolor,
             std::shared_ptr<ImgTask> srcgray, std::shared_ptr<ImgTask> srccolor);

private:
  virtual void task();

  void match_contrast();
  void match_transform(int max_resolution, bool rough);
  void match_whitebalance();

  void apply_contrast_whitebalance(cv::Mat &img);
  void apply_transform(const cv::Mat &src, cv::Mat &dst, bool inverse);

  std::shared_ptr<ImgTask> m_refgray;
  std::shared_ptr<Task_LoadImg> m_refcolor;
  std::shared_ptr<ImgTask> m_srcgray;
  std::shared_ptr<ImgTask> m_srccolor;

  cv::Rect m_roi;
  cv::Mat m_transformation;
  cv::Mat m_contrast;
  cv::Mat m_whitebalance;
};

}
