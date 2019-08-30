#include "task_loadimg.hh"
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
}
