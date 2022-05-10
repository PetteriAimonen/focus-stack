#include "task_grayscale.hh"
#include <opencv2/imgproc/imgproc.hpp>
#include <cstdio>

using namespace focusstack;

Task_Grayscale::Task_Grayscale(std::shared_ptr<ImgTask> input, std::shared_ptr<Task_Grayscale> reference)
{
  m_filename = input->filename();
  m_name = "Grayscale " + m_filename;
  m_index = input->index();

  m_input = input;
  m_reference = reference;

  m_depends_on.push_back(input);

  if (reference)
  {
    m_depends_on.push_back(reference);
  }
}

void Task_Grayscale::task()
{
  cv::Mat img = m_input->img();

  if (img.channels() == 1)
  {
    // Input is already grayscale
    m_result = img;
  }
  else
  {
    if (m_reference)
    {
      m_weights = m_reference->weights();
    }
    else
    {
      do_pca();
    }

    cv::Mat channels[3];
    cv::split(img, channels);
    m_result = channels[0] * m_weights.at<float>(0)
             + channels[1] * m_weights.at<float>(1)
             + channels[2] * m_weights.at<float>(2);
  }

  m_valid_area = m_input->valid_area();
  m_input.reset();
  m_reference.reset();
}

// Collect samples from image and do principal component analysis
// to determine the best weights for grayscale conversion.
void Task_Grayscale::do_pca()
{
  int xsamples = 64;
  int ysamples = 64;
  int total = xsamples * ysamples;
  cv::Mat samples(total, 3, CV_32F);
  const cv::Mat &src = m_input->img();

  for (int ys = 0; ys < ysamples; ys++)
  {
    for (int xs = 0; xs < xsamples; xs++)
    {
      int y = src.rows * ys / ysamples;
      int x = src.cols * xs / xsamples;
      int idx = ys * xsamples + xs;

      cv::Vec3b color = src.at<cv::Vec3b>(y, x);

      samples.at<float>(idx, 0) = color[0];
      samples.at<float>(idx, 1) = color[1];
      samples.at<float>(idx, 2) = color[2];
    }
  }

  cv::PCA pca(samples, cv::Mat(), cv::PCA::DATA_AS_ROW, 2);

  cv::Mat tmp(1, 2, CV_32F);
  tmp.at<float>(0, 0) = 1.0f;
  tmp.at<float>(0, 1) = 0.0f;
  m_weights = pca.backProject(tmp);

  tmp = 0.0f;
  m_weights -= pca.backProject(tmp);

  m_weights /= cv::sum(m_weights)[0];

  m_logger->verbose("Using grayscale weights R:%0.3f, G:%0.3f, B:%0.3f\n",
           m_weights.at<float>(2), m_weights.at<float>(1), m_weights.at<float>(0));
}

