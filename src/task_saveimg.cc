#include "task_saveimg.hh"
#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs.hpp>

using namespace focusstack;

Task_SaveImg::Task_SaveImg(std::string filename, std::shared_ptr<ImgTask> input, std::shared_ptr<Task_LoadImg> origsize)
{
  m_filename = filename;
  m_name = "Save " + filename;

  m_input = input;
  m_depends_on.push_back(input);

  m_origsize = origsize;
  if (m_origsize)
  {
    m_depends_on.push_back(m_origsize);
  }
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

  if (m_origsize)
  {
    // Crop to original size
    cv::Size orig_size = m_origsize->orig_size();
    cv::Mat tmp(orig_size, img.type());

    int crop_x = img.cols - orig_size.width;
    int crop_y = img.rows - orig_size.height;

    img(cv::Rect(crop_x / 2, crop_y / 2, orig_size.width, orig_size.height)).copyTo(tmp);
    img = tmp;
  }

  cv::imwrite(m_filename, img);
  m_input.reset();
  m_origsize.reset();
}


