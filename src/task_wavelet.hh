// Performs forward or inverse wavelet transformation on an image.
// Uses Daubechies wavelets as described in
//  "Image Processing with Complex Daubechies Wavelets", J.M. Lina, 1997
//
// Similar algorithm is available at
// https://github.com/fiji-BIG/wavelets/blob/master/src/main/java/wavelets/ComplexWavelet.java

#pragma once
#include "worker.hh"

namespace focusstack {

class Task_Wavelet: public ImgTask
{
public:
  Task_Wavelet(std::shared_ptr<ImgTask> input, bool inverse);

  // Number of levels in the wavelet decomposition
  static const int levels = 3;

private:
  virtual void task();

  static void decompose(const cv::Mat &input, cv::Mat &output);
  static void decompose_1d(const cv::Mat &src, cv::Mat &dest, bool vertical, bool imag_filter);

  static void compose(const cv::Mat &input, cv::Mat &output);
  static void compose_1d(const cv::Mat &src, cv::Mat &dest, bool vertical, bool imag_filter);

  std::shared_ptr<ImgTask> m_input;
  bool m_inverse;

  // For unit testing
  friend class Task_Wavelet_Roundtrip1D_Test;
};

}
