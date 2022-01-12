#include "task_depthmap.hh"
#include "task_wavelet.hh"
#include "task_wavelet_templates.hh"
#include "task_merge.hh"
#include "histogrampercentile.hh"
#include <opencv2/imgcodecs.hpp>

using namespace focusstack;

Task_Depthmap::Task_Depthmap(std::shared_ptr<ImgTask> input,
                std::vector<std::shared_ptr<ImgTask> > neighbours,
                int depth,
                std::shared_ptr<Task_Depthmap> previous):
  m_input(input), m_neighbours(neighbours), m_depth(depth), m_previous(previous)
{
  m_filename = "depthmap.png";
  m_name = "Construct depthmap layer " + std::to_string(depth);

  m_depends_on.push_back(m_input);
  m_depends_on.insert(m_depends_on.begin(), m_neighbours.begin(), m_neighbours.end());

  if (m_previous)
    m_depends_on.push_back(m_previous);
}

void Task_Depthmap::task()
{
  cv::Mat input = m_input->img().clone();
  int rows = input.rows;
  int cols = input.cols;
  m_valid_area = m_input->valid_area();

  // Continue from previous layer or start afresh?
  if (m_previous)
  {
    m_depthmap = m_previous->m_depthmap;
    m_largest_delta = m_previous->m_largest_delta;
    m_largest_focusmeasure = m_previous->m_largest_focusmeasure;
    limit_valid_area(m_previous->valid_area());
  }
  else
  {
    m_depthmap.create(rows, cols, CV_16UC1);
    m_largest_delta.create(rows, cols, CV_32FC1);
    m_largest_focusmeasure.create(rows, cols, CV_32FC1);
    m_depthmap = 0;
    m_largest_delta = 0;
    m_largest_focusmeasure = -INFINITY;
  }
  m_previous.reset();

  // Calculate delta between this layer and each of the neighbours.
  // The layer at focus should have a highest difference in focus measure compared to neighbours.
  for (auto neighbour: m_neighbours)
  {
    cv::Mat delta = input - neighbour->img();
    cv::Mat mask = (delta > m_largest_delta);

    m_depthmap.setTo(m_depth, mask);
    delta.copyTo(m_largest_delta, mask);
    input.copyTo(m_largest_focusmeasure, mask);

    limit_valid_area(neighbour->valid_area());
  }

  m_input.reset();
  m_neighbours.clear();
  m_result = m_depthmap;
}

cv::Mat Task_Depthmap::mask(int halo_radius) const
{
  cv::Mat alpha = m_largest_delta.clone();
  if (halo_radius > 0)
  {
    // Subtract a dilated version of the delta map.
    // This helps eliminate halos around sharp brightness transitions.
    cv::Mat dilated;
    int ksize = halo_radius * 2 + 1;
    cv::dilate(alpha, dilated, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(ksize, ksize)));
    alpha -= dilated * 0.5f;
  }

  // Values from Task_Focusmeasure are squared, sqrt() them now to get better dynamic range.
  alpha.setTo(0, alpha < 0);
  cv::sqrt(alpha, alpha);

  // Select scaling levels using histogram and percentile points
  HistogramPercentile hist(alpha, 1024);
  float high = hist.percentile(0.999);
  float low = 0.0;
  
  // Convert to 8-bit range
  cv::Mat result;
  float scale = 255.0f / (high - low);
  alpha.convertTo(result, CV_8UC1, scale, -scale * low);
  return result;
}
