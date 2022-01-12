#include "worker.hh"
#include <cstdio>

#ifdef USE_MALLINFO
#include <malloc.h>
#endif

using namespace focusstack;

Task::Task(): m_filename("unknown"), m_index(0), m_name("Base task"), m_running(false), m_done(false)
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
    if (!task)
    {
      throw std::logic_error("Task " + m_name + " depends on nullptr");
    }

    if (!task->is_completed())
    {
      return false;
    }
  }

  return true;
}

void Task::run(std::shared_ptr<Logger> logger)
{
  std::unique_lock<std::mutex> lock(m_mutex);

  if (logger)
  {
    m_logger = logger;
  }
  else
  {
    m_logger = std::make_shared<Logger>();
  }

  if (m_done)
  {
    throw std::logic_error("Task " + m_name + " has already completed but run() called");
  }

  m_running = true;

  try {
    // Run the subclass implementation
    this->task();

    // Release memory we no longer need
    m_depends_on.clear();

    // Mark as completed
    m_done = true;
    m_running = false;
    m_wakeup.notify_all();
  }
  catch (...)
  {
    // Mark as completed anyway
    m_done = true;
    m_running = false;
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

Worker::Worker(int max_threads, std::shared_ptr<Logger> logger):
  m_logger(logger), m_closed(false), m_tasks_started(0), m_total_tasks(0),
  m_completed_tasks(0), m_opencl_users(0), m_failed(false)
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
  assert(task);
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

bool Worker::wait_all(int timeout_ms)
{
  std::unique_lock<std::mutex> lock(m_mutex);

  auto timeout = std::chrono::system_clock::now();

  if (timeout_ms >= 0)
  {
   timeout += std::chrono::milliseconds(timeout_ms);
  }
  else
  {
    // Check for deadlocks after every 10 seconds
    timeout += std::chrono::seconds(10);
  }

  while (m_tasks.size() && !m_failed)
  {
    if (m_wakeup.wait_until(lock, timeout) == std::cv_status::timeout)
    {
      // Check if we are waiting on an unscheduled task
      for (std::shared_ptr<Task> task: m_tasks)
      {
        for (std::shared_ptr<Task> dependency: task->get_depends())
        {
          assert(dependency);
          if (!dependency->is_completed() && !m_running.count(dependency))
          {
            if (std::find(m_tasks.begin(), m_tasks.end(), dependency) == m_tasks.end())
            {
              m_logger->error("Task %s is waiting on unscheduled task %s\n",
                              task->name().c_str(), dependency->name().c_str());
            }
          }
        }
      }

      if (timeout_ms >= 0)
      {
        return false; // Return with timeout
      }
    }
  }

  return true; // Everything completed
}

void Worker::get_status(int &total_tasks, int &completed_tasks)
{
  std::unique_lock<std::mutex> lock(m_mutex);
  total_tasks = m_total_tasks;
  completed_tasks = m_completed_tasks;
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
          m_running.insert(task);
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

        if (m_logger->get_level() <= Logger::LOG_VERBOSE)
        {
          m_logger->verbose("%6.3f [%3d/%3d] T%d Starting task: %s\n",
                      seconds_passed(), m_tasks_started, m_total_tasks, thread_idx, task->name().c_str());
        }
        else
        {
          m_logger->progress("[%3d/%3d] %-40.40s\r", m_tasks_started, m_total_tasks, task->name().c_str());
        }
      }

      try
      {
        task->run(m_logger);
      }
      catch (std::exception &e)
      {
        m_error = "Task " + task->name() + " on thread " + std::to_string(thread_idx)
                  + " failed with exception:\n" + e.what();
        m_logger->error("\n\n%s\n", m_error.c_str());
        m_failed = true;
        m_wakeup.notify_all();
        return;
      }

      if (m_logger->get_level() <= Logger::LOG_VERBOSE)
      {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_logger->verbose("%6.3f           T%d Finished task %d in %0.3f s.\n",
                          seconds_passed(), thread_idx, taskidx, seconds_passed() - start);

#ifdef USE_MALLINFO
        struct mallinfo mem = mallinfo();
        m_logger->verbose("%6.3f           Memory use: %0.3f MB.\n", seconds_passed(), mem.uordblks / 1e6);
#endif
      }

      if (task->uses_opencl())
      {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_opencl_users--;
      }

      {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_completed_tasks++;
        m_running.erase(task);
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
