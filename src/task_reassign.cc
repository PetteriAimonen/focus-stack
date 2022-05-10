#include "task_reassign.hh"
#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs.hpp>

#define REASSIGN_MAX_BATCH 32

using namespace focusstack;

Task_Reassign_Map::Task_Reassign_Map(const std::vector<std::shared_ptr<ImgTask> > &grayscale_imgs,
                                     const std::vector<std::shared_ptr<ImgTask> > &color_imgs, std::shared_ptr<Task_Reassign_Map> old_map):
  m_grayscale_imgs(grayscale_imgs), m_color_imgs(color_imgs), m_old_map(old_map)
{
  m_filename = "reassign_map";
  m_name = "Build color reassignment map from " + std::to_string(m_color_imgs.size()) + " images";
  m_depends_on.insert(m_depends_on.begin(), grayscale_imgs.begin(), grayscale_imgs.end());
  m_depends_on.insert(m_depends_on.begin(), color_imgs.begin(), color_imgs.end());

  if (m_old_map)
  {
    m_depends_on.push_back(m_old_map);
  }
}

void Task_Reassign_Map::task()
{
  if (m_old_map)
  {
    m_grayscale_input = m_old_map->m_grayscale_input;
  }
  else
  {
    m_grayscale_input = (m_color_imgs.at(0)->img().channels() == 1);
  }

  if (m_grayscale_input)
  {
    build_gray();
  }
  else
  {
    build_color();
  }

  m_grayscale_imgs.clear();
  m_color_imgs.clear();
  m_old_map.reset();
}

void Task_Reassign_Map::build_color()
{
  // Check that all input images are in correct format
  int imgcount = m_grayscale_imgs.size();
  assert(imgcount < REASSIGN_MAX_BATCH);
  for (int i = 0; i < imgcount; i++)
  {
    assert(m_grayscale_imgs.at(i)->img().type() == CV_8U);
    assert(m_color_imgs.at(i)->img().type() == CV_8UC3);
  }

  // Calculate how much space we may need for the data
  const color_entry_t *old_colors = m_old_map ? (m_old_map->m_colors.data()) : nullptr;
  const uint8_t *old_counts = m_old_map ? (m_old_map->m_counts.data()) : nullptr;
  size_t old_map_size = m_old_map ? (m_old_map->m_colors.size()) : 0;
  int width = m_grayscale_imgs.at(0)->img().cols;
  int height = m_grayscale_imgs.at(0)->img().rows;

  // Preallocate the arrays to as much size as we may need to avoid checks during inner loop.
  m_colors.resize(width * height * (m_grayscale_imgs.size() + 1) + old_map_size);
  m_counts.resize(width * height);
  color_entry_t *colors_wrpos = m_colors.data();
  uint8_t *counts_wrpos = m_counts.data();

  // Each new pixel is checked against gray_seen[] array. If the entry matches current pixel_idx,
  // we have already seen this pixel value at this position, so it is not necessary to add new entry.
  uint32_t gray_seen[256] = {0};
  uint32_t pixel_idx = 1;
  const uint8_t *grayscale_row_ptrs[REASSIGN_MAX_BATCH] = {nullptr};
  const cv::Vec3b *color_row_ptrs[REASSIGN_MAX_BATCH] = {nullptr};

  // Go through all pixels in order, and add any color values that aren't already in map.
  for (int y = 0; y < height; y++)
  {
    // Get row pointers for each image, to speed up inner loop
    for (int i = 0; i < imgcount; i++)
    {
      grayscale_row_ptrs[i] = m_grayscale_imgs.at(i)->img().ptr<uint8_t>(y);
      color_row_ptrs[i] = m_color_imgs.at(i)->img().ptr<cv::Vec3b>(y);
    }

    for (int x = 0; x < width; x++)
    {
      pixel_idx++;

      int color_count = 0;

      // Bring in values from old map.
      // Copy each color into m_colors and mark it present in gray_seen[].
      if (old_colors)
      {
        color_count = (*old_counts++) + 1;
        for (int i = 0; i < color_count; i++)
        {
          color_entry_t color = *old_colors++;
          gray_seen[color.gray] = pixel_idx;
          *colors_wrpos++ = color;
        }
      }

      // Check for new values in the input images
      for (int i = 0; i < imgcount; i++)
      {
        uint8_t gray = grayscale_row_ptrs[i][x];
        if (gray_seen[gray] != pixel_idx)
        {
          gray_seen[gray] = pixel_idx;
          *colors_wrpos++ = color_entry_t(gray, color_row_ptrs[i][x]);
          color_count++;
        }
      }

      *counts_wrpos++ = color_count - 1;
    }
  }

  // Release any extra memory that was preallocated
  assert(colors_wrpos - m_colors.data() < m_colors.size());
  m_colors.resize(colors_wrpos - m_colors.data());
}

