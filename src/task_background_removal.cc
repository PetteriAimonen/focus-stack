#include "task_background_removal.hh"
#include "radialfilter.hh"
#include <opencv2/imgproc.hpp>

using namespace focusstack;

Task_BackgroundRemoval::Task_BackgroundRemoval(std::shared_ptr<ImgTask> input, int threshold, int gapsize):
  m_input(input), m_threshold(threshold), m_gapsize(gapsize)
{
  m_filename = "segmented.png";
  m_name = "Perform background segmentation";
  m_depends_on.push_back(input);
}

void Task_BackgroundRemoval::task()
{
  cv::Mat input = m_input->img();
  m_valid_area = m_input->valid_area();
  cv::Mat mask(input.rows, input.cols, CV_8UC1, cv::Scalar(0));

  if (m_threshold < 0)
  {
    // White background
    mask.setTo(255, input < -m_threshold);
  }
  else
  {
    // Black background
    mask.setTo(255, input > m_threshold);
  }

  m_input.reset();

  // Remove small noise
  cv::erode(mask, mask, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5)));
  cv::dilate(mask, mask, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5)));

  // Connect points
  cv::Mat connected = RadialFilter::connect(mask, m_gapsize);
  connected.setTo(255, mask);

  cv::threshold(connected, connected, 4, 255, cv::THRESH_BINARY);

  cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
  cv::dilate(connected, connected, kernel);
  cv::erode(connected, connected, kernel);
  cv::erode(connected, connected, kernel);
  cv::dilate(connected, connected, kernel);

  m_result = connected;
}
