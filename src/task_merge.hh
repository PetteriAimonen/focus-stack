// Merges multiple wavelet images to a stacked image.
// Refer to "Complex Wavelets for Extended Depth-of-Field: A New Method for the Fusion of Multichannel Microscopy Images"
//          by B. Forster, D. Van De Ville, J. Berent, D. Sage, M. Unser, September 2004

#pragma once
#include "worker.hh"

namespace focusstack {

class Task_Merge: public ImgTask
{
public:
  Task_Merge(const std::vector<std::shared_ptr<ImgTask> > &images, int consistency);

private:
  virtual void task();

  static void get_sq_absval(const cv::Mat &complex_mat, cv::Mat &absval);

  void denoise_subbands(cv::Mat &depthmap);
  void denoise_neighbours(cv::Mat &depthmap);

  std::vector<std::shared_ptr<ImgTask> > m_images;
  int m_consistency;
};

}
