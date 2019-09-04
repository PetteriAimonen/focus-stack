#include "task_saveimg.hh"
#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs.hpp>

using namespace focusstack;

Task_SaveImg::Task_SaveImg(std::string filename, std::shared_ptr<ImgTask> input)
{
  m_filename = filename;
  m_name = "Save " + filename;

  m_input = input;
  m_depends_on.push_back(input);
}

void Task_SaveImg::task()
{
  cv::Mat img = m_input->img();

  if (img.channels() == 2)
  {
    // Convert complex wavelets to RGB image
    cv::Mat channels[3];
    cv::split(img, channels);
    channels[0].convertTo(channels[0], CV_8U);
    channels[1].convertTo(channels[1], CV_8U);
    channels[2].create(img.rows, img.cols, CV_8U);
    channels[2] = 0;
    cv::merge(channels, 3, img);
  }

  cv::imwrite(m_filename, img);
  m_input.reset();
}


