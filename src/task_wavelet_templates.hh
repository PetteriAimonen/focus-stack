// This file contains templated implementations of the wavelet decomposition.
// These are used both with M for CPU-based computation and with cv::UMat
// for GPU-based computation.

#pragma once
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/ocl.hpp>
#include <mutex>
#include "task_wavelet_opencl_kernels.cl"

namespace focusstack {

template <typename M>
class Wavelet {
public:
  static void decompose_multilevel(const M &input, M &output, int levelcount);
  static void decompose(const M &input, M &output);
  static void decompose_1d(const M &src, M &dest, bool vertical);

  static void compose_multilevel(const M &input, M &output, int levelcount);
  static void compose(const M &input, M &output);
  static void compose_1d(const M &src, M &dest, bool vertical);

  static cv::ocl::Program &opencl_load_kernel();

private:
  // Complex Daubechies wavelets.
  static constexpr int FILTER_LEN = 6;

  static constexpr float c_lopass[16] = {
    -0.0662912607f, -0.0855816496f,
     0.1104854346f, -0.0855816496f,
     0.6629126074f,  0.1711632992f,
     0.6629126074f,  0.1711632992f,
     0.1104854346f, -0.0855816496f,
    -0.0662912607f, -0.0855816496f,

    0.0f, 0.0f, 0.0f, 0.0f // Padding for float16 access in OpenCL kernel
  };

  static constexpr float c_hipass[16] = {
     -0.0662912607f,  0.0855816496f,
     -0.1104854346f, -0.0855816496f,
      0.6629126074f, -0.1711632992f,
     -0.6629126074f,  0.1711632992f,
      0.1104854346f,  0.0855816496f,
      0.0662912607f, -0.0855816496f,
      0.0f, 0.0f, 0.0f, 0.0f
  };
};

// These lines are needed to avoid undefined reference to the constexpr arrays.
template <typename M> constexpr float Wavelet<M>::c_lopass[];
template <typename M> constexpr float Wavelet<M>::c_hipass[];

// Performs multiple levels of decomposition
// Begins with whole image, then processes the upper left corner that contains
// the downscaled image from previous step.
template <typename M>
void Wavelet<M>::decompose_multilevel(const M &input, M &output, int levelcount)
{
  M tmp(input.rows, input.cols, CV_32FC2);

  for (int i = 0; i < levelcount; i++)
  {
    int w = input.cols >> i;
    int h = input.rows >> i;
    M srcarea;
    M dstarea = output(cv::Rect(0, 0, w, h));

    if (i == 0)
    {
      srcarea = input(cv::Rect(0, 0, w, h));
    }
    else
    {
      srcarea = tmp(cv::Rect(0, 0, w, h));
      dstarea.copyTo(srcarea);
    }

    decompose(srcarea, dstarea);
  }
}

// Performs multiple levels of composition.
// Begins with the upper left corner, composing it to the downscaled
// image for next level.
template <typename M>
void Wavelet<M>::compose_multilevel(const M& input, M& output, int levelcount)
{
  M tmp(input.rows, input.cols, CV_32FC2);

  input.copyTo(tmp);

  for (int i = levelcount - 1; i >= 0; i--)
  {
    int w = input.cols >> i;
    int h = input.rows >> i;
    M srcarea = tmp(cv::Rect(0, 0, w, h));
    M dstarea = output(cv::Rect(0, 0, w, h));

    compose(srcarea, dstarea);

    dstarea.copyTo(tmp(cv::Rect(0, 0, w, h)));
  }
}

// Performs one level of decomposition.
// Takes an input image, and outputs the complex values for downscaled / horizontal / diagonal / vertical subbands.
// Input matrix should not alias output matrix.
template <typename M>
void Wavelet<M>::decompose(const M& input, M &output)
{
  M tmp1(input.rows, input.cols, CV_32FC2);

  decompose_1d(input, tmp1, true);
  decompose_1d(tmp1, output, false);
}

// Performs one level of composition
template <typename M>
void Wavelet<M>::compose(const M& input, M &output)
{
  M tmp1(input.rows, input.cols, CV_32FC2);

  compose_1d(input, tmp1, true);
  compose_1d(tmp1, output, false);
}

// This function performs 1-dimensional complex wavelet decomposition.
// Both matrices should be 2-channel, where first channel is real part and
// second channel is imaginary part. First half of the dest row/col will
// contain the lowpass result, and second half will contain the highpass
// result.
template <>
inline void Wavelet<cv::Mat>::decompose_1d(const cv::Mat &src, cv::Mat &dest, bool vertical)
{
  int count = vertical ? src.cols : src.rows;
  int length = vertical ? src.rows : src.cols;
  int halflen = length / 2;
  const cv::Vec2f *lopass = reinterpret_cast<const cv::Vec2f*>(c_lopass);
  const cv::Vec2f *hipass = reinterpret_cast<const cv::Vec2f*>(c_hipass);

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
        if (pos < 0) pos = length + pos;
        if (pos >= length) pos = pos - length;

        cv::Point xy = vertical ? cv::Point(x, pos) : cv::Point(pos, x);
        cv::Vec2f val = src.at<cv::Vec2f>(xy);

        re_lo += val[0] * lopass[j][0] - val[1] * lopass[j][1];
        im_lo += val[1] * lopass[j][0] + val[0] * lopass[j][1];
        re_hi += val[0] * hipass[j][0] - val[1] * hipass[j][1];
        im_hi += val[1] * hipass[j][0] + val[0] * hipass[j][1];
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
template <>
inline void Wavelet<cv::Mat>::compose_1d(const cv::Mat& src, cv::Mat& dest, bool vertical)
{
  int count = vertical ? src.cols : src.rows;
  int length = vertical ? src.rows : src.cols;
  int halflen = length / 2;
  const cv::Vec2f *lopass = reinterpret_cast<const cv::Vec2f*>(c_lopass);
  const cv::Vec2f *hipass = reinterpret_cast<const cv::Vec2f*>(c_hipass);

  for (int x = 0; x < count; x++)
  {
    for (int y = 0; y < length; y++)
    {
      float re = 0.0f;
      float im = 0.0f;

      for (int j = (y + FILTER_LEN / 2) % 2; j < FILTER_LEN; j += 2)
      {
        int pos = (y - j + FILTER_LEN / 2) / 2;
        if (pos < 0) pos = halflen + pos;
        if (pos >= halflen) pos = pos - halflen;

        cv::Vec2f val_lo = src.at<cv::Vec2f>(vertical ? cv::Point(x, pos) : cv::Point(pos, x));
        cv::Vec2f val_hi = src.at<cv::Vec2f>(vertical ? cv::Point(x, pos + halflen) : cv::Point(pos + halflen, x));

        re += val_lo[0] * lopass[j][0] + val_hi[0] * hipass[j][0];
        re += val_lo[1] * lopass[j][1] + val_hi[1] * hipass[j][1];
        im += val_lo[1] * lopass[j][0] + val_hi[1] * hipass[j][0];
        im -= val_lo[0] * lopass[j][1] + val_hi[0] * hipass[j][1];
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


template<typename M>
cv::ocl::Program &Wavelet<M>::opencl_load_kernel()
{
  static std::once_flag s_init;
  static cv::ocl::Program s_program;

  std::call_once(s_init, [](){
    cv::ocl::ProgramSource kernel_progsrc(g_focusstack_wavelet_kernel_src);
    cv::String buildflags = "";
    cv::String errmsg;
    s_program.create(kernel_progsrc, buildflags, errmsg);
  });

  return s_program;
}

// 1-dimensional decomposition with OpenCL acceleration
template <>
inline void Wavelet<cv::UMat>::decompose_1d(const cv::UMat &src, cv::UMat &dest, bool vertical)
{
  cv::ocl::Program &prog = opencl_load_kernel();
  cv::ocl::Kernel kernel;
  size_t globalThreads[2];

  if (vertical)
  {
    kernel.create("decompose_vertical", prog);
    globalThreads[0] = dest.cols;
    globalThreads[1] = dest.rows / 2;
  }
  else
  {
    kernel.create("decompose_horizontal", prog);
    globalThreads[0] = dest.cols / 2;
    globalThreads[1] = dest.rows;
  }

  kernel.args(cv::ocl::KernelArg::ReadOnlyNoSize(src),
              cv::ocl::KernelArg::WriteOnly(dest),
              cv::ocl::KernelArg::Constant(c_lopass, sizeof(float) * 16),
              cv::ocl::KernelArg::Constant(c_hipass, sizeof(float) * 16));

  if (!kernel.run(2, globalThreads, NULL, true))
  {
    throw std::runtime_error("Failed to execute OpenCL kernel");
  }
}

template <>
inline void Wavelet<cv::UMat>::compose_1d(const cv::UMat &src, cv::UMat &dest, bool vertical)
{
  cv::ocl::Program &prog = opencl_load_kernel();
  cv::ocl::Kernel kernel;
  size_t globalThreads[2];

  if (vertical)
  {
    kernel.create("compose_vertical", prog);
    globalThreads[0] = dest.cols;
    globalThreads[1] = dest.rows / 2;
  }
  else
  {
    kernel.create("compose_horizontal", prog);
    globalThreads[0] = dest.cols / 2;
    globalThreads[1] = dest.rows;
  }

  kernel.args(cv::ocl::KernelArg::ReadOnlyNoSize(src),
              cv::ocl::KernelArg::WriteOnly(dest),
              cv::ocl::KernelArg::Constant(c_lopass, sizeof(float) * 16),
              cv::ocl::KernelArg::Constant(c_hipass, sizeof(float) * 16));

  if (!kernel.run(2, globalThreads, NULL, true))
  {
    throw std::runtime_error("Failed to execute OpenCL kernel");
  }
}


}
