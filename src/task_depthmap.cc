#include "task_depthmap.hh"
#include "task_wavelet.hh"
#include "task_wavelet_templates.hh"
#include "task_merge.hh"

using namespace focusstack;

Task_Depthmap::Task_Depthmap(std::shared_ptr<Task_Merge> merged_wavelet, float smoothing, int max_depth)
{
  m_name = "Construct depthmap";
  m_merged_wavelet = merged_wavelet;
  m_smoothing = smoothing;
  m_max_depth = max_depth;

  m_depends_on.push_back(merged_wavelet);
}

void Task_Depthmap::task()
{
  // The depthmap from Task_Merge is in wavelet format.
  // The absolute value of the merged wavelet image at each level tells the contrast.
  // Take the depth reading from the level with the best contrast information.
  cv::Mat depthmap = m_merged_wavelet->depthmap();
  cv::Mat merged = m_merged_wavelet->img();
  
  // Precalculate squared absolute values of the wavelets.
  cv::Mat absval(depthmap.rows, depthmap.cols, CV_32F);
  Task_Merge::get_sq_absval(merged, absval);

  cv::Mat result_depth;
  cv::Mat result_absval;

  bool first = true;
  for (int level = Task_Wavelet::levels - 1; level >= 0; level--)
  {
    // Calculate size of the wavelet subband regions
    int w = depthmap.cols >> level;
    int h = depthmap.rows >> level;
    int w2 = w / 2;
    int h2 = h / 2;

    // First take the diagonal subband and scale absolute values by 4.
    // This is because every lowpass step doubles the strength, and the
    // absolute values are squared.
    cv::Mat max_absval = absval(cv::Rect(w2, h2, w2, h2)) * 4.0f;
    cv::Mat combined = depthmap(cv::Rect(w2, h2, w2, h2)).clone();

    // Then find the subband with the largest absolute value at each pixel.
    cv::Rect subbands[2] = { {w2, 0, w2, h2}, {0, h2, w2, h2} };
    for (cv::Rect subband: subbands)
    {
      cv::Mat subband_absval = absval(subband);
      cv::Mat subband_depthmap = depthmap(subband);

      cv::Mat mask = (subband_absval > max_absval);
      subband_absval.copyTo(max_absval, mask);
      subband_depthmap.copyTo(combined, mask);
    }

    // Combine depth information from other levels
    if (first)
    {
      combined.convertTo(result_depth, CV_32F);
      result_absval = max_absval;
    }
    else
    {
      // There are two thresholds: per-pixel and global.
      // The larger the threshold, the lower weight will each update get.
      float threshold = m_smoothing * cv::mean(result_absval).val[0];

      for (int y = 0; y < h2; y++)
      {
        for (int x = 0; x < w2; x++)
        {
          float weight = max_absval.at<float>(y, x) / std::min(threshold, result_absval.at<float>(y, x));
          if (weight > 1.0f)
          {
            result_depth.at<float>(y, x) = combined.at<uint16_t>(y, x);
            result_absval.at<float>(y, x) = max_absval.at<float>(y, x);
          }
          else if (weight > 0.25f)
          {
            result_depth.at<float>(y, x) = combined.at<uint16_t>(y, x) * weight
                                         + result_depth.at<float>(y, x) * (1 - weight);
            result_absval.at<float>(y, x) = max_absval.at<float>(y, x) * weight
                                         + result_absval.at<float>(y, x) * (1 - weight);
          }
          else
          {
            // Ignore small noise values
          }
        }
      }
    }

    // Upscale by 2x for the next level.
    cv::resize(result_depth, result_depth, cv::Size(w, h), 0, 0, cv::INTER_LINEAR);
    cv::resize(result_absval, result_absval, cv::Size(w, h), 0, 0, cv::INTER_LINEAR);

    first = false;
  }

  result_depth.convertTo(m_result, CV_8U, 256.0 / m_max_depth);
  
  m_merged_wavelet.reset();
}
