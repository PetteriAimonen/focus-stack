// Performs pixel reassignment after inverse wavelet transform.
// Refer to "Complex Wavelets for Extended Depth-of-Field: A New Method for the Fusion of Multichannel Microscopy Images"
//          by B. Forster, D. Van De Ville, J. Berent, D. Sage, M. Unser, September 2004

#pragma once
#include "worker.hh"

namespace focusstack {

class Task_Reassign: public ImgTask
{
public:
  Task_Reassign(const std::vector<std::shared_ptr<ImgTask> > &images, std::shared_ptr<ImgTask> merged);

  virtual const cv::Mat &img() const { return m_result; }

  virtual void run();

  virtual std::string filename() const { return m_merged->filename(); }
  virtual std::string name() const { return "Reassign pixel values"; }

private:
  std::vector<std::shared_ptr<ImgTask> > m_images;
  std::shared_ptr<ImgTask> m_merged;
  cv::Mat m_result;
};

}
