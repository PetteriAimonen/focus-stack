#include "task_depthmap_inpaint.hh"
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include "fast_bilateral.hh"
#include "radialfilter.hh"

using namespace focusstack;

Task_Depthmap_Inpaint::Task_Depthmap_Inpaint(std::shared_ptr<Task_Depthmap> depthmap, int threshold, int smooth_xy, int smooth_z, bool save_steps):
  m_depthmap(depthmap), m_threshold(threshold),
  m_smooth_xy(smooth_xy), m_smooth_z(smooth_z),
  m_save_steps(save_steps)
{
  m_filename = "filtered_depthmap.png";
  m_name = "Inpaint depthmap";

  m_depends_on.push_back(m_depthmap);
}

void Task_Depthmap_Inpaint::task()
{
  cv::Mat depth = m_depthmap->depthmap();
  cv::Mat mask = m_depthmap->mask(20);

  // Make editable copy and set to zero outside the valid area
  m_valid_area = m_depthmap->valid_area();
  cv::Mat tmp(depth.size(), depth.type(), cv::Scalar(0));
  depth(m_valid_area).copyTo(tmp(m_valid_area));
  depth = tmp;

  m_depthmap.reset();

  // Scale depthmap values to range 0 - 255, reserving the lowest level as "unknown".
  double maxdepth;
  cv::minMaxIdx(depth, nullptr, &maxdepth);
  float scaler, offset;
  if (maxdepth < 254)
  {
    scaler = 255.0f / (maxdepth + 1);
    offset = scaler;
  }
  else
  {
    scaler = 254.0f / maxdepth;
    offset = 1;
  }
  depth.convertTo(depth, CV_8UC1, scaler, offset);

  // cv::imwrite("depth_mask.png", mask);

  depth.setTo(0, mask < m_threshold);

  if (m_save_steps)
  {
    cv::imwrite("depth_before_inpaint.png", depth);
  }

  // Fill in any zero areas by averaging from closest points
  depth = RadialFilter::average(depth);

  // Some final averaging to smooth the result and remove outliers
  int medsize = 2 * (m_smooth_xy / 8) + 3;
  cv::medianBlur(depth, depth, medsize);
  cv_extend::bilateralFilter(depth, m_result, m_smooth_z, m_smooth_xy);
  cv::medianBlur(m_result, m_result, medsize);
}
