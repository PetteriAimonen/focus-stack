#include <gtest/gtest.h>
#include <opencv2/core/ocl.hpp>
#include "task_wavelet_templates.hh"

namespace focusstack {

// Decompose with OpenCL, compose with CPU
TEST(Task_Wavelet_OpenCL, Decompose1D) {
  if (!cv::ocl::haveOpenCL()) GTEST_SKIP();
  cv::ocl::setUseOpenCL(true);

  cv::Mat input(1, 16, CV_32FC2);
  input = cv::Vec2f(0, 0);
  input(cv::Rect(8, 0, 8, 1)) = cv::Vec2f(1.0f, 0.0f);

  cv::Mat wavelet(1, 16, CV_32FC2);

  {
    cv::UMat uinput(1, 16, CV_32FC2);
    input.copyTo(uinput);

    cv::UMat uwavelet(1, 16, CV_32FC2);
    Wavelet<cv::UMat>::decompose_1d(uinput, uwavelet, false);
    uwavelet.copyTo(wavelet);
  }

  cv::Mat composed(1, 16, CV_32FC2);
  Wavelet<cv::Mat>::compose_1d(wavelet, composed, false);

  for (int i = 0; i < 16; i++)
  {
    float delta_re = std::abs(input.at<cv::Vec2f>(i)[0] - composed.at<cv::Vec2f>(i)[0]);
    float delta_im = std::abs(input.at<cv::Vec2f>(i)[1] - composed.at<cv::Vec2f>(i)[1]);

    ASSERT_LE(delta_re, 0.001f);
    ASSERT_LE(delta_im, 0.001f);
  }
}

TEST(Task_Wavelet_OpenCL, Roundtrip1D) {
  if (!cv::ocl::haveOpenCL()) GTEST_SKIP();

  cv::Mat input(1, 16, CV_32FC2);
  cv::Mat composed(1, 16, CV_32FC2);

  input = cv::Vec2f(0, 0);
  input(cv::Rect(8, 0, 8, 1)) = cv::Vec2f(1.0f, 0.0f);

  {
    cv::UMat uinput(1, 16, CV_32FC2);
    input.copyTo(uinput);

    cv::UMat uwavelet(1, 16, CV_32FC2);
    cv::UMat ucomposed(1, 16, CV_32FC2);
    Wavelet<cv::UMat>::decompose_1d(uinput, uwavelet, false);
    Wavelet<cv::UMat>::compose_1d(uwavelet, ucomposed, false);

    ucomposed.copyTo(composed);
  }

//   printf("Real: ");
//   for (int x = 0; x < 16; x++)
//   {
//     printf("%8.3f, ", composed.at<cv::Vec2f>(0, x)[0]);
//   }
//   printf("\n");
//
//   printf("Imag: ");
//   for (int x = 0; x < 16; x++)
//   {
//     printf("%8.3f, ", composed.at<cv::Vec2f>(0, x)[1]);
//   }
//   printf("\n");

  for (int i = 0; i < 16; i++)
  {
    float delta_re = std::abs(input.at<cv::Vec2f>(i)[0] - composed.at<cv::Vec2f>(i)[0]);
    float delta_im = std::abs(input.at<cv::Vec2f>(i)[1] - composed.at<cv::Vec2f>(i)[1]);

    ASSERT_LE(delta_re, 0.001f);
    ASSERT_LE(delta_im, 0.001f);
  }
}

TEST(Task_Wavelet_OpenCL, Decompose2D) {
  if (!cv::ocl::haveOpenCL()) GTEST_SKIP();

  cv::Mat input(8, 8, CV_32FC2);
  cv::Mat wavelet(8, 8, CV_32FC2);

  input = cv::Vec2f(0, 0);
  input(cv::Rect(0, 0, 2, 4)) = cv::Vec2f(1.0f, 0.0f);

  {
    cv::UMat uinput(8, 8, CV_32FC2);
    input.copyTo(uinput);

    cv::UMat uwavelet(8, 8, CV_32FC2);
    Wavelet<cv::UMat>::decompose(uinput, uwavelet);
    uwavelet.copyTo(wavelet);
  }

  // These arrays were obtained by running https://github.com/fiji-BIG/wavelets/
  // implementation, to verify the algorithms behave identically.

  const float expected_real[8][8] = {
    {   0.547,    0.547,   -0.047,   -0.047,   -0.391,    0.391,   -0.047,    0.047, },
    {   1.182,    1.182,   -0.088,   -0.088,   -0.898,    0.898,   -0.117,    0.117, },
    {   0.547,    0.547,   -0.047,   -0.047,   -0.391,    0.391,   -0.047,    0.047, },
    {  -0.088,   -0.088,   -0.006,   -0.006,    0.117,   -0.117,    0.023,   -0.023, },
    {  -0.391,   -0.391,    0.047,    0.047,    0.225,   -0.225,    0.018,   -0.018, },
    {  -0.000,   -0.000,    0.000,    0.000,    0.000,   -0.000,    0.000,   -0.000, },
    {   0.391,    0.391,   -0.047,   -0.047,   -0.225,    0.225,   -0.018,    0.018, },
    {   0.000,    0.000,    0.000,    0.000,    0.000,    0.000,    0.000,    0.000, }
  };

  const float expected_imag[8][8] = {
    {   0.061,    0.061,   -0.061,   -0.061,    0.182,   -0.182,    0.061,   -0.061, },
    {   0.265,    0.265,   -0.144,   -0.144,    0.303,   -0.303,    0.121,   -0.121, },
    {   0.061,    0.061,   -0.061,   -0.061,    0.182,   -0.182,    0.061,   -0.061, },
    {  -0.144,   -0.144,    0.023,    0.023,    0.061,   -0.061,    0.000,    0.000, },
    {   0.091,    0.091,    0.030,    0.030,   -0.219,    0.219,   -0.053,    0.053, },
    {   0.000,    0.000,    0.000,    0.000,   -0.000,    0.000,   -0.000,    0.000, },
    {  -0.091,   -0.091,   -0.030,   -0.030,    0.219,   -0.219,    0.053,   -0.053, },
    {   0.000,    0.000,    0.000,    0.000,    0.000,    0.000,    0.000,    0.000, },
  };

  for (int y = 0; y < 8; y++)
  {
    for (int x = 0; x < 8; x++)
    {
      ASSERT_LE(std::abs(wavelet.at<cv::Vec2f>(y, x)[0] - expected_real[y][x]), 0.002f);
      ASSERT_LE(std::abs(wavelet.at<cv::Vec2f>(y, x)[1] - expected_imag[y][x]), 0.002f);
    }
  }
}

TEST(Task_Wavelet_OpenCL, Roundtrip2D) {
  if (!cv::ocl::haveOpenCL()) GTEST_SKIP();

  cv::Mat input(8, 8, CV_32FC2);
  cv::Mat output(8, 8, CV_32FC2);

  input = cv::Vec2f(0, 0);
  input(cv::Rect(0, 0, 2, 4)) = cv::Vec2f(1.0f, 0.0f);

  {
    cv::UMat uinput(8, 8, CV_32FC2);
    input.copyTo(uinput);

    cv::UMat uwavelet(8, 8, CV_32FC2);
    cv::UMat uoutput(8, 8, CV_32FC2);
    Wavelet<cv::UMat>::decompose(uinput, uwavelet);
    Wavelet<cv::UMat>::compose(uwavelet, uoutput);
    uoutput.copyTo(output);
  }

//   printf("Real:\n");
//   for (int y = 0; y < 8; y++)
//   {
//     for (int x = 0; x < 8; x++)
//     {
//       printf("%8.3f, ", output.at<cv::Vec2f>(y, x)[0]);
//     }
//     printf("\n");
//   }
//
//   printf("\nImag:\n");
//   for (int y = 0; y < 8; y++)
//   {
//     for (int x = 0; x < 8; x++)
//     {
//       printf("%8.3f, ", output.at<cv::Vec2f>(y, x)[1]);
//     }
//     printf("\n");
//   }

  for (int y = 0; y < 8; y++)
  {
    for (int x = 0; x < 8; x++)
    {
      ASSERT_LE(std::abs(output.at<cv::Vec2f>(y, x)[0] - input.at<cv::Vec2f>(y, x)[0]), 0.002f);
      ASSERT_LE(std::abs(output.at<cv::Vec2f>(y, x)[1] - input.at<cv::Vec2f>(y, x)[1]), 0.002f);
    }
  }
}

TEST(Task_Wavelet_OpenCL, Multilevel) {
  if (!cv::ocl::haveOpenCL()) GTEST_SKIP();

  cv::Mat input(16, 16, CV_32FC2);
  cv::Mat output(16, 16, CV_32FC2);

  input = cv::Vec2f(0, 0);
  input(cv::Rect(0, 0, 2, 4)) = cv::Vec2f(1.0f, 0.0f);

  {
    cv::UMat uinput(16, 16, CV_32FC2);
    input.copyTo(uinput);

    cv::UMat uwavelet(16, 16, CV_32FC2);
    cv::UMat uoutput(16, 16, CV_32FC2);

    Wavelet<cv::UMat>::decompose_multilevel(uinput, uwavelet, 3);
    Wavelet<cv::UMat>::compose_multilevel(uwavelet, uoutput, 3);
    uoutput.copyTo(output);
  }

  for (int y = 0; y < 16; y++)
  {
    for (int x = 0; x < 16; x++)
    {
      ASSERT_LE(std::abs(output.at<cv::Vec2f>(y, x)[0] - input.at<cv::Vec2f>(y, x)[0]), 0.002f);
      ASSERT_LE(std::abs(output.at<cv::Vec2f>(y, x)[1] - input.at<cv::Vec2f>(y, x)[1]), 0.002f);
    }
  }
}

}