void Task_Reassign_Map::build_gray()
{
  int start = 0;
  if (m_old_map)
  {
    m_gray_min = m_old_map->m_gray_min.clone();
    m_gray_max = m_old_map->m_gray_max.clone();
    m_old_map.reset();
  }
  else
  {
    cv::Mat img0 = m_color_imgs.at(0)->img();
    m_gray_min = img0.clone();
    m_gray_max = img0.clone();
    start = 1;
  }

  int imgcount = m_color_imgs.size();
  for (int i = start; i < imgcount; i++)
  {
    assert(m_color_imgs.at(i)->img().type() == CV_8UC1);
    cv::min(m_gray_min, m_color_imgs.at(i)->img(), m_gray_min);
    cv::max(m_gray_max, m_color_imgs.at(i)->img(), m_gray_max);
  }
}

Task_Reassign::Task_Reassign(std::shared_ptr<Task_Reassign_Map> map,
                             std::shared_ptr<ImgTask> merged):
  m_map(map), m_merged(merged)
{
  m_filename = m_merged->filename();
  m_name = "Reassign pixel values";

  m_depends_on.push_back(map);
  m_depends_on.push_back(merged);
}

void Task_Reassign::task()
{
  m_valid_area = m_merged->valid_area();

  if (m_map->m_grayscale_input)
  {
    m_logger->verbose("Performing grayscale range limiting in reassignment step\n");
    reassign_gray();
  }
  else
  {
    reassign_color();
  }

  m_map.reset();
  m_merged.reset();
  return;
}

void Task_Reassign::reassign_color()
{
  cv::Mat merged = m_merged->img();
  m_result.create(merged.rows, merged.cols, CV_8UC3);

  const Task_Reassign_Map::color_entry_t *colors = m_map->m_colors.data();
  const uint8_t *counts = m_map->m_counts.data();
  int width = merged.cols;
  int height = merged.rows;

  for (int y = 0; y < height; y++)
  {
    for (int x = 0; x < width; x++)
    {
      // Get the number of color map entries for this pixel
      int color_count = *counts++ + 1;
      const Task_Reassign_Map::color_entry_t *pos = colors;
      colors += color_count;

      // Go through all entries and find the closest one
      uint8_t gray = merged.at<uint8_t>(y, x);
      Task_Reassign_Map::color_entry_t closest = *pos++;
      int error = std::abs(closest.gray - gray);

      while (error > 0 && pos != colors)
      {
        Task_Reassign_Map::color_entry_t candidate = *pos++;
        int distance = std::abs(candidate.gray - gray);
        if (distance < error)
        {
          error = distance;
          closest = candidate;
        }
      }

      m_result.at<cv::Vec3b>(y, x) = closest.color;
    }
  }
}

void Task_Reassign::reassign_gray()
{
  m_result = m_merged->img().clone();
  cv::min(m_result, m_map->m_gray_max, m_result);
  cv::max(m_result, m_map->m_gray_min, m_result);
}