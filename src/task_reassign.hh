// Performs pixel reassignment after inverse wavelet transform.
// Refer to "Complex Wavelets for Extended Depth-of-Field: A New Method for the Fusion of Multichannel Microscopy Images"
//          by B. Forster, D. Van De Ville, J. Berent, D. Sage, M. Unser, September 2004

#pragma once
#include "worker.hh"

namespace focusstack {

// Task_Reassign_Map builds a map between grayscale and color values.
// The map can be built incrementally, so that source images can be
// unloaded from RAM sooner.
//
// Each unique gray value for each pixel is stored only once, which
// conserves RAM compared to storing all source images.
//
// For grayscale input images color reassignment is not strictly necessary,
// but limiting the output values to input range reduces ringing artefacts
// in the image. The class uses an optimized implementation for grayscale
// case.
class Task_Reassign_Map: public Task
{
public:
  Task_Reassign_Map(const std::vector<std::shared_ptr<ImgTask> > &grayscale_imgs,
                    const std::vector<std::shared_ptr<ImgTask> > &color_imgs,
                    std::shared_ptr<Task_Reassign_Map> old_map);

private:
  virtual void task();

  // Build reassigment map for color input images.
  void build_color();

  // Build reassignment map for grayscale images.
  // This only stores the range of grayscale values present in input,
  // which helps with reducing any ringing artefacts.
  void build_gray();

  bool m_grayscale_input;
  std::vector<std::shared_ptr<ImgTask> > m_grayscale_imgs;
  std::vector<std::shared_ptr<ImgTask> > m_color_imgs;
  std::shared_ptr<Task_Reassign_Map> m_old_map;

  struct color_entry_t
  {
    uint8_t gray;
    cv::Vec3b color;

    // Writing out the three elements of Vec3b improves performance
    // at low optimization levels.
    color_entry_t(uint8_t _gray, const cv::Vec3b &_color):
      gray(_gray), color(_color[0], _color[1], _color[2]) {}
    color_entry_t() {}
    color_entry_t(const color_entry_t& old):
      gray(old.gray), color(old.color[0], old.color[1], old.color[2]) {}
    color_entry_t& operator=(const color_entry_t &old) = default;
  };

  // m_colors has color entries for all pixels, concatenated into one vector.
  // m_counts has number of entries per each pixel, minus one. Maximum count is 256, minimum count is 1.
  std::vector<color_entry_t> m_colors;
  std::vector<uint8_t> m_counts;

  cv::Mat m_gray_min;
  cv::Mat m_gray_max;

  friend class Task_Reassign;
};

// Task_Reassign converts grayscale image to color image by looking
// up color values from a previously built map.
class Task_Reassign: public ImgTask
{
public:
  Task_Reassign(std::shared_ptr<Task_Reassign_Map> map,
                std::shared_ptr<ImgTask> merged);

private:
  virtual void task();

  void reassign_color();
  void reassign_gray();

  std::shared_ptr<Task_Reassign_Map> m_map;
  std::shared_ptr<ImgTask> m_merged;
};

}
