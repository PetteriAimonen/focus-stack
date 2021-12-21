#include "task_3dpreview.hh"
#include <opencv2/imgproc.hpp>

using namespace focusstack;

Task_3DPreview::Task_3DPreview(std::shared_ptr<ImgTask> depthmap,
                               std::shared_ptr<ImgTask> depthmap_mask,
                               std::shared_ptr<ImgTask> merged,
                               cv::Vec3f view_vector, float z_scale):
  m_depthmap(depthmap), m_depthmap_mask(depthmap_mask), m_merged(merged),
  m_view_vector(view_vector), m_z_scale(z_scale)
{
  m_name = "Render 3D preview image";
  m_filename = "3dview.png";

  m_depends_on.push_back(m_depthmap);
  m_depends_on.push_back(m_merged);

  if (m_depthmap_mask)
  {
    m_depends_on.push_back(m_depthmap_mask);
  }
}

static inline float v3f_norm(cv::Vec3f v)
{
  return sqrtf(v.dot(v));
}

static inline cv::Vec3f v3f_normalize(cv::Vec3f v)
{
  return v * (1.0f / v3f_norm(v));
}

void Task_3DPreview::task()
{
  cv::Mat depthmap = m_depthmap->img();
  cv::Mat merged = m_merged->img();

  // Output image size is same as merged image size
  int rows = merged.rows;
  int cols = merged.cols;
  m_result.create(rows, cols, CV_8UC4);
  m_result = 0;

  // Calculate camera plane base vectors
  cv::Vec3f view_vector = v3f_normalize(m_view_vector);
  cv::Vec3f object_z = cv::Vec3f(0, 0, 1);
  cv::Vec3f camera_y = v3f_normalize(object_z - view_vector * view_vector[2]);
  cv::Vec3f camera_x = v3f_normalize(camera_y.cross(view_vector));
  float camera_z = v3f_norm(camera_y.dot(object_z)) * m_z_scale;

  // Select iteration direction from back to front
  int x_start, x_end, x_step;
  int y_start, y_end, y_step;

  if (view_vector[0] > 0)
  {
    x_start = 0; x_end = merged.cols - 1; x_step = 1;
  }
  else
  {
    x_start = merged.cols - 1; x_end = 0; x_step = -1;
  }

  if (view_vector[1] > 0)
  {
    y_start = 0; y_end = merged.rows - 1; y_step = 1;
  }
  else
  {
    y_start = merged.rows - 1; y_end = 0; y_step = -1;
  }

  // Render pixel by pixel from back to front
  int y_prev = y_start;
  for (int y = y_start; y < y_end; y += y_step)
  {
    int x_prev = x_start;
    for (int x = x_start; x < x_end; x += x_step)
    {
      uint8_t depth = depthmap.at<uint8_t>(y, x);
      uint8_t depth_back = std::max({
        depth,
        depthmap.at<uint8_t>(y_prev, x),
        depthmap.at<uint8_t>(y_prev, x_prev),
        depthmap.at<uint8_t>(y, std::max(x_start, x - x_step)),
      });

      // Project the depthmap point to camera plane
      cv::Vec3f objp(x - merged.cols / 2, y - merged.rows / 2, 0);
      float cam_x = -camera_x.dot(objp) + cols / 2;
      float cam_y = -camera_y.dot(objp) + rows / 2;

      cv::Vec3b pixel = merged.at<cv::Vec3b>(y, x);

      if (cam_x >= 0 && cam_x < cols - 1)
      {
        for (float d = depth; d <= depth_back; d += 0.9f / camera_z)
        {
          float cam_y_d = cam_y - camera_z * (d - 128);

          if (cam_y_d > 0 && cam_y_d < rows)
          {
            m_result.at<cv::Vec4b>(cam_y_d, cam_x) = cv::Vec4b(
              pixel[0], pixel[1], pixel[2], 255
            );

            m_result.at<cv::Vec4b>(cam_y_d, cam_x + 1) = cv::Vec4b(
              pixel[0], pixel[1], pixel[2], 255
            );
          }
        }
      }

      x_prev = x;
    }
    y_prev = y;
  }
}