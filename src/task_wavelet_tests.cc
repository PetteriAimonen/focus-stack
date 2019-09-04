#include <gtest/gtest.h>
#include "task_wavelet.hh"

namespace focusstack {

TEST(Task_Wavelet, Roundtrip1D) {
  cv::Mat input(1, 16, CV_32FC2);
  cv::Mat wavelet(1, 16, CV_32FC2);
  cv::Mat composed(1, 16, CV_32FC2);

  input = cv::Vec2f(0, 0);
  input(cv::Rect(8, 0, 8, 1)) = cv::Vec2f(1.0f, 0.0f);

  Task_Wavelet::decompose_1d(input, wavelet, false, false);
  Task_Wavelet::compose_1d(wavelet, composed, false, false);

  for (int i = 0; i < 16; i++)
  {
    float delta_re = std::abs(input.at<cv::Vec2f>(i)[0] - composed.at<cv::Vec2f>(i)[0]);
    float delta_im = std::abs(input.at<cv::Vec2f>(i)[1] - composed.at<cv::Vec2f>(i)[1]);

    ASSERT_LE(delta_re, 0.05f);
    ASSERT_LE(delta_im, 0.05f);
  }
}

}
