#include "task_loadimg.hh"
#include "task_wavelet.hh"
#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs.hpp>

using namespace focusstack;

Task_LoadImg::Task_LoadImg(std::string filename)
{
  m_filename = filename;
  m_name = "Load " + filename;
}

Task_LoadImg::Task_LoadImg(std::string name, const cv::Mat &img)
{
  m_filename = name;
  m_name = "Memory image " + name;
  m_result = img.clone();
}

void Task_LoadImg::task()
{
  if (!m_result.data)
  {
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
