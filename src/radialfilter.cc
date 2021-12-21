#include "radialfilter.hh"
#include <opencv2/imgproc.hpp>

using namespace focusstack;

static inline float sq(float x) { return x * x; }

// Averaging class for the Bresenham walker
// Keeps track of the last pixel and sums it to each output position.
struct radialfilter_avg_walker_t {
  radialfilter_avg_walker_t(cv::Mat &input, cv::Mat &sum): m_input(input), m_sum(sum), m_prev_value(0) {}
  cv::Mat &m_input;
  cv::Mat &m_sum;
  int m_prev_value;

  bool operator()(int x, int y)
  {
    if (x < 0 || y < 0 || x >= m_input.cols || y >= m_input.rows)
    {
      return false;
    }

    int value = m_input.at<uint8_t>(y, x);
    cv::Vec2s &s = m_sum.at<cv::Vec2s>(y, x);
    if (m_prev_value > 0)
    {
      s = cv::Vec2s(s[0] + m_prev_value, s[1] + 1);
    }
    else if (value > 0)
    {
      s = cv::Vec2s(s[0] + value, s[1] + 1);
    }

    if (value > 0)
    {
      m_prev_value = value;
    }

    return true;
  }
};

// Painting class for the walker
// Adds 1 to all pixels between two points.
struct radialfilter_paint_walker_t {
  radialfilter_paint_walker_t(cv::Mat &result, int endx, int endy): m_result(result), m_endx(endx), m_endy(endy) {}
  cv::Mat &m_result;
  int m_endx, m_endy;

  bool operator()(int x, int y)
  {
    if (x < 0 || y < 0 || x >= m_result.cols || y >= m_result.rows)
    {
      return false;
    }

    uint8_t &s = m_result.at<uint8_t>(y, x);
    if (s < 255) s += 1;

    if (x == m_endx && y == m_endy)
    {
      return false;
    }
    else
    {
      return true;
    }
  }
};

#include <iostream>

// Connecting class for the walker
// Keeps track of the last pixel and then paints a line between them.
struct radialfilter_connect_walker_t {
  radialfilter_connect_walker_t(cv::Mat &input, cv::Mat &result, int distance_limit):
    m_input(input), m_result(result), m_prevx(-1), m_prevy(-1), m_distance_limit(distance_limit) {}
  cv::Mat &m_input;
  cv::Mat &m_result;
  int m_prevx, m_prevy;
  int m_distance_limit;

  bool operator()(int x, int y)
  {
    if (x < 0 || y < 0 || x >= m_result.cols || y >= m_result.rows)
    {
      return false;
    }

    if (m_input.at<uint8_t>(y, x) > 0)
    {
      if (m_prevx > 0)
      {
        int distance = sq(x - m_prevx) + sq(y - m_prevy);
        if (distance > 2 && distance < sq(m_distance_limit))
        {
          radialfilter_paint_walker_t painter(m_result, x, y);
          RadialFilter::bresenham_walk_direction(painter, m_prevx, m_prevy, x - m_prevx, y - m_prevy);
        }
      }

      m_prevx = x;
      m_prevy = y;
    }

    return true;
  }
};

cv::Mat RadialFilter::average(cv::Mat input, int raycount)
{
  int cols = input.cols;
  int rows = input.rows;

  cv::Mat sum(rows, cols, CV_16UC2);
  sum = cv::Vec2f(0, 0);

  radialfilter_avg_walker_t walker(input, sum);

  for (int i = 0; i < raycount; i++)
  {
    float angle = 2 * M_PI * i / raycount;
    walk_at_angle(walker, input.size(), angle);
  }

  cv::Mat result(rows, cols, CV_8UC1);
  result = 0;

  for (int y = 0; y < rows; y++)
  {
    for (int x = 0; x < cols; x++)
    {
      cv::Vec2s s = sum.at<cv::Vec2s>(y, x);

      if (s[1] > 0)
      {
        result.at<uint8_t>(y, x) = (s[0] + s[1]/2) / s[1];
      }
      else
      {
        result.at<uint8_t>(y, x) = input.at<uint8_t>(y, x);
      }
    }
  }

  return result;
}

cv::Mat RadialFilter::connect(cv::Mat input, int distance_limit, int raycount)
{
  int cols = input.cols;
  int rows = input.rows;

  cv::Mat result(rows, cols, CV_8UC1);
  result = 0;

  radialfilter_connect_walker_t walker(input, result, distance_limit);

  for (int i = 0; i < raycount; i++)
  {
    float angle = 2 * M_PI * i / raycount;
    walk_at_angle(walker, input.size(), angle);
  }

  return result;
}

template <typename F>
void RadialFilter::walk_at_angle(F callback, cv::Size imgsize, float angle)
{
  int r = std::max(imgsize.width, imgsize.height);
  int dy = sinf(angle) * r;
  int dx = cosf(angle) * r;

  if (dy > 0)
  {
    // Start from upper edge
    for (int x = 0; x < imgsize.width; x++)
    {
      F walker(callback);
      bresenham_walk_direction(walker, x, 0, dx, dy);
    }
  }
  else if (dy < 0)
  {
    // Start from bottom edge
    for (int x = 0; x < imgsize.width; x++)
    {
      F walker(callback);
      bresenham_walk_direction(walker, x, imgsize.height - 1, dx, dy);
    }
  }

  if (dx > 0)
  {
    // Start from left edge
    for (int y = 0; y < imgsize.height; y++)
    {
      F walker(callback);
      bresenham_walk_direction(walker, 0, y, dx, dy);
    }
  }
  else if (dx < 0)
  {
    // Start from right edge
    for (int y = 0; y < imgsize.height; y++)
    {
      F walker(callback);
      bresenham_walk_direction(walker, imgsize.width - 1, y, dx, dy);
    }
  }
}

template <typename F>
void RadialFilter::bresenham_walk_direction(F callback, int x0, int y0, int dx, int dy)
{
  int sx = 1, sy = -1;

  if (dx < 0) { dx = -dx; sx = -sx; }
  if (dy > 0) { dy = -dy; sy = -sy; }

  int err = dx + dy;

  while(callback(x0, y0))
  {
    int e2 = 2 * err;

    if (e2 >= dy)
    {
      err += dy;
      x0 += sx;
    }

    if (e2 <= dx)
    {
      err += dx;
      y0 += sy;
    }
  }
}
