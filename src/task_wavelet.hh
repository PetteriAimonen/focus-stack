// Performs forward or inverse wavelet transformation on an image.
// Uses Daubechies wavelets as described in
//  "Image Processing with Complex Daubechies Wavelets", J.M. Lina, 1997
//
// Similar algorithm is available at https://github.com/fiji-BIG/wavelets/

#pragma once
#include "worker.hh"

namespace focusstack {

class Task_Wavelet: public ImgTask
{
public:
  Task_Wavelet(std::shared_ptr<ImgTask> input, bool inverse);

  // Decide the number of decomposition levels that will be
  // used for given image size. Ideally (1 << levels) should
  // be larger than largest blur in the image, but small enough
  // that the image doesn't need to be unnecessarily padded
  // in Task_LoadImg.
  // If expanded_size is given, it is set to image size that
  // is equal or larger than input and divisible by (1 << levels).
  static int levels_for_size(cv::Size size, cv::Size *expanded_size = nullptr);

  // Range of return values for levels_for_size().
  static const int min_levels = 5;
  static const int max_levels = 10;

protected:
  virtual void task();

  std::shared_ptr<ImgTask> m_input;
  bool m_inverse;
};

}
