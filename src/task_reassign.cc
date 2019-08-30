#include "task_reassign.hh"
#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs.hpp>

using namespace focusstack;

Task_Reassign::Task_Reassign(const std::vector<std::shared_ptr<ImgTask> >& images, std::shared_ptr<ImgTask> merged):
  m_images(images), m_merged(merged)
{
  m_filename = m_merged->filename();
  m_name = "Reassign pixel values";

  m_depends_on.insert(m_depends_on.begin(), images.begin(), images.end());
  m_depends_on.push_back(merged);
}

void Task_Reassign::task()
{
  m_merged->img().convertTo(m_result, CV_8U);

  m_images.clear();
  m_merged.reset();
}
