#include "task_depthmap_inpaint.hh"
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include "fast_bilateral.hh"
#include "radialfilter.hh"

using namespace focusstack;

Task_Depthmap_Inpaint::Task_Depthmap_Inpaint(std::shared_ptr<Task_Depthmap> depthmap,
    int threshold, int smooth_xy, int smooth_z, int halo_radius, bool save_steps):
  m_depthmap(depthmap), m_threshold(threshold),
  m_smooth_xy(smooth_xy), m_smooth_z(smooth_z),
  m_halo_radius(halo_radius),
  m_save_steps(save_steps)
{
  m_filename = "filtered_depthmap.png";
  m_name = "Inpaint depthmap";

  if (m_threshold < 1) m_threshold = 1;

  m_depends_on.push_back(m_depthmap);
}

static void masked_blur(const cv::Mat &input, cv::Mat &output, const cv::Mat &mask, int radius)
{
  cv::Mat weight(input.rows, input.cols, CV_32FC1, cv::Scalar(0));
  weight.setTo(1, mask);

  cv::Mat in_masked(input.rows, input.cols, input.type(), cv::Scalar(0));
  input.copyTo(in_masked, mask);
  in_masked.convertTo(in_masked, CV_32FC1);

  int ksize = radius * 4 + 1;
  cv::GaussianBlur(weight, weight, cv::Size(ksize, ksize), radius, radius);
  cv::GaussianBlur(in_masked, in_masked, cv::Size(ksize, ksize), radius, radius);

  in_masked /= weight;
  in_masked.setTo(0, weight < 1.0f / radius / radius);
  in_masked.convertTo(output, input.type());
}

void Task_Depthmap_Inpaint::task()
{
  int max_depth = m_depthmap->maxdepth();
  cv::Mat depth = m_depthmap->depthmap().clone();
  cv::Mat mask = m_depthmap->mask(m_halo_radius * 2);
  cv::Mat mask_nh = m_depthmap->mask(m_halo_radius / 2);
  m_valid_area = m_depthmap->valid_area();
  m_depthmap.reset();

  // Set mask to zero outside the valid area
  cv::Mat border_mask(mask.size(), mask.type(), cv::Scalar(1));
  border_mask(m_valid_area) = 0;
  mask.setTo(0, border_mask);
  mask_nh.setTo(0, border_mask);

  // Make an initial low resolution depthmap
  cv::Mat depth_lowres;
  const int lowres_blur = 16;
  masked_blur(depth, depth_lowres, mask > m_threshold, lowres_blur);
  cv::resize(depth_lowres, depth_lowres, cv::Size(), 0.25f, 0.25f, cv::INTER_NEAREST);

  if (m_save_steps)
  {
    cv::imwrite("depth_inpaint_mask.png", mask);
    cv::imwrite("depth_inpaint_lr_points.png", depth_lowres);
  }

  depth_lowres = RadialFilter::average(depth_lowres);

  if (m_save_steps)
  {
    cv::imwrite("depth_inpaint_lr_out.png", depth_lowres);
  }

  // Make maximum and minimum limits based on the low resolution depthmap.
  cv::Mat minlimit, maxlimit;
  int ksize = lowres_blur * 4 + 1;
  cv::resize(depth_lowres, minlimit, depth.size());
  cv::dilate(minlimit, maxlimit, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(ksize, ksize)));
  cv::erode(minlimit, minlimit, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(ksize, ksize)));

  const int outlier_limit = 64;
  depth.setTo(0, mask_nh <= m_threshold / 4);
  depth.setTo(0, depth < minlimit - outlier_limit);
  depth.setTo(0, depth > maxlimit + outlier_limit);

  if (m_save_steps)
  {
    cv::imwrite("depth_inpaint_masked.png", depth);
  }

  masked_blur(depth, depth, depth > 0, 2);

  if (m_save_steps)
  {
    cv::imwrite("depth_inpaint_points.png", depth);
  }

  // Fill in small gaps in outlines
  int connect_threshold = 255 / max_depth + 1;
  depth = RadialFilter::connect(depth, lowres_blur * 2, connect_threshold);

  if (m_save_steps)
  {
    cv::imwrite("depth_inpaint_lines.png", depth);
  }

  // Fill in any zero areas by averaging from closest points
  depth = RadialFilter::average(depth);

  if (m_save_steps)
  {
    cv::imwrite("depth_inpaint_filled.png", depth);
  }

  // Some final averaging to smooth the result and remove outliers
  m_result = depth;
  if (m_smooth_xy > 0)
  {
    int medsize = 2 * (m_smooth_xy / 8) + 3;
    cv::medianBlur(m_result, m_result, medsize);

    // Bilateral filter gets very slow if smoothing parameters are too small
    if (m_smooth_xy >= 8 && m_smooth_z > 4)
    {
      cv::Mat tmp;
      cv_extend::bilateralFilter(m_result, tmp, m_smooth_z, m_smooth_xy);
      m_result = tmp;
    }

    cv::medianBlur(m_result, m_result, medsize);
  }
}