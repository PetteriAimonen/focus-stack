// Construct spatial depthmap from the focus measures for individual layers.

#pragma once
#include "worker.hh"
#include "task_merge.hh"

namespace focusstack {

// This task works incrementally, updating the depthmap array for each new image.
// The focus measure for each layer is compared against its neighbours.
// If the current layer has the best focus, the depthmap value is set to depth.
class Task_Depthmap: public ImgTask
{
public:
  Task_Depthmap(std::shared_ptr<ImgTask> input,
                std::vector<std::shared_ptr<ImgTask> > neighbours,
                int depth,
                std::shared_ptr<Task_Depthmap> previous = nullptr);

  const cv::Mat &alphamap() { return m_largest_delta; }
  const cv::Mat &depthmap() { return m_depthmap; }

  // Form a rough mask of known depth values.
  // Halo radius is the blur distance for eliminating halo artefacts around high contrast edges.
  cv::Mat mask(int halo_radius) const;

private:
  virtual void task();

  cv::Mat m_depthmap;
  cv::Mat m_largest_delta;
  cv::Mat m_largest_focusmeasure;
  std::shared_ptr<ImgTask> m_input;
  std::vector<std::shared_ptr<ImgTask> > m_neighbours;
  int m_depth;
  std::shared_ptr<Task_Depthmap> m_previous;

  cv::Mat m_combined_image;
};

}
