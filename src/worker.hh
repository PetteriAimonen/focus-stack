// Work queue that distributes alignment and merging operations between threads.

#pragma once
#include <thread>
#include <queue>
#include <memory>
#include <exception>
#include <opencv2/core/core.hpp>

namespace focusstack {

// Generic runnable task, optionally with some dependencies on other tasks
class Task
{
public:
  virtual bool ready_to_run();
  virtual bool is_completed() const { return m_done; }
  virtual void run() = 0;
  virtual std::string filename() const { return "unknown"; }
  virtual std::string name() const { return filename(); }

protected:
  std::vector<Task&> m_inputs; // List of tasks this task needs as inputs
  bool m_done;
};

// Task that has image as a result.
// Can have either both or one of rgb/grayscale results, if other is not available it throws exception.
class ImgTask: public Task
{
public:
  virtual const cv::Mat &img_rgb() const { throw std::logic_error("RGB data is not available"); }
  virtual const cv::Mat &img_gray() const  { throw std::logic_error("Grayscale data is not available"); }
};

// Work queue class that distributes tasks to threads.
class Worker
{
public:
  Worker(int max_threads, bool verbose);

  void add(std::unique_ptr<Task> task);

  void runall();

private:
  bool m_verbose;
  std::vector<std::thread> m_threads;
  std::vector<std::unique_ptr<Task> > m_tasks;

  void worker();
};


}
