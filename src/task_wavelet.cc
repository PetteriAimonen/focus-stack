#include "task_wavelet.hh"
#include <opencv2/imgproc/imgproc.hpp>

using namespace focusstack;

// Complex Daubechies wavelets.
// hr = real lowpass, gr = real highpass, hi = imag lowpass, gi = imag highpass
#define FILTER_LEN 6
static const float filter_hr[FILTER_LEN] = {-0.0662912607f,  0.1104854346f,  0.6629126074f,
                                             0.6629126074f,  0.1104854346f, -0.0662912607f};
static const float filter_gr[FILTER_LEN] = {-0.0662912607f, -0.1104854346f,  0.6629126074f,
                                            -0.6629126074f,  0.1104854346f,  0.0662912607f};
static const float filter_hi[FILTER_LEN] = {-0.0855816496f, -0.0855816496f,  0.1711632992f,
                                             0.1711632992f, -0.0855816496f, -0.0855816496f};
static const float filter_gi[FILTER_LEN] = { 0.0855816496f, -0.0855816496f, -0.1711632992f,
                                             0.1711632992f,  0.0855816496f, -0.0855816496f};

Task_Wavelet::Task_Wavelet(std::shared_ptr<ImgTask> input, bool inverse)
{
  m_input = input;
  m_inverse = inverse;

  m_filename = input->filename();

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
    int factor = (2 << levels);
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
      cv::merge(channels, 2, m_result);
    }

    for (int i = 0; i < levels; i++)
    {
      int w = m_result.cols >> i;
      int h = m_result.rows >> i;
      cv::Mat srcarea = m_result(cv::Rect(0, 0, w, h));
      cv::Mat dstarea = tmp(cv::Rect(0, 0, w, h));

      decompose(srcarea, dstarea);

      // Copy the result back to m_result
      dstarea.copyTo(srcarea);
    }
  }
  else
  {
    // Perform composition from complex wavelets to real-valued image
    cv::Mat src = m_input->img();
    cv::Mat tmp1(src.rows, src.cols, CV_32FC2);
    cv::Mat tmp2(src.rows, src.cols, CV_32FC2);

    src.copyTo(tmp1);

    for (int i = levels - 1; i >= 0; i--)
    {
      int w = tmp1.cols >> i;
      int h = tmp1.rows >> i;
      cv::Mat srcarea = tmp1(cv::Rect(0, 0, w, h));
      cv::Mat dstarea = tmp2(cv::Rect(0, 0, w, h));

      compose(srcarea, dstarea);

      std::swap(tmp1, tmp2);
    }

    cv::Mat channels[2];
    cv::split(tmp1, channels);

    channels[0].convertTo(m_result, CV_8U);
  }

  m_input.reset();
}

// Performs one level of decomposition.
// Takes an input image, and outputs the complex values for downscaled / horizontal / diagonal / vertical subbands.
// Input matrix should not alias output matrix.
void Task_Wavelet::decompose(const cv::Mat& input, cv::Mat &output)
{
  cv::Mat tmp1(input.rows, input.cols, CV_32FC2);
  cv::Mat tmp2(input.rows, input.cols, CV_32FC2);

  // Real x Real decomposition
  decompose_1d(input, tmp1, true, false);
  decompose_1d(tmp1, output, false, false);

  // Real x Imag decomposition
  decompose_1d(tmp1, tmp2, false, true);
  output += tmp2;

  // Imag x Imag decomposition
  decompose_1d(input, tmp1, true, true);
  decompose_1d(tmp1, tmp2, false, true);
  output += tmp2;

  // Imag x Real decomposition
  decompose_1d(tmp1, tmp2, false, false);
  output += tmp2;
}

// Performs one level of composition
void Task_Wavelet::compose(const cv::Mat& input, cv::Mat &output)
{
  cv::Mat tmp1(input.rows, input.cols, CV_32FC2);
  cv::Mat tmp2(input.rows, input.cols, CV_32FC2);

  // Real x Real composition
  compose_1d(input, tmp1, true, false);
  compose_1d(tmp1, output, false, false);

  // Real x Imag composition
  compose_1d(tmp1, tmp2, false, true);
  output += tmp2;

  // Imag x Imag composition
  compose_1d(input, tmp1, true, true);
  compose_1d(tmp1, tmp2, false, true);
  output += tmp2;

  // Imag x Real composition
  compose_1d(tmp1, tmp2, false, false);
  output += tmp2;
}

