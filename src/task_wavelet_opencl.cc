#include "task_wavelet_opencl.hh"
#include "task_wavelet_templates.hh"

using namespace focusstack;

Task_Wavelet_OpenCL::Task_Wavelet_OpenCL(std::shared_ptr<ImgTask> input, bool inverse):
  Task_Wavelet(input, inverse)
{
}

void Task_Wavelet_OpenCL::task()
{
  if (!m_inverse)
  {
    // Perform decomposition from real-valued image to complex wavelets

    cv::Mat img = m_input->img();

    // nth level wavelet decomposition requires image width to be multiple of 2^n
    int factor = (1 << levels);
    assert(img.rows % factor == 0 && img.cols % factor == 0);

    cv::Mat tmp(img.rows, img.cols, CV_32FC2);

    // Convert input image to complex values
    {
      cv::Mat fimg(img.rows, img.cols, CV_32F);
      cv::Mat zeros(img.rows, img.cols, CV_32F);

      img.convertTo(fimg, CV_32F);
      zeros = 0;

      cv::Mat channels[] = {fimg, zeros};
      cv::merge(channels, 2, tmp);
    }

    cv::UMat utmp(img.rows, img.cols, CV_32FC2);
    tmp.copyTo(utmp);

    cv::UMat uresult(img.rows, img.cols, CV_32FC2);
    Wavelet<cv::UMat>::decompose_multilevel(utmp, uresult, levels);

    uresult.copyTo(m_result);
  }
  else
  {
    // Perform composition from complex wavelets to real-valued image
    cv::UMat usrc = m_input->img().getUMat(cv::ACCESS_READ);
    cv::UMat utmp(usrc.rows, usrc.cols, CV_32FC2);

    Wavelet<cv::UMat>::compose_multilevel(usrc, utmp, levels);

    cv::Mat channels[2];
    cv::split(utmp.getMat(cv::ACCESS_READ), channels);

    channels[0].convertTo(m_result, CV_8U);
  }

  m_input.reset();
}
