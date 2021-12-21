/**
   This sofrware and program is distributed under MIT License.
   Original alogorithm : http://people.csail.mit.edu/sparis/bf/
   Please cite above paper for research purpose.
   The MIT License (MIT)
   Copyright (c) 2015 Yuichi Takeda
   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:
   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.
 */

#ifndef __FAST_BILATERAL__
#define __FAST_BILATERAL__

#include <opencv2/opencv.hpp>

namespace cv_extend {

void bilateralFilter(cv::InputArray src, cv::OutputArray dst,
                     double sigmaColor, double sigmaSpace);

void bilateralFilterImpl(cv::Mat1d src, cv::Mat1d dst,
                         double sigmaColor, double sigmaSpace);


template<typename T, typename T_, typename T__>
inline
T clamp(const T_ min, const T__ max, const T x)
{
    return
        ( x < static_cast<T>(min) ) ? static_cast<T>(min) :
        ( x < static_cast<T>(max) ) ? static_cast<T>(x) :
        static_cast<T>(max);
}

template<typename T>
inline
T
trilinear_interpolation( const cv::Mat mat,
                         const double y,
                         const double x,
                         const double z)
{
    const size_t height = mat.size[0];
    const size_t width  = mat.size[1];
    const size_t depth  = mat.size[2];

    const size_t y_index  = clamp(0, height-1, static_cast<size_t>(y));
    const size_t yy_index = clamp(0, height-1, y_index+1);
    const size_t x_index  = clamp(0, width-1, static_cast<size_t>(x));
    const size_t xx_index = clamp(0, width-1, x_index+1);
    const size_t z_index  = clamp(0, depth-1, static_cast<size_t>(z));
    const size_t zz_index = clamp(0, depth-1, z_index+1);
    const double y_alpha = y - y_index;
    const double x_alpha = x - x_index;
    const double z_alpha = z - z_index;

    return
        (1.0-y_alpha) * (1.0-x_alpha) * (1.0-z_alpha) * mat.at<T>(y_index, x_index, z_index) +
        (1.0-y_alpha) * x_alpha       * (1.0-z_alpha) * mat.at<T>(y_index, xx_index, z_index) +
        y_alpha       * (1.0-x_alpha) * (1.0-z_alpha) * mat.at<T>(yy_index, x_index, z_index) +
        y_alpha       * x_alpha       * (1.0-z_alpha) * mat.at<T>(yy_index, xx_index, z_index) +
        (1.0-y_alpha) * (1.0-x_alpha) * z_alpha       * mat.at<T>(y_index, x_index, zz_index) +
        (1.0-y_alpha) * x_alpha       * z_alpha       * mat.at<T>(y_index, xx_index, zz_index) +
        y_alpha       * (1.0-x_alpha) * z_alpha       * mat.at<T>(yy_index, x_index, zz_index) +
        y_alpha       * x_alpha       * z_alpha       * mat.at<T>(yy_index, xx_index, zz_index);

}


/**
 * Implementation
 */

void bilateralFilter(cv::InputArray _src, cv::OutputArray _dst,
                     double sigmaColor, double sigmaSpace)
{
    cv::Mat src = _src.getMat();

    CV_Assert(src.channels() == 1);

    // bilateralFilterImpl runs with double depth, single channel
    if ( src.depth() != CV_64FC1 ) {
        src = cv::Mat(_src.size(), CV_64FC1);
        _src.getMat().convertTo(src, CV_64FC1);
    }

    cv::Mat dst_tmp = cv::Mat(_src.size(), CV_64FC1);
    bilateralFilterImpl(src, dst_tmp, sigmaColor, sigmaSpace);

    _dst.create(dst_tmp.size(), _src.type());
    dst_tmp.convertTo(_dst.getMat(), _src.type());

}

void bilateralFilterImpl(cv::Mat1d src, cv::Mat1d dst,
                         double sigma_color, double sigma_space)
{
    using namespace cv;
    const size_t height = src.rows, width = src.cols;
    const size_t padding_xy = 2, padding_z = 2;
    double src_min, src_max;
    cv::minMaxLoc(src, &src_min, &src_max);

    const int small_height = static_cast<int>((height-1)/sigma_space) + 1 + 2 * padding_xy;
    const int small_width  = static_cast<int>((width-1)/sigma_space) + 1 + 2 * padding_xy;
    const int small_depth  = static_cast<int>((src_max-src_min)/sigma_color) + 1 + 2 * padding_xy;

    int data_size[] = {small_height, small_width, small_depth};
    cv::Mat data(3, data_size, CV_64FC2);
    data.setTo(0);

    // down sample
    for ( int y = 0; y < height; ++y ) {
        for ( int x = 0; x < width; ++x) {
            const size_t small_x = static_cast<size_t>( x/sigma_space + 0.5) + padding_xy;
            const size_t small_y = static_cast<size_t>( y/sigma_space + 0.5) + padding_xy;
            const double z = src.at<double>(y,x) - src_min;
            const size_t small_z = static_cast<size_t>( z/sigma_color + 0.5 ) + padding_z;

            cv::Vec2d v = data.at<cv::Vec2d>(small_y, small_x, small_z);
            v[0] += src.at<double>(y,x);
            v[1] += 1.0;
            data.at<cv::Vec2d>(small_y, small_x, small_z) = v;
        }
    }

    // convolution
    cv::Mat buffer(3, data_size, CV_64FC2);
    buffer.setTo(0);
    int offset[3];
    offset[0] = &(data.at<cv::Vec2d>(1,0,0)) - &(data.at<cv::Vec2d>(0,0,0));
    offset[1] = &(data.at<cv::Vec2d>(0,1,0)) - &(data.at<cv::Vec2d>(0,0,0));
    offset[2] = &(data.at<cv::Vec2d>(0,0,1)) - &(data.at<cv::Vec2d>(0,0,0));

    for ( int dim = 0; dim < 3; ++dim ) { // dim = 3 stands for x, y, and depth
        const int off = offset[dim];
        for ( int ittr = 0; ittr < 2; ++ittr ) {
            cv::swap(data, buffer);

            for ( int y = 1; y < small_height-1; ++y ) {
                for ( int x = 1; x < small_width-1; ++x ) {
                    cv::Vec2d *d_ptr = &(data.at<cv::Vec2d>(y,x,1));
                    cv::Vec2d *b_ptr = &(buffer.at<cv::Vec2d>(y,x,1));
                    for ( int z = 1; z < small_depth-1; ++z, ++d_ptr, ++b_ptr ) {
                        cv::Vec2d b_prev = *(b_ptr-off), b_curr = *b_ptr, b_next = *(b_ptr+off);
                        *d_ptr = (b_prev + b_next + 2.0 * b_curr) / 4.0;
                    } // z
                } // x
            } // y

        } // ittr
    } // dim

    // upsample

    for ( cv::MatIterator_<cv::Vec2d> d = data.begin<cv::Vec2d>(); d != data.end<cv::Vec2d>(); ++d )
    {
        (*d)[0] /= (*d)[1] != 0 ? (*d)[1] : 1;
    }

    for ( int y = 0; y < height; ++y ) {
        for ( int x = 0; x < width; ++x ) {
            const double z = src.at<double>(y,x) - src_min;
            const double px = static_cast<double>(x) / sigma_space + padding_xy;
            const double py = static_cast<double>(y) / sigma_space + padding_xy;
            const double pz = static_cast<double>(z) / sigma_color + padding_z;
            dst.at<double>(y,x) = trilinear_interpolation<cv::Vec2d>(data, py, px, pz)[0];
        }
    }

}

} // end of namespace cv_extend
#endif