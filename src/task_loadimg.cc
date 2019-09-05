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

void Task_LoadImg::task()
{
  m_result = cv::imread(m_filename, cv::IMREAD_COLOR);

  if (!m_result.data)
  {
    throw std::runtime_error("Could not load " + m_filename);
  }

  m_orig_size = m_result.size();

  // Expand image width & height to multiple of 8 as required by wavelet decomposition
  int factor = (1 << Task_Wavelet::levels);
  if (m_result.cols % factor != 0 || m_result.rows % factor != 0)
  {
    int expand_x = factor - m_result.cols % factor;
    int expand_y = factor - m_result.rows % factor;
    int width = m_result.cols + expand_x;
    int height = m_result.rows + expand_y;
    cv::Mat tmp(height, width, m_result.type());

    cv::copyMakeBorder(m_result, tmp,
                       expand_y / 2, expand_y - expand_y / 2,
                       expand_x / 2, expand_x - expand_x / 2,
                       cv::BORDER_REFLECT);

    m_result = tmp;
  }
}
