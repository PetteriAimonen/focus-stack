#include "histogrampercentile.hh"
#include <opencv2/imgproc.hpp>

using namespace focusstack;

HistogramPercentile::HistogramPercentile(const cv::Mat &image, int histogram_size)
{
  // Number of histogram points is divided uniformly between minimum and maximum value.
  double min, max;
  cv::minMaxIdx(image, &min, &max);
  m_minimum = min;
  m_maximum = max;

  cv::Mat src[1] = {image};
  int histChannels[1] = {0};
  int histSize[1] = {histogram_size};
  float histRange[2] = {m_minimum, m_maximum};
  const float *histRanges[1] = {histRange};
  cv::calcHist(src, 1, histChannels, cv::Mat(), m_histogram, 1, histSize, histRanges, true);

  m_total_pixels = image.rows * image.cols;
}

HistogramPercentile::HistogramPercentile(const cv::Mat &image, const cv::Mat &mask, int histogram_size)
{
  // Number of histogram points is divided uniformly between minimum and maximum value.
  double min, max;
  cv::minMaxIdx(image, &min, &max, nullptr, nullptr, mask);
  m_minimum = min;
  m_maximum = max;

  cv::Mat src[1] = {image};
  int histChannels[1] = {0};
  int histSize[1] = {histogram_size};
  float histRange[2] = {m_minimum, m_maximum};
  const float *histRanges[1] = {histRange};
  cv::calcHist(src, 1, histChannels, mask, m_histogram, 1, histSize, histRanges, true);

  m_total_pixels = cv::countNonZero(mask);
}

float HistogramPercentile::percentile(float ratio) const
{
  double sum = 0.0f;
  double threshold = m_total_pixels * ratio;
  for (int i = 0; i < m_histogram.rows; i++)
  {
    float sum1 = sum;
    sum += m_histogram.at<float>(i, 0);
    if (sum >= threshold)
    {
      float interpolated = i - (sum - threshold) / (sum - sum1);
      return m_minimum + (m_maximum - m_minimum) * interpolated / m_histogram.rows;
    }
  }

  return m_maximum;
}