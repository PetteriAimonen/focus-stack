#include "task_denoise.hh"
#include "task_wavelet.hh"

using namespace focusstack;

Task_Denoise::Task_Denoise(std::shared_ptr<ImgTask> input, float level)
{
  m_filename = "denoised_" + input->basename();
  m_name = "Denoise " + input->basename();

  m_input = input;
  m_depends_on.push_back(input);
  m_level = level;
}

static inline float threshold_filter(float x, float level)
{
  if (x < -level)
  {
    return x + level;
  }
  else if (x > level)
  {
    return x - level;
  }
  else
  {
    return 0.0f;
  }
}

void Task_Denoise::task()
{
  cv::Mat src = m_input->img();
  m_result.create(src.rows, src.cols, CV_32FC2);
  src.copyTo(m_result);

  int lowest_w = src.cols >> Task_Wavelet::levels;
  int lowest_h = src.rows >> Task_Wavelet::levels;

  for (int y = 0; y < src.rows; y++)
  {
    for (int x = 0; x < src.cols; x++)
    {
      if (y < lowest_h && x < lowest_w)
      {
        // Don't filter the downscaled image
        continue;
      }

      cv::Vec2f v = m_result.at<cv::Vec2f>(y, x);

      float absval = v[0] * v[0] + v[1] * v[1];

      if (absval <= m_level)
      {
        // Cut off all wavelets below the threshold
        v[0] = 0.0f;
        v[1] = 0.0f;
      }
      else
      {
        // Diminish other wavelets, but don't change the phase.
        float ratio = (absval - m_level) / absval;
        v[0] *= ratio;
        v[1] *= ratio;
      }

      m_result.at<cv::Vec2f>(y, x) = v;
    }
  }

  m_input.reset();
}


