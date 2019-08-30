// Performs pixel reassignment after inverse wavelet transform.
// Refer to "Complex Wavelets for Extended Depth-of-Field: A New Method for the Fusion of Multichannel Microscopy Images"
//          by B. Forster, D. Van De Ville, J. Berent, D. Sage, M. Unser, September 2004

#pragma once
#include "worker.hh"

namespace focusstack {

class Task_Reassign: public ImgTask
{
public:
  Task_Reassign(const std::vector<std::shared_ptr<ImgTask> > &grayscale_imgs,
                const std::vector<std::shared_ptr<ImgTask> > &color_imgs,
                std::shared_ptr<ImgTask> merged);

private:
  virtual void task();

  std::vector<std::shared_ptr<ImgTask> > m_grayscale_imgs;
  std::vector<std::shared_ptr<ImgTask> > m_color_imgs;
  std::shared_ptr<ImgTask> m_merged;
};

}
