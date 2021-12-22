#include "task_wavelet.hh"
#include "task_wavelet_templates.hh"

using namespace focusstack;

Task_Wavelet::Task_Wavelet(std::shared_ptr<ImgTask> input, bool inverse)
{
  m_input = input;
  m_inverse = inverse;

  m_filename = input->filename();
  m_index = input->index();

  if (inverse)
    m_name = "Inverse-wavelet " + m_filename;
  else
    m_name = "Forward-wavelet " + m_filename;

  m_depends_on.push_back(input);
}

void Task_Wavelet::task()
{
  if (!m_inverse)
  {
    // Perform decomposition from real-valued image to complex wavelets

    cv::Mat img = m_input->img();

    // nth level wavelet decomposition requires image width to be multiple of 2^n
    int factor = (1 << levels);
    assert(img.rows % factor == 0 && img.cols % factor == 0);

    cv::Mat tmp(img.rows, img.cols, CV_32FC2);
    m_result.create(img.rows, img.cols, CV_32FC2);

    // Convert input image to complex values
    {
      cv::Mat fimg(img.rows, img.cols, CV_32F);
      cv::Mat zeros(img.rows, img.cols, CV_32F);

      img.convertTo(fimg, CV_32F);
      zeros = 0;

      cv::Mat channels[] = {fimg, zeros};
      cv::merge(channels, 2, tmp);
    }

    Wavelet<cv::Mat>::decompose_multilevel(tmp, m_result, levels);
  }
  else
  {
    // Perform composition from complex wavelets to real-valued image
    cv::Mat src = m_input->img();
    cv::Mat tmp(src.rows, src.cols, CV_32FC2);

    Wavelet<cv::Mat>::compose_multilevel(src, tmp, levels);

    cv::Mat channels[2];
    cv::split(tmp, channels);

    channels[0].convertTo(m_result, CV_8U);
  }

  m_valid_area = m_input->valid_area();
  m_input.reset();
}










