#include "task_grayscale.hh"
#include <opencv2/imgproc/imgproc.hpp>

using namespace focusstack;

Task_Grayscale::Task_Grayscale(std::shared_ptr<ImgTask> input, std::shared_ptr<Task_Grayscale> reference)
{
  m_filename = input->filename();
  m_name = "Grayscale " + m_filename;

  m_input = input;
  m_reference = reference;

  m_depends_on.push_back(input);

  if (reference)
  {
    m_depends_on.push_back(reference);
  }
}

void Task_Grayscale::task()
{
  cv::cvtColor(m_input->img(), m_result, cv::COLOR_BGR2GRAY);

  m_input.reset();
  m_reference.reset();
}
