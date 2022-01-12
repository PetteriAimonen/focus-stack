// Implements a radial interpolation filter.
// For each output point, searches in many angles radially outwards for the nearest non-zero pixel.
// Each pixel is then interpolated as an average of the neighbours.
// This automatically weights close neighbours more, because more rays will hit them.

#pragma once
#include <opencv2/core.hpp>

namespace focusstack {

// Find the closest valid point in each radial direction and
// average them together to fill in any points with 0 value.
class RadialFilter
{
public:
  // Average all neighbour pixels for each output pixel.
  // Optionally segment labels can be specified to average only inside same segment.
  static cv::Mat average(cv::Mat input, int raycount = 64);

  // Connect two pixels with a line if they are close together in space and value.
  // If value limit is > 255, result matrix is a count of lines crossing each output point.
  // If value limit is smaller, result matrix is connected with the color of the points.
  static cv::Mat connect(cv::Mat input, int distance_limit, int value_limit = 256, int raycount = 64);

  // Connect two pixels with a line indicating the blob density
  static cv::Mat blobdistance(cv::Mat input, int raycount = 64);

  template <typename F>
  static inline void walk_at_angle(F callback, cv::Size imgsize, float angle);

  // Bresenham line algorithm.
  // Continues in given direction until callback returns false.
  template <typename F>
  static inline void bresenham_walk_direction(F callback, int x0, int y0, int dx, int dy);
};

}