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
                int depth, bool last,
                std::shared_ptr<Task_Depthmap> previous = nullptr,
                bool save_steps = false);

  const cv::Mat &depthmap() const { return m_result; }

  int maxdepth() const { return m_maxdepth; }

  // Form a rough mask of known depth values.
  // Halo radius is the blur distance for eliminating halo artefacts around high contrast edges.
  cv::Mat mask(int halo_radius) const;

private:
  virtual void task();

  // Estimate the background noise level (camera noise level)
  float estimate_noise_level(const cv::Mat &data);

  // Add one depth level to m_guo estimation matrix.
  void add_to_guo(const cv::Mat &y_values, float x);

  // Compute the final fitted Gaussian function for each pixel
  void compute_result();

  // Depth is estimated by fitting a Gaussian curve to the
  // focus measures using Guo's algorithm:
  // https://www.researchgate.net/publication/252062037_A_Simple_Algorithm_for_Fitting_a_Gaussian_Function_DSP_Tips_and_Tricks
  // The noiselevel is a constant background level of the data,
  // estimated from the first image in the stack.
  // The matrix m_guo is image-sized matrix with 8 channels:
  // 0: sum(y²)
  // 1: sum(x y²)
  // 2: sum(x² y²)
  // 3: sum(x³ y²)
  // 4: sum(x⁴ y²)
  // 5: sum(y² ln y)
  // 6: sum(x y² ln y)
  // 7: sum(x² y² ln y)
  int m_maxdepth;
  float m_noiselevel;
  cv::Mat m_guo;

  cv::Mat m_gauss_mean;
  cv::Mat m_gauss_dev;
  cv::Mat m_gauss_amp;

  std::shared_ptr<ImgTask> m_input;
  int m_depth;
  std::shared_ptr<Task_Depthmap> m_previous;
  bool m_last;
  bool m_save_steps;
};

}
