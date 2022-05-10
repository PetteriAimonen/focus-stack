#include "task_loadimg.hh"
#include "task_wavelet.hh"
#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <fstream>

using namespace focusstack;

Task_LoadImg::Task_LoadImg(std::string filename, float wait_images)
{
  m_filename = filename;
  m_name = "Load " + filename;
  m_wait_images = wait_images;
  m_wait_images_until = std::chrono::system_clock::now()
                      + std::chrono::milliseconds((int)(m_wait_images * 1000));
}

Task_LoadImg::Task_LoadImg(std::string name, const cv::Mat &img)
{
  m_filename = name;
  m_name = "Memory image " + name;
  m_result = img.clone();
  m_wait_images = 0;
  m_wait_images_until = std::chrono::system_clock::now()
                      + std::chrono::milliseconds((int)(m_wait_images * 1000));
}

bool Task_LoadImg::ready_to_run()
{
  if (!ImgTask::ready_to_run())
  {
    return false;
  }

  // Wait for image files to appear.
  // This is useful for processing images as soon as they appear.
  if (m_wait_images > 0 && std::chrono::system_clock::now() < m_wait_images_until)
  {
    std::ifstream f(m_filename.c_str());
    if (!f.good())
    {
      return false;
    }
  }

  return true;
}

void Task_LoadImg::task()
{
  if (!m_result.data)
  {
    m_result = cv::imread(m_filename, cv::IMREAD_ANYCOLOR);
  }

  while (!m_result.data && std::chrono::system_clock::now() < m_wait_images_until)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    m_result = cv::imread(m_filename, cv::IMREAD_ANYCOLOR);
  }

  if (!m_result.data)
  {
    throw std::runtime_error("Could not load " + m_filename);
  }

  m_orig_size = m_result.size();
  m_valid_area = cv::Rect(0, 0, m_result.cols, m_result.rows);

  // Expand image width & height to multiple of (1 << levels) as required by wavelet decomposition
  cv::Size expanded;
  int levels = Task_Wavelet::levels_for_size(m_orig_size, &expanded);
  std::string name = basename();
  m_logger->verbose("%s has resolution %dx%d, using %d wavelet levels and expanding to %dx%d\n",
                    name.c_str(), m_orig_size.width, m_orig_size.height, levels,
                    expanded.width, expanded.height);

  if (expanded != m_orig_size)
  {
    int expand_x = expanded.width - m_orig_size.width;
    int expand_y = expanded.height - m_orig_size.height;
    cv::Mat tmp(expanded.height, expanded.width, m_result.type());

    cv::copyMakeBorder(m_result, tmp,
                       expand_y / 2, expand_y - expand_y / 2,
                       expand_x / 2, expand_x - expand_x / 2,
                       cv::BORDER_REFLECT);

    m_result = tmp;
    m_valid_area = cv::Rect(cv::Point(expand_x / 2, expand_y / 2), m_orig_size);
  }
}
