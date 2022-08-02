#include "task_merge.hh"
#include "task_wavelet.hh"

using namespace focusstack;

Task_Merge::Task_Merge(std::shared_ptr<Task_Merge> prev_merge,
                       const std::vector<std::shared_ptr<ImgTask> > &images,
                       int consistency):
  m_prev_merge(prev_merge), m_images(images), m_consistency(consistency)
{
  m_filename = "merge_result.jpg";
  m_name = "Merge " + std::to_string(m_images.size()) + " images";

  if (prev_merge)
    m_depends_on.push_back(prev_merge);

  m_depends_on.insert(m_depends_on.begin(), images.begin(), images.end());
}

void Task_Merge::task()
{
  int rows = m_images.front()->img().rows;
  int cols = m_images.front()->img().cols;

  cv::Mat max_absval(rows, cols, CV_32F);

  if (m_prev_merge)
  {
    m_result = m_prev_merge->img().clone();
    m_depthmap = m_prev_merge->depthmap();
    get_sq_absval(m_result, max_absval);
  }
  else
  {
    m_result.create(rows, cols, CV_32FC2);
    m_depthmap.create(rows, cols, CV_16U);
    m_depthmap = 0;
    max_absval = -1.0f;
  }

  // Most of the pixel copying is done in loop below using masks, as it is faster.
  // Minor touch-ups are done per-pixel in denoise loops.
  // Keep a map from image index to image pointer for the denoise step.
  m_index_map.clear();
  m_index_map.reserve(m_images.size());

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
    m_depthmap.setTo(m_images.at(i)->index(), mask);

    m_index_map[m_images.at(i)->index()] = m_images.at(i);
  }

  if (m_consistency >= 1)
  {
    denoise_subbands();
  }

  if (m_consistency >= 2)
  {
    denoise_neighbours();
  }

  // Find out the intersection of input image valid areas.
  m_valid_area = m_images.at(0)->valid_area();
  for (int i = 1; i < m_images.size(); i++)
  {
    limit_valid_area(m_images.at(i)->valid_area());
  }

  m_images.clear();
  m_index_map.clear();
  m_prev_merge.reset();
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

cv::Mat Task_Merge::get_source_img(int index)
{
  auto iter = m_index_map.find(index);
  if (iter != m_index_map.end())
    return iter->second->img();
  else
    return m_prev_merge->img();
}

// Compare the horizontal / vertical / diagonal subbands at each level
// and perform two-out-of-three voting filter.
void Task_Merge::denoise_subbands()
{
  int levels = Task_Wavelet::levels_for_size(m_result.size());
  for (int level = 0; level < levels; level++)
  {
    int w = m_result.cols >> level;
    int h = m_result.rows >> level;
    int w2 = w / 2;
    int h2 = h / 2;

    cv::Mat sub1 = m_depthmap(cv::Rect(w2, 0, w2, h2));
    cv::Mat sub2 = m_depthmap(cv::Rect(w2, h2, w2, h2));
    cv::Mat sub3 = m_depthmap(cv::Rect(0, h2, w2, h2));

    for (int y = 0; y < h2; y++)
    {
      for (int x = 0; x < w2; x++)
      {
        uint16_t v1 = sub1.at<uint16_t>(y, x);
        uint16_t v2 = sub2.at<uint16_t>(y, x);
        uint16_t v3 = sub3.at<uint16_t>(y, x);

        // If two out of three subbands match, update the third one to match also.
        if (v1 == v2 && v2 == v3)
        {
          // Nothing to do
        }
        else if (v2 == v3)
        {
          // Update sub1
          sub1.at<uint16_t>(y, x) = v2;
          m_result.at<cv::Vec2f>(y, w2 + x) = get_source_img(v2).at<cv::Vec2f>(y, w2 + x);
        }
        else if (v1 == v3)
        {
          // Update sub2
          sub2.at<uint16_t>(y, x) = v1;
          m_result.at<cv::Vec2f>(h2 + y, w2 + x) = get_source_img(v1).at<cv::Vec2f>(h2 + y, w2 + x);
        }
        else if (v1 == v2)
        {
          // Update sub3
          sub3.at<uint16_t>(y, x) = v1;
          m_result.at<cv::Vec2f>(h2 + y, x) = get_source_img(v1).at<cv::Vec2f>(h2 + y, x);
        }
      }
    }
  }
}

// Compare the four neighbours of each pixel and if they all
// are above/below, eliminate the center outlier.
void Task_Merge::denoise_neighbours()
{
  for (int y = 1; y < m_depthmap.rows - 1; y++)
  {
    for (int x = 1; x < m_depthmap.cols - 1; x++)
    {
      uint16_t left = m_depthmap.at<uint16_t>(y, x - 1);
      uint16_t right = m_depthmap.at<uint16_t>(y, x + 1);
      uint16_t top = m_depthmap.at<uint16_t>(y - 1, x);
      uint16_t bottom = m_depthmap.at<uint16_t>(y + 1, x);
      uint16_t center = m_depthmap.at<uint16_t>(y, x);

      if ((center > top && center > bottom && center > left && center > right) ||
          (center < top && center < bottom && center < left && center < right))
      {
        // Center pixel is an outlier, average the side pixels to get a better value.
        int avg = (top + bottom + left + right + 2) / 4;
        m_depthmap.at<uint16_t>(y, x) = avg;
        m_result.at<cv::Vec2f>(y, x) = get_source_img(avg).at<cv::Vec2f>(y, x);
      }
    }
  }
}
