#include <gtest/gtest.h>
#include "radialfilter.hh"
#include <iostream>

namespace focusstack {

TEST(RadialFilter, average) {
  cv::Mat input(4, 4, CV_8UC1, cv::Scalar(0));
  input.at<uint8_t>(1, 1) = 64;
  input.at<uint8_t>(1, 3) = 16;

  cv::Mat output = RadialFilter::average(input, 8);

  // std::cout << "\nRadialFilter::average input: \n" << input << "\n\n" << std::endl;
  // std::cout << "\nRadialFilter::average output: \n" << output << "\n\n" << std::endl;

  uint8_t expected[4][4] = {
   {  64,  64,  40,  16},
   {  64,  59,  40,  22},
   {  64,  64,  48,  16},
   {   0,  40,   0,  48}
  };

  for (int y = 0; y < 4; y++)
  {
    for (int x = 0; x < 4; x++)
    {
      ASSERT_EQ(output.at<uint8_t>(y, x), expected[y][x]);
    }
  }
}

TEST(RadialFilter, connect) {
  cv::Mat input(4, 4, CV_8UC1, cv::Scalar(0));
  input.at<uint8_t>(1, 1) = 10;
  input.at<uint8_t>(2, 3) = 20;

  cv::Mat output = RadialFilter::connect(input, 64);

  // std::cout << "\nRadialFilter::connect input: \n" << input << "\n\n" << std::endl;
  // std::cout << "\nRadialFilter::connect output: \n" << output << "\n\n" << std::endl;

  uint8_t expected[4][4] = {
   {   0,   0,   0,   0},
   {   0,   1,   1,   0},
   {   0,   0,   1,   1},
   {   0,   0,   0,   0}
  };

  for (int y = 0; y < 4; y++)
  {
    for (int x = 0; x < 4; x++)
    {
      if (expected[y][x] > 0)
      {
        ASSERT_GT(output.at<uint8_t>(y, x), 0);
      }
      else
      {
        ASSERT_EQ(output.at<uint8_t>(y, x), 0);
      }
    }
  }
}

}
