// Work queue that distributes alignment and merging operations between threads.

#pragma once
#include <thread>
#include <deque>
#include <vector>
#include <memory>
#include <exception>
#include <opencv2/core/core.hpp>

namespace focusstack {

// Generic runnable task, optionally with some dependencies on other tasks
class Task
{
public:
  virtual bool ready_to_run();
  bool is_completed() const { return m_done; }
  void run();
  virtual std::string filename() const { return "unknown"; }
  virtual std::string name() const { return filename(); }

protected:
  virtual void task();

  std::vector<std::shared_ptr<Task> > m_inputs; // List of tasks this task needs as inputs
  bool m_done;
};

// Task that has image as a result.
class ImgTask: public Task
{
public:
  virtual const cv::Mat &img() const;
};

// Work queue class that distributes tasks to threads.
class Worker
{
public:
  Worker(int max_threads, bool verbose);

  // Add task to the end of the queue
  void add(std::shared_ptr<Task> task);
  void add(const std::vector<std::shared_ptr<Task> > task);

  // Prepend task, causing it to run as soon as possible
  void prepend(std::shared_ptr<Task> task);

  void runall();

private:
  bool m_verbose;
  std::vector<std::thread> m_threads;
  std::deque<std::shared_ptr<Task> > m_tasks;

  void worker();
};


}
