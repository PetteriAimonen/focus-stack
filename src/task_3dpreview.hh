// Render a 3-dimensional preview image of the generated depthmap.

#pragma once
#include "worker.hh"

namespace focusstack {

class Task_3DPreview: public ImgTask
{
public:
  // Parameter view_vector is a direction vector from origin to camera.
  // Z-axis is always vertical in result image.
  Task_3DPreview(std::shared_ptr<ImgTask> depthmap,
                 std::shared_ptr<ImgTask> depthmap_mask,
                 std::shared_ptr<ImgTask> merged,
                 cv::Vec3f view_vector,
                 float z_scale);

private:
  virtual void task();

  std::shared_ptr<ImgTask> m_depthmap;
  std::shared_ptr<ImgTask> m_depthmap_mask;
  std::shared_ptr<ImgTask> m_merged;
  cv::Vec3f m_view_vector;
  float m_z_scale;
};

}