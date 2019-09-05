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

  // Number of levels in the wavelet decomposition
  static const int levels = 6;

private:
  virtual void task();

  static void decompose_multilevel(const cv::Mat &input, cv::Mat &output, int levelcount);
  static void decompose(const cv::Mat &input, cv::Mat &output);
  static void decompose_1d(const cv::Mat &src, cv::Mat &dest, bool vertical, bool imag_filter);

  static void compose_multilevel(const cv::Mat &input, cv::Mat &output, int levelcount);
  static void compose(const cv::Mat &input, cv::Mat &output);
  static void compose_1d(const cv::Mat &src, cv::Mat &dest, bool vertical, bool imag_filter);

  std::shared_ptr<ImgTask> m_input;
  bool m_inverse;

  // For unit testing
  friend class Task_Wavelet_Roundtrip1D_Test;
  friend class Task_Wavelet_Decompose2D_Test;
  friend class Task_Wavelet_Roundtrip2D_Test;
  friend class Task_Wavelet_Multilevel_Test;
};

}
