#include "task_merge.hh"
#include "task_wavelet.hh"

using namespace focusstack;

Task_Merge::Task_Merge(const std::vector<std::shared_ptr<ImgTask> >& images, int consistency):
  m_images(images), m_consistency(consistency)
{
  m_filename = "merge_result.jpg";
  m_name = "Merge " + std::to_string(m_images.size()) + " images";

  m_depends_on.insert(m_depends_on.begin(), images.begin(), images.end());
}

void Task_Merge::task()
{
  int rows = m_images.front()->img().rows;
  int cols = m_images.front()->img().cols;

  cv::Mat max_absval(rows, cols, CV_32F);
  max_absval = -1.0f;

  cv::Mat depthmap(rows, cols, CV_16U);
  depthmap = 0;

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
    depthmap.setTo(i, mask);
  }

  if (m_consistency >= 1)
  {
    denoise_subbands(depthmap);
  }

  if (m_consistency >= 2)
  {
    denoise_neighbours(depthmap);
  }

  m_images.clear();
}

void Task_Merge::get_sq_absval(const cv::Mat& complex_mat, cv::Mat& absval)
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

void Task_Merge::denoise_subbands(cv::Mat& depthmap)
{
  for (int level = 0; level < Task_Wavelet::levels; level++)
  {
    int w = m_result.cols >> level;
    int h = m_result.rows >> level;
    int w2 = w / 2;
    int h2 = h / 2;

    cv::Mat sub1 = depthmap(cv::Rect(w2, 0, w2, h2));
    cv::Mat sub2 = depthmap(cv::Rect(w2, h2, w2, h2));
    cv::Mat sub3 = depthmap(cv::Rect(0, h2, w2, h2));

    for (int y = 0; y < h2; y++)
    {
      for (int x = 0; x < w2; x++)
      {
        uint16_t v1 = sub1.at<uint16_t>(y, x);
        uint16_t v2 = sub2.at<uint16_t>(y, x);
        uint16_t v3 = sub3.at<uint16_t>(y, x);

        // If two out of three subbands match, update the third one to match also.
        if (v2 == v3 && v1 != v2)
        {
          // Update sub1
          sub1.at<uint16_t>(y, x) = v2;
          m_result.at<cv::Vec2f>(y, w2 + x) = m_images.at(v2)->img().at<cv::Vec2f>(y, w2 + x);
        }
        else if (v1 == v3 && v2 != v1)
        {
          // Update sub2
          sub2.at<uint16_t>(y, x) = v1;
          m_result.at<cv::Vec2f>(h2 + y, w2 + x) = m_images.at(v1)->img().at<cv::Vec2f>(h2 + y, w2 + x);
        }
        else if (v1 == v2 && v3 != v1)
        {
          // Update sub3
          sub3.at<uint16_t>(y, x) = v1;
          m_result.at<cv::Vec2f>(h2 + y, x) = m_images.at(v1)->img().at<cv::Vec2f>(h2 + y, x);
        }
      }
    }
  }
}

void Task_Merge::denoise_neighbours(cv::Mat& depthmap)
{
  for (int y = 1; y < depthmap.rows - 1; y++)
  {
    for (int x = 1; x < depthmap.cols - 1; x++)
    {
      uint16_t left = depthmap.at<uint16_t>(y, x - 1);
      uint16_t right = depthmap.at<uint16_t>(y, x + 1);
      uint16_t top = depthmap.at<uint16_t>(y - 1, x);
      uint16_t bottom = depthmap.at<uint16_t>(y + 1, x);

      if (top == bottom && left == right && left == top)
      {
        depthmap.at<uint16_t>(y, x) = top;
        m_result.at<cv::Vec2f>(y, x) = m_images.at(top)->img().at<cv::Vec2f>(y, x);
      }
    }
  }
}
