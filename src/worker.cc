#include "worker.hh"
#include <cstdio>

#ifdef USE_MALLINFO
#include <malloc.h>
#endif

using namespace focusstack;

Task::Task(): m_filename("unknown"), m_name("Base task"), m_done(false)
{

}

Task::~Task()
{
}

// Check whether all dependencies of this task have been completed
bool Task::ready_to_run()
{
  for (const std::shared_ptr<Task> &task: m_depends_on)
  {
    assert(task);
    if (!task->is_completed())
    {
      return false;
    }
  }

  return true;
}

void Task::run(bool verbose)
{
  std::unique_lock<std::mutex> lock(m_mutex);

  m_verbose = verbose;

  if (m_done)
  {
    throw std::logic_error("Task has already completed");
  }

  try {
    // Run the subclass implementation
    this->task();

    // Release memory we no longer need
    m_depends_on.clear();

    // Mark as completed
    m_done = true;
    m_wakeup.notify_all();
  }
  catch (...)
  {
    // Mark as completed anyway
    m_done = true;
    m_wakeup.notify_all();

    throw;
  }
}

std::string Task::basename() const
{
  // Separate filename from path
  std::string basename = filename();
  size_t pos = basename.find_last_of("/\\");
  if (pos != std::string::npos)
  {
    basename = basename.substr(pos + 1);
  }
  return basename;
}

void Task::wait()
{
  std::unique_lock<std::mutex> lock(m_mutex);

  while (!m_done)
  {
    m_wakeup.wait(lock);
  }
}

Worker::Worker(int max_threads, bool verbose):
  m_verbose(verbose), m_closed(false), m_tasks_started(0), m_total_tasks(0), m_opencl_users(0),
  m_failed(false)
{
  m_start_time = std::chrono::steady_clock::now();

  for (int i = 0; i < max_threads; i++)
  {
    m_threads.emplace_back([this, i](){ this->worker(i); });
  }
}

Worker::~Worker()
{
  {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_tasks.clear();
    m_closed = true;
  }

  m_wakeup.notify_all();

  for (std::thread &thread: m_threads)
  {
    thread.join();
  }
}

void Worker::add(std::shared_ptr<Task> task)
{
  std::unique_lock<std::mutex> lock(m_mutex);
  m_tasks.emplace_back(task);
  m_total_tasks++;
  m_wakeup.notify_all();
}

void Worker::prepend(std::shared_ptr<Task> task)
{
  std::unique_lock<std::mutex> lock(m_mutex);
  m_tasks.emplace_front(task);
  m_total_tasks++;
  m_wakeup.notify_all();
}

void Worker::wait_all()
{
  std::unique_lock<std::mutex> lock(m_mutex);
  while (m_tasks.size() && !m_failed)
  {
    m_wakeup.wait(lock);
  }
}

float Worker::seconds_passed() const
{
  std::chrono::time_point<std::chrono::steady_clock> now = std::chrono::steady_clock::now();
  auto delta = now - m_start_time;
  return std::chrono::duration_cast<std::chrono::milliseconds>(delta).count() / 1000.0f;
}

// This is the worker thread; it is run in multiple copies in separate threads.
// Each thread will take the first runnable task from the queue and execute it.
void Worker::worker(int thread_idx)
{
  while (!m_closed)
  {
    std::shared_ptr<Task> task = nullptr;

    {
      std::unique_lock<std::mutex> lock(m_mutex);

      // Search for next runnable task
      for (int i = 0; i < m_tasks.size(); i++)
      {
        if (m_opencl_users > 0 && m_tasks.at(i)->uses_opencl())
        {
          continue;
        }

        if (m_tasks.at(i)->ready_to_run())
        {
          task = m_tasks.at(i);
          m_tasks.erase(m_tasks.begin() + i);
          break;
        }
      }

      if (task && task->uses_opencl())
        m_opencl_users++;
    }

    if (task)
    {
      float start = seconds_passed();
      int taskidx = 0;

      {
        std::unique_lock<std::mutex> lock(m_mutex);
        taskidx = ++m_tasks_started;

        if (m_verbose)
        {
          std::printf("%6.3f [%3d/%3d] T%d Starting task: %s\n",
                      seconds_passed(), m_tasks_started, m_total_tasks, thread_idx, task->name().c_str());
        }
        else
        {
          std::printf("[%3d/%3d] %-40.40s\r", m_tasks_started, m_total_tasks, task->name().c_str());
          std::fflush(stdout);
        }
      }

      try
      {
        task->run(m_verbose);
      }
      catch (std::exception &e)
      {
        fprintf(stderr, "\n\nTask %s on thread %d failed with exception:\n%s\n",
                task->name().c_str(), thread_idx, e.what());
        m_error = e;
        m_failed = true;
        m_wakeup.notify_all();
        return;
      }

      if (m_verbose)
      {
        std::unique_lock<std::mutex> lock(m_mutex);
        std::printf("%6.3f           T%d Finished task %d in %0.3f s.\n",
                    seconds_passed(), thread_idx, taskidx, seconds_passed() - start);

#ifdef USE_MALLINFO
        struct mallinfo mem = mallinfo();
        std::printf("%6.3f           Memory use: %0.3f MB.\n", seconds_passed(), mem.uordblks / 1e6);
#endif
      }

      if (task->uses_opencl())
      {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_opencl_users--;
      }

      // Wake all threads to re-check dependencies
      m_wakeup.notify_all();
    }
    else
    {
      std::unique_lock<std::mutex> lock(m_mutex);

      if (m_closed)
      {
        break;
      }

      m_wakeup.wait(lock);
    }
  }
}
