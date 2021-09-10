#include <gtest/gtest.h>
#include "task_grayscale.hh"
#include "logger.hh"

namespace focusstack {

TEST(Task_Grayscale, PCA) {
  cv::Mat input(64, 64, CV_8UC3);
  input = 0;
  input.at<cv::Vec3b>(32, 32)[1] = 1.0f;

  std::shared_ptr<Logger> logger = std::make_shared<Logger>();
  Task_Grayscale task(std::make_shared<ImgTask>(input));
  task.run(logger);

  ASSERT_EQ(task.weights().at<float>(0), 0.0f);
  ASSERT_EQ(task.weights().at<float>(1), 1.0f);
  ASSERT_EQ(task.weights().at<float>(2), 0.0f);
}

}
