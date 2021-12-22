#include "task_saveimg.hh"
#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs.hpp>

using namespace focusstack;

Task_SaveImg::Task_SaveImg(std::string filename, std::shared_ptr<ImgTask> input, int jpgquality, bool nocrop)
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
  m_nocrop= nocrop;
  m_input = input;
  m_depends_on.push_back(input);
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

  if (!m_nocrop)
  {
    cv::Rect croparea = m_input->valid_area();
    if (!croparea.empty() && croparea.size() != m_result.size())
    {
      m_logger->verbose("%s cropping from (%d, %d) to (%d, %d)\n",
        m_filename.c_str(), m_result.cols, m_result.rows, croparea.width, croparea.height);

      // Crop to original size
      cv::Mat tmp(croparea.size(), m_result.type());
      m_result(croparea).copyTo(tmp);
      m_result = tmp;
    }
  }

  // Input image can be released now
  m_input.reset();

  if (m_filename != "" && m_filename != ":memory:")
  {
    std::vector<int> compression_params;
    compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
    compression_params.push_back(m_jpgquality);
    cv::imwrite(m_filename, m_result, compression_params);
  }
}


