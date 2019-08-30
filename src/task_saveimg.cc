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
  cv::imwrite(m_filename, m_input->img());
  m_input.reset();
}


