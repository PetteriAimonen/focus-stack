#include "task_saveimg.hh"
#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs.hpp>

using namespace focusstack;

Task_SaveImg::Task_SaveImg(std::string filename, std::shared_ptr<ImgTask> input, int jpgquality, std::shared_ptr<Task_LoadImg> origsize)
{
  m_filename = filename;

  if (filename != "" && filename != ":memory:")
  {
    m_name = "Save " + filename;
  }
  else
  {
    m_name = "Final crop " + input->filename();
  }

  m_jpgquality = jpgquality;
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
  m_result = m_input->img();

  if (m_result.channels() == 2)
  {
    // Convert complex wavelets to RGB image
    cv::Mat channels[3];
    cv::split(m_result, channels);
    channels[0].convertTo(channels[0], CV_8U);
    channels[1].convertTo(channels[1], CV_8U);
    channels[2].create(m_result.rows, m_result.cols, CV_8U);
    channels[2] = 0;
    cv::merge(channels, 3, m_result);
  }

  if (m_origsize)
  {
    // Crop to original size
    cv::Size orig_size = m_origsize->orig_size();
    cv::Mat tmp(orig_size, m_result.type());

    int crop_x = m_result.cols - orig_size.width;
    int crop_y = m_result.rows - orig_size.height;

    m_result(cv::Rect(crop_x / 2, crop_y / 2, orig_size.width, orig_size.height)).copyTo(tmp);
    m_result = tmp;
  }

  // Input images can be released now
  m_input.reset();
  m_origsize.reset();

  if (m_filename != "" && m_filename != ":memory:")
  {
    std::vector<int> compression_params;
    compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
    compression_params.push_back(m_jpgquality);
    cv::imwrite(m_filename, m_result, compression_params);
  }
}


