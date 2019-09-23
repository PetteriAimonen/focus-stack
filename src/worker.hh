// Work queue that distributes alignment and merging operations between threads.

#pragma once
#include <thread>
#include <deque>
#include <vector>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <exception>
#include <opencv2/core/core.hpp>

namespace focusstack {

// Generic runnable task, optionally with some dependencies on other tasks
class Task
{
public:
  Task();
  virtual ~Task();

  virtual bool ready_to_run();
  virtual bool uses_opencl() { return false; }
  bool is_completed() const { return m_done; }
  void run(bool verbose = false);
  std::string filename() const { return m_filename; }
  std::string name() const { return m_name; }
  std::string basename() const;

  void wait();

protected:
  virtual void task() { };

  bool m_verbose;
  std::string m_filename;
  std::string m_name;
  std::mutex m_mutex;
  std::vector<std::shared_ptr<Task> > m_depends_on; // List of tasks this task needs as inputs

  std::condition_variable m_wakeup;
  bool m_done;
};

// Task that has image as a result.
class ImgTask: public Task
{
public:
  ImgTask() {};
  ImgTask(cv::Mat result): m_result(result) {}
  virtual const cv::Mat &img() const { return m_result; }

protected:
  cv::Mat m_result;
};

// Work queue class that distributes tasks to threads.
class Worker
{
public:
  Worker(int max_threads, bool verbose);
  ~Worker();

  // Add task to the end of the queue
  void add(std::shared_ptr<Task> task);

  // Prepend task, causing it to run as soon as possible
  void prepend(std::shared_ptr<Task> task);

  // Wait until all tasks have finished
  void wait_all();

  bool failed() const { return m_failed; }

  std::exception error() const { return m_error; }

private:
  bool m_verbose;
  std::vector<std::thread> m_threads;
  std::deque<std::shared_ptr<Task> > m_tasks;

  bool m_closed;
  int m_tasks_started;
  int m_total_tasks;
  int m_opencl_users;

  bool m_failed;
  std::exception m_error;

  std::mutex m_mutex;
  std::condition_variable m_wakeup;

  std::chrono::time_point<std::chrono::steady_clock> m_start_time;
  float seconds_passed() const;

  void worker(int thread_idx);
};


}
