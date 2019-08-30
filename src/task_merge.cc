#include "task_merge.hh"

using namespace focusstack;

Task_Merge::Task_Merge(const std::vector<std::shared_ptr<ImgTask> >& images):
  m_images(images)
{
  m_filename = "merge_result.jpg";
  m_name = "Merge " + std::to_string(m_images.size()) + " images";

  m_depends_on.insert(m_depends_on.begin(), images.begin(), images.end());
}

void Task_Merge::task()
{
  m_result = m_images.at(0)->img();

  m_images.clear();
}