// This function performs 1-dimensional complex wavelet decomposition.
// Both matrices should be 2-channel, where first channel is real part and
// second channel is imaginary part. First half of the dest row/col will
// contain the lowpass result, and second half will contain the highpass
// result.
void Task_Wavelet::decompose_1d(const cv::Mat &src, cv::Mat &dest, bool vertical, bool imag_filter)
{
  int count = vertical ? src.cols : src.rows;
  int length = vertical ? src.rows : src.cols;
  int halflen = length / 2;
  const float *lopass = imag_filter ? filter_hi : filter_hr;
  const float *hipass = imag_filter ? filter_gi : filter_gr;

  for (int x = 0; x < count; x++)
  {
    for (int y = 0; y < length; y += 2)
    {
      float re_lo = 0.0f;
      float im_lo = 0.0f;
      float re_hi = 0.0f;
      float im_hi = 0.0f;

      for (int j = 0; j < FILTER_LEN; j++)
      {
        int pos = y + j - FILTER_LEN / 2;
        if (pos < 0) pos = -pos;
        if (pos >= length) pos = (length - 1) - (pos - (length - 1));

        cv::Point xy = vertical ? cv::Point(x, pos) : cv::Point(pos, x);
        cv::Vec2f val = src.at<cv::Vec2f>(xy);

        if (imag_filter)
        {
          im_lo += val[0] * lopass[j];
          re_lo -= val[1] * lopass[j]; // imag src * imag filt = -real output
          im_hi += val[0] * hipass[j];
          re_hi -= val[1] * hipass[j];
        }
        else
        {
          re_lo += val[0] * lopass[j];
          im_lo += val[1] * lopass[j];
          re_hi += val[0] * hipass[j];
          im_hi += val[1] * hipass[j];
        }
      }

      if (vertical)
      {
        dest.at<cv::Vec2f>(cv::Point(x, y/2)) = cv::Vec2f(re_lo, im_lo);
        dest.at<cv::Vec2f>(cv::Point(x, y/2 + halflen)) = cv::Vec2f(re_hi, im_hi);
      }
      else
      {
        dest.at<cv::Vec2f>(cv::Point(y/2, x)) = cv::Vec2f(re_lo, im_lo);
        dest.at<cv::Vec2f>(cv::Point(y/2 + halflen, x)) = cv::Vec2f(re_hi, im_hi);
      }
    }
  }
}

// Opposite of decompose_1d.
void Task_Wavelet::compose_1d(const cv::Mat& src, cv::Mat& dest, bool vertical, bool imag_filter)
{
  int count = vertical ? src.cols : src.rows;
  int length = vertical ? src.rows : src.cols;
  int halflen = length / 2;
  const float *lopass = imag_filter ? filter_hi : filter_hr;
  const float *hipass = imag_filter ? filter_gi : filter_gr;

  for (int x = 0; x < count; x++)
  {
    for (int y = 0; y < length; y++)
    {
      float re = 0.0f;
      float im = 0.0f;

      for (int j = (y + FILTER_LEN / 2) % 2; j < FILTER_LEN; j += 2)
      {
        int pos = (y - j + FILTER_LEN / 2) / 2;
        if (pos < 0) pos = -pos;
        if (pos >= halflen) pos = (halflen - 1) - (pos - (halflen - 1));

        cv::Vec2f val_lo = src.at<cv::Vec2f>(vertical ? cv::Point(x, pos) : cv::Point(pos, x));
        cv::Vec2f val_hi = src.at<cv::Vec2f>(vertical ? cv::Point(x, pos + halflen) : cv::Point(pos + halflen, x));

        if (imag_filter)
        {
          im += val_lo[0] * lopass[j] + val_hi[0] * hipass[j];
          re -= val_lo[1] * lopass[j] + val_hi[1] * hipass[j];
        }
        else
        {
          re += val_lo[0] * lopass[j] + val_hi[0] * hipass[j];
          im += val_lo[1] * lopass[j] + val_hi[1] * hipass[j];
        }
      }

      if (vertical)
      {
        dest.at<cv::Vec2f>(cv::Point(x, y)) = cv::Vec2f(re, im);
      }
      else
      {
        dest.at<cv::Vec2f>(cv::Point(y, x)) = cv::Vec2f(re, im);
      }
    }
  }
}


