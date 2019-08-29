// Merges multiple wavelet images to a stacked image.
// Refer to "Complex Wavelets for Extended Depth-of-Field: A New Method for the Fusion of Multichannel Microscopy Images"
//          by B. Forster, D. Van De Ville, J. Berent, D. Sage, M. Unser, September 2004

#pragma once
#include "worker.hh"

namespace focusstack {

class Task_Merge: public ImgTask
{
public:
  Task_Merge(const std::vector<ImgTask&> &images);

  virtual const cv::Mat &img_gray() const { return m_result; }

  virtual void run();

  virtual std::string filename() const { return "merge result"; }
  virtual std::string name() const { return "Merge " + std::to_string(m_images.size()) + " images"; }

private:
  std::vector<ImgTask&> m_images;
  cv::Mat m_result;
};

}
