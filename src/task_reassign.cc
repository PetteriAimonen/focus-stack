#include "task_reassign.hh"
#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs.hpp>

using namespace focusstack;

Task_Reassign::Task_Reassign(const std::vector<std::shared_ptr<ImgTask> > &grayscale_imgs,
                             const std::vector<std::shared_ptr<ImgTask> > &color_imgs,
                             std::shared_ptr<ImgTask> merged):
  m_grayscale_imgs(grayscale_imgs), m_color_imgs(color_imgs), m_merged(merged)
{
  m_filename = m_merged->filename();
  m_name = "Reassign pixel values";

  m_depends_on.insert(m_depends_on.begin(), grayscale_imgs.begin(), grayscale_imgs.end());
  m_depends_on.insert(m_depends_on.begin(), color_imgs.begin(), color_imgs.end());
  m_depends_on.push_back(merged);
}

void Task_Reassign::task()
{
  // Initialize using first image
  cv::Mat best_distance;
  cv::absdiff(m_merged->img(), m_grayscale_imgs.at(0)->img(), best_distance);
  m_result = m_color_imgs.at(0)->img();

  // Search for better matches
  for (int i = 1; i < m_grayscale_imgs.size(); i++)
  {
    cv::Mat distance;
    cv::absdiff(m_merged->img(), m_grayscale_imgs.at(i)->img(), distance);
    cv::Mat mask = (distance < best_distance);
    distance.copyTo(best_distance, mask);
    m_color_imgs.at(i)->img().copyTo(m_result, mask);
  }

  m_grayscale_imgs.clear();
  m_color_imgs.clear();
  m_merged.reset();
}
