#include "task_depthmap.hh"
#include "task_wavelet.hh"
#include "task_wavelet_templates.hh"
#include "task_merge.hh"
#include "histogrampercentile.hh"
#include <opencv2/imgcodecs.hpp>
#include <stdio.h>

using namespace focusstack;

Task_Depthmap::Task_Depthmap(std::shared_ptr<ImgTask> input,
                int depth, bool last,
                std::shared_ptr<Task_Depthmap> previous,
                bool save_steps):
  m_input(input), m_depth(depth), m_previous(previous)
{
  m_filename = "depthmap.png";
  m_name = "Construct depthmap layer " + std::to_string(depth);

  m_last = last;
  m_save_steps = save_steps;
  m_maxdepth = m_depth;

  if (m_input)
    m_depends_on.push_back(m_input);

  if (m_previous)
    m_depends_on.push_back(m_previous);

  if (!m_input && !m_previous)
  {
    throw new std::logic_error("Task_Depthmap: Either input or previous layer is required!");
  }
}

void Task_Depthmap::task()
{
  // Continue from previous layer or start afresh?
  if (m_previous)
  {
    m_valid_area = m_previous->m_valid_area;
    m_maxdepth = std::max(m_depth, m_previous->m_maxdepth);
    m_noiselevel = m_previous->m_noiselevel;
    m_guo = m_previous->m_guo;
  }
  else
  {
    assert(m_input);
    cv::Mat input = m_input->img();
    m_valid_area = m_input->valid_area();
    m_noiselevel = 10.0f; // estimate_noise_level(input);
    m_guo.create(input.rows, input.cols, CV_32FC(8));
    m_guo = 0;
  }
  m_previous.reset();

  // Process input image from Task_FocusMeasure
  if (m_input)
  {
    cv::Mat input = m_input->img().clone();
    limit_valid_area(m_input->valid_area());
    assert(m_guo.size() == input.size());

    cv::Mat y_nobias = input - m_noiselevel;
    y_nobias.setTo(1, y_nobias < 1);

    add_to_guo(y_nobias, m_depth);

    m_input.reset();
  }

  // Convert the collected sums into a Gaussian fit.
  if (m_last)
  {
    compute_result();
    m_guo.release();
  }
}

float Task_Depthmap::estimate_noise_level(const cv::Mat &data)
{
  HistogramPercentile hist(data, 1024);
  float noisefloor = hist.percentile(0.1f);
  m_logger->verbose("Estimated focus measure noise level: %0.3f\n", noisefloor);
  return noisefloor;
}

void Task_Depthmap::add_to_guo(const cv::Mat &y_values, float x)
{
  cv::Mat y_log;
  cv::log(y_values, y_log);

  // Refer to "A Simple Algorithm for Fitting a Gaussian Function" by Hongwei Guo:
  // https://www.researchgate.net/publication/252062037_A_Simple_Algorithm_for_Fitting_a_Gaussian_Function_DSP_Tips_and_Tricks
  for (int yi = 0; yi < m_guo.rows; yi++)
  {
    for (int xi = 0; xi < m_guo.cols; xi++)
    {
      float y = y_values.at<float>(yi, xi);
      float y2 = y * y;
      float lny = y_log.at<float>(yi, xi);
      cv::Vec<float, 8> &guo = m_guo.at<cv::Vec<float, 8> >(yi, xi);

      guo[0] += y2;
      guo[1] += x * y2;
      guo[2] += x * x * y2;
      guo[3] += x * x * x * y2;
      guo[4] += x * x * x * x * y2;
      guo[5] += y2 * lny;
      guo[6] += (x * y2) * lny;
      guo[7] += (x * x * y2) * lny;
    }
  }
}

cv::Mat Task_Depthmap::mask(int halo_radius) const
{
  // Start with Gaussian amplitude subtracted by noiselevel.
  cv::Mat mask;
  m_gauss_amp.convertTo(mask, CV_8UC1, 1.0, -m_noiselevel);

  // Mask out points with high deviation
  mask.setTo(0, m_gauss_dev > 128);

  // Apply halo removal by reducing amplitude near high amplitude edges
  if (halo_radius > 0)
  {
    cv::Mat dilated;
    int ksize = halo_radius * 2 + 1;
    cv::dilate(mask, dilated, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(ksize, ksize)));
    mask -= dilated * 0.5f;
  }

  return mask;
}

void Task_Depthmap::compute_result()
{
  // For each pixel we solve equation of form A * C = B
  cv::Mat A(3, 3, CV_32FC1);
  cv::Mat B(3, 1, CV_32FC1);
  cv::Mat C(3, 1, CV_32FC1);

  // Scale results to 1-255, level 0 is left for unknown depth.
  float scaler, offset;
  if (m_maxdepth < 254)
  {
    scaler = 255.0f / (m_maxdepth + 1);
    offset = scaler;
  }
  else
  {
    scaler = 254.0f / m_maxdepth;
    offset = 1;
  }

  m_gauss_mean.create(m_guo.rows, m_guo.cols, CV_32FC1);
  m_gauss_dev.create(m_guo.rows, m_guo.cols, CV_32FC1);
  m_gauss_amp.create(m_guo.rows, m_guo.cols, CV_32FC1);

  for (int yi = 0; yi < m_guo.rows; yi++)
  {
    for (int xi = 0; xi < m_guo.cols; xi++)
    {
      cv::Vec<float, 8> &guo = m_guo.at<cv::Vec<float, 8> >(yi, xi);
      A.at<float>(0, 0) = guo[0];
      A.at<float>(0, 1) = A.at<float>(1, 0) = guo[1];
      A.at<float>(0, 2) = A.at<float>(1, 1) = A.at<float>(2, 0) = guo[2];
      A.at<float>(2, 1) = A.at<float>(1, 2) = guo[3];
      A.at<float>(2, 2) = guo[4];
      B.at<float>(0, 0) = guo[5];
      B.at<float>(1, 0) = guo[6];
      B.at<float>(2, 0) = guo[7];

      cv::solve(A, B, C, cv::DECOMP_QR);

      // Compute gaussian parameters
      // Equations (5) to (7)
      float a = C.at<float>(0, 0);
      float b = C.at<float>(1, 0);
      float c = C.at<float>(2, 0);
      float mean = -b / (2 * c);
      float dev = sqrtf(-1 / (2 * c));
      float amp = expf(a - (b * b) / (4 * c));

      // c should always be negative for valid gaussians.
      if (c < -0.00001f && mean >= 0 && mean <= m_maxdepth)
      {
        m_gauss_mean.at<float>(yi, xi) = mean * scaler + offset;
        m_gauss_dev.at<float>(yi, xi) = dev * scaler;
        m_gauss_amp.at<float>(yi, xi) = amp;
      }
      else
      {
        m_gauss_mean.at<float>(yi, xi) = 0;
        m_gauss_dev.at<float>(yi, xi) = 255;
        m_gauss_amp.at<float>(yi, xi) = 0;
      }
    }
  }

  m_gauss_mean.convertTo(m_result, CV_8UC1);
  
  if (m_save_steps)
  {
    cv::Mat tmp;
    cv::imwrite("gauss_mean.png", m_result);

    m_gauss_dev.convertTo(tmp, CV_8UC1);
    cv::imwrite("gauss_dev.png", tmp);

    m_gauss_amp.convertTo(tmp, CV_8UC1);
    cv::imwrite("gauss_amp.png", tmp);
  }
}
