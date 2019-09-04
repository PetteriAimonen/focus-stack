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
  m_result = m_images.front()->img();
  return;

  int rows = m_images.front()->img().rows;
  int cols = m_images.front()->img().cols;

  cv::Mat max_absval(rows, cols, CV_32F);
  max_absval = 0.0f;

  m_result.create(rows, cols, CV_32FC2);

  // For each pixel in the wavelet image, select the wavelet with highest
  // absolute value.
  for (int i = 0; i < m_images.size(); i++)
  {
    const cv::Mat &wavelet = m_images.at(i)->img();
    cv::Mat absval(rows, cols, CV_32F);
    get_sq_absval(wavelet, absval);

    cv::Mat mask = (absval > max_absval);
    absval.copyTo(max_absval, mask);
    wavelet.copyTo(m_result, mask);
  }

  m_images.clear();
}

void focusstack::Task_Merge::get_sq_absval(const cv::Mat& complex_mat, cv::Mat& absval)
{
  for (int y = 0; y < complex_mat.rows; y++)
  {
    for (int x = 0; x < complex_mat.cols; x++)
    {
      cv::Vec2f v = complex_mat.at<cv::Vec2f>(y, x);
      absval.at<float>(y, x) = v[0] * v[0] + v[1] * v[1];
    }
  }
}

