#include "radialfilter.hh"
#include <opencv2/imgproc.hpp>

using namespace focusstack;

static inline float sq(float x) { return x * x; }

// Averaging class for the Bresenham walker
// Keeps track of the last pixel and sums it to each output position.
struct radialfilter_avg_walker_t {
  radialfilter_avg_walker_t(cv::Mat &input, cv::Mat &sum):
    m_input(input), m_sum(sum), m_prev_value(0) {}
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

// Adding paint class for the walker
// Adds 1 to all pixels between two points.
struct radialfilter_add_walker_t {
  radialfilter_add_walker_t(cv::Mat &result, int endx, int endy): m_result(result), m_endx(endx), m_endy(endy) {}
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

    return !(x == m_endx && y == m_endy);
  }
};

// Setting paint class for the walker
// Sets pixels between two points to value
struct radialfilter_paint_walker_t {
  radialfilter_paint_walker_t(cv::Mat &result, int endx, int endy, int value):
    m_result(result), m_endx(endx), m_endy(endy), m_value(value) {}
  cv::Mat &m_result;
  int m_endx, m_endy;
  int m_value;

  bool operator()(int x, int y)
  {
    if (x < 0 || y < 0 || x >= m_result.cols || y >= m_result.rows)
    {
      return false;
    }

    cv::Vec2s &s = m_result.at<cv::Vec2s>(y, x);
    s = cv::Vec2s(s[0] + m_value, s[1] + 1);

    return !(x == m_endx && y == m_endy);
  }
};

// Connecting class for the walker
// Keeps track of the last pixel and then paints a line between them.
struct radialfilter_connect_walker_t {
  radialfilter_connect_walker_t(cv::Mat &input, cv::Mat &result, int distance_limit, int value_limit):
    m_input(input), m_result(result), m_prevx(-1), m_prevy(-1), m_prevv(0),
    m_distance_limit(distance_limit), m_value_limit(value_limit) {}
  cv::Mat &m_input;
  cv::Mat &m_result;
  int m_prevx, m_prevy, m_prevv;
  int m_distance_limit;
  int m_value_limit;

  bool operator()(int x, int y)
  {
    if (x < 0 || y < 0 || x >= m_result.cols || y >= m_result.rows)
    {
      return false;
    }

    int v = m_input.at<uint8_t>(y, x);
    if (v > 0)
    {
      if (m_prevx > 0)
      {
        int distance = sq(x - m_prevx) + sq(y - m_prevy);
        if (distance > 2 && distance < sq(m_distance_limit))
        {
          radialfilter_paint_walker_t painter(m_result, x, y, (m_prevv + v) / 2);
          RadialFilter::bresenham_walk_direction(painter, m_prevx, m_prevy, x - m_prevx, y - m_prevy);
        }
      }

      m_prevx = x;
      m_prevy = y;
      m_prevv = v;
    }

    return true;
  }
};

// Maximum painting class for the walker
// Sets pixels to value if higher than old value.
struct radialfilter_paint_max_walker_t {
  radialfilter_paint_max_walker_t(cv::Mat &result, int value, int endx, int endy):
    m_result(result), m_value(value), m_endx(endx), m_endy(endy) {}
  cv::Mat &m_result;
  int m_value, m_endx, m_endy;

  bool operator()(int x, int y)
  {
    if (x < 0 || y < 0 || x >= m_result.cols || y >= m_result.rows)
    {
      return false;
    }

    uint8_t &s = m_result.at<uint8_t>(y, x);
    if (s < m_value) s = m_value;

    return !(x == m_endx && y == m_endy);
  }
};

// Blob distance calculation class for the walker
// Measures distance between blobs
struct radialfilter_blob_distance_walker_t {
  radialfilter_blob_distance_walker_t(cv::Mat &input, cv::Mat &result):
    m_input(input), m_result(result), m_prevx(-1), m_prevy(-1), m_prev_was_0(false) {}
  cv::Mat &m_input;
  cv::Mat &m_result;
  int m_prevx, m_prevy;
  bool m_prev_was_0;

  bool operator()(int x, int y)
  {
    if (x < 0 || y < 0 || x >= m_result.cols || y >= m_result.rows)
    {
      return false;
    }

    if (m_input.at<uint8_t>(y, x) > 0) // Are we inside a blob?
    {
      if (m_prev_was_0) // Is this first pixel in the blob?
      {
        if (m_prevx > 0) // Was there a previous blob?
        {
          int distance = sqrtf(sq(x - m_prevx) + sq(y - m_prevy));

          if (distance < 255)
          {
            radialfilter_paint_max_walker_t painter(m_result, 255 - distance, x, y);
            RadialFilter::bresenham_walk_direction(painter, m_prevx, m_prevy, x - m_prevx, y - m_prevy);
          }
        }

        // Store start of blob
        m_prevx = x;
        m_prevy = y;
        m_prev_was_0 = false;
      }
    }
    else
    {
      // Outside of blob
      m_prev_was_0 = true;
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

  // Convert sum and count to result format
  cv::Mat result(rows, cols, CV_8UC1);
  cv::Mat c_sum, c_count;
  cv::extractChannel(sum, c_sum, 0);
  cv::extractChannel(sum, c_count, 1);

  c_sum /= c_count;
  c_sum.convertTo(result, CV_8UC1);
  input.copyTo(result, c_count == 0);

  return result;
}

cv::Mat RadialFilter::connect(cv::Mat input, int distance_limit, int value_limit, int raycount)
{
  int cols = input.cols;
  int rows = input.rows;

  cv::Mat sum(rows, cols, CV_16UC2);
  sum = cv::Vec2f(0, 0);

  radialfilter_connect_walker_t walker(input, sum, distance_limit, value_limit);

  for (int i = 0; i < raycount; i++)
  {
    float angle = 2 * M_PI * i / raycount;
    walk_at_angle(walker, input.size(), angle);
  }

  // Convert sum and count to result format
  cv::Mat result(rows, cols, CV_8UC1);
  cv::Mat c_sum, c_count;
  cv::extractChannel(sum, c_sum, 0);
  cv::extractChannel(sum, c_count, 1);

  if (value_limit >= 256)
  {
    // Result is count of crossing lines
    c_count.convertTo(result, CV_8UC1);
  }
  else
  {
    // Result is average of line values
    c_sum /= c_count;
    c_sum.setTo(0, c_count == 0);
    c_sum.convertTo(result, CV_8UC1);
    input.copyTo(result, input > 0);
  }

  return result;
}

cv::Mat RadialFilter::blobdistance(cv::Mat input, int raycount)
{
  int cols = input.cols;
  int rows = input.rows;

  cv::Mat result(rows, cols, CV_8UC1);
  result = 0;

  radialfilter_blob_distance_walker_t walker(input, result);

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
