// Merges multiple wavelet images to a stacked image.
// Refer to "Complex Wavelets for Extended Depth-of-Field: A New Method for the Fusion of Multichannel Microscopy Images"
//          by B. Forster, D. Van De Ville, J. Berent, D. Sage, M. Unser, September 2004

#pragma once
#include "worker.hh"
#include <unordered_map>

namespace focusstack {

class Task_Merge: public ImgTask
{
public:
  Task_Merge(std::shared_ptr<Task_Merge> prev_merge,
             const std::vector<std::shared_ptr<ImgTask> > &images,
             int consistency);

  const cv::Mat &depthmap() const { return m_depthmap; }

private:
  virtual void task();

  static void get_sq_absval(const cv::Mat &complex_mat, cv::Mat &absval);

  cv::Mat get_source_img(int index);
  void denoise_subbands();
  void denoise_neighbours();

  cv::Mat m_depthmap;

  std::unordered_map<int, std::shared_ptr<ImgTask> > m_index_map;
  std::shared_ptr<Task_Merge> m_prev_merge;
  std::vector<std::shared_ptr<ImgTask> > m_images;
  int m_consistency;
};

}
