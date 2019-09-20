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

protected:
  virtual void task();

  std::shared_ptr<ImgTask> m_input;
  bool m_inverse;
};

}
