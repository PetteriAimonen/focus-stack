#include "worker.hh"
#include <cstdio>

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
    if (!task->is_completed())
    {
      return false;
    }
  }

  return true;
}

void Task::run()
{
  std::lock_guard<std::mutex> lock(m_mutex);

  if (m_done)
  {
    throw std::logic_error("Task has already completed");
  }

  // Run the subclass implementation
  this->task();

  // Release memory we no longer need
  m_depends_on.clear();

  // Mark as completed
  m_done = true;
}

Worker::Worker(int max_threads, bool verbose):
  m_verbose(verbose), m_closed(false), m_tasks_completed(0), m_total_tasks(0)
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
  }

  m_closed = true;
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
  while (m_tasks.size())
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
        if (m_tasks.at(i)->ready_to_run())
        {
          task = m_tasks.at(i);
          m_tasks.erase(m_tasks.begin() + i);
          break;
        }
      }
    }

    if (task)
    {
      if (m_verbose)
      {
        std::unique_lock<std::mutex> lock(m_mutex);
        std::printf("%6.3f [%3d/%3d] T%d Starting task: %s\n",
                    seconds_passed(), m_tasks_completed, m_total_tasks, thread_idx, task->name().c_str());
      }

      task->run();

      {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_tasks_completed++;
        std::printf("%6.3f [%3d/%3d] T%d Finished task: %s\n",
                    seconds_passed(), m_tasks_completed, m_total_tasks, thread_idx, task->name().c_str());
      }

      // Wake all threads to re-check dependencies
      m_wakeup.notify_all();
    }
    else
    {
      std::unique_lock<std::mutex> lock(m_mutex);
      m_wakeup.wait(lock);
    }
  }
}
