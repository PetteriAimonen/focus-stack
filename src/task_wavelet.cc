#include "task_wavelet.hh"

using namespace focusstack;

Task_Wavelet::Task_Wavelet(std::shared_ptr<ImgTask> input, bool inverse)
{
  m_input = input;
  m_inverse = inverse;

  m_filename = input->filename();

  if (inverse)
    m_name = "Inverse-wavelet " + m_filename;
  else
    m_name = "Forward-wavelet " + m_filename;

  m_depends_on.push_back(input);
}

void Task_Wavelet::task()
{
  m_result = m_input->img();
  m_input.reset();
}


