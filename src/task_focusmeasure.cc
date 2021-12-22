#include "task_focusmeasure.hh"
#include <opencv2/imgproc.hpp>

using namespace focusstack;

Task_FocusMeasure::Task_FocusMeasure(std::shared_ptr<ImgTask> input, float radius, float threshold)
{
    m_filename = "focusmeasure_" + input->basename();
    m_name = "Focus measure for " + input->basename();
    m_index = input->index();

    m_input = input;
    m_depends_on.push_back(input);
    m_radius = radius;
    m_threshold = threshold;
}

void Task_FocusMeasure::task()
{
    // Algorithm is based on Tenengrad focus measure from
    // 'Autofocusing Algorithm Selection in Computer Microscopy' by Sun et Al.

    const cv::Mat &input = m_input->img();
    int rows = input.rows;
    int cols = input.cols;
    m_valid_area = m_input->valid_area();

    // Calculate squared gradient magnitude by calculating
    // horizontal and vertical Sobel operator.
    cv::Mat sobel(rows, cols, CV_32F);
    cv::Mat magnitude(rows, cols, CV_32F, cv::Scalar(0));
    cv::Sobel(input, sobel, CV_32FC1, 1, 0);
    cv::accumulateSquare(sobel, magnitude);
    cv::Sobel(input, sobel, CV_32FC1, 0, 1);
    cv::accumulateSquare(sobel, magnitude);
    sobel.release();
    m_input.reset();

    magnitude.setTo(0, magnitude < m_threshold);

    if (m_radius > 0)
    {
        int blurwindow = (int)(m_radius * 4) + 1;
        cv::GaussianBlur(magnitude, magnitude, cv::Size(blurwindow, blurwindow), m_radius, m_radius, cv::BORDER_REFLECT);
    }

    m_result = magnitude;
}