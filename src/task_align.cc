#include "task_align.hh"

using namespace focusstack;

Task_Align::Task_Align(std::shared_ptr<ImgTask> reference, std::shared_ptr<ImgTask> grayscale, std::shared_ptr<ImgTask> color)
{
  // Separate filename from path and add "aligned_" in front.
  std::string basename = color->filename();
  size_t pos = basename.find_last_of("/\\");
  if (pos != std::string::npos)
  {
    basename = basename.substr(pos + 1);
  }

  m_filename = "aligned_" + basename;
  m_name = "Align " + basename;

  m_reference = reference;
  m_grayscale = grayscale;
  m_color = color;

  m_depends_on.push_back(reference);
  m_depends_on.push_back(grayscale);
  m_depends_on.push_back(color);
}

void Task_Align::task()
{
  m_result = m_color->img();

  m_reference.reset();
  m_grayscale.reset();
  m_color.reset();
}

