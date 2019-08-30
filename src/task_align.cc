#include "task_align.hh"
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/video.hpp>
#include <opencv2/core/utility.hpp>

using namespace focusstack;

Task_Align::Task_Align(std::shared_ptr<ImgTask> reference, std::shared_ptr<ImgTask> grayscale, std::shared_ptr<ImgTask> color)
{
  // Separate filename from path and add "aligned_" in front.
  std::string basename = color->filename();
  size_t pos = basename.find_last_of("/\\");
  if (pos != std::string::npos)
  {
    basename = basename.substr(pos + 1);
  }

  m_filename = "aligned_" + basename;
  m_name = "Align " + basename;

  m_reference = reference;
  m_grayscale = grayscale;
  m_color = color;

  m_depends_on.push_back(reference);
  m_depends_on.push_back(grayscale);
  m_depends_on.push_back(color);

  // Create initial guess for the transformation
  m_transformation.create(2, 3, CV_32F);
  m_transformation = 0;
  m_transformation.at<float>(0, 0) = 1.0f;
  m_transformation.at<float>(1, 1) = 1.0f;
  m_contrast = 1.0f;
}

void Task_Align::task()
{
  if (m_reference == m_grayscale)
  {
    m_result = m_color->img();
  }
  else
  {
    match_contrast();
    match_transform();
    apply_transform(m_color->img(), m_result, false);
  }


  m_reference.reset();
  m_grayscale.reset();
  m_color.reset();
}

void Task_Align::match_contrast()
{
  float sum = cv::sum(m_grayscale->img())[0];
  float refsum = cv::sum(m_reference->img())[0];

  m_contrast = refsum / sum;
}

void Task_Align::match_transform()
{
  cv::Mat tmp = m_grayscale->img();
  tmp *= m_contrast;
  cv::findTransformECC(m_reference->img(), tmp, m_transformation);
}

void Task_Align::apply_transform(const cv::Mat &src, cv::Mat &dst, bool inverse)
{
  int invflag = (!inverse) ? cv::WARP_INVERSE_MAP : 0;
  cv::warpAffine(src, dst, m_transformation, cv::Size(src.cols, src.rows), cv::INTER_CUBIC | invflag, cv::BORDER_REFLECT);
  dst *= m_contrast;
}
