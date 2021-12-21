// Constructs and starts the various tasks involved in the stacking process.

#pragma once
#include <sstream>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <opencv2/core/core.hpp>

namespace focusstack {

class Task_LoadImg;
class Task_Grayscale;
class Task_Merge;
class Task_Align;
class Task_Reassign_Map;
class Worker;
class ImgTask;
class Logger;

class FocusStack {
public:
  FocusStack();
  virtual ~FocusStack();

  enum align_flags_t
  {
    ALIGN_DEFAULT             = 0x00,
    ALIGN_NO_WHITEBALANCE     = 0x01,
    ALIGN_NO_CONTRAST         = 0x02,
    ALIGN_FULL_RESOLUTION     = 0x04,
    ALIGN_GLOBAL              = 0x08
  };

  enum log_level_t
  {
      LOG_VERBOSE = 10,
      LOG_PROGRESS = 20,
      LOG_INFO = 30,
      LOG_ERROR = 40
  };

  void set_inputs(const std::vector<std::string> &files) { m_inputs = files; }
  void set_output(std::string output) { m_output = output; }
  std::string get_output() const { return m_output; }
  void set_depthmap(std::string depthmap) { m_depthmap = depthmap; }
  std::string get_depthmap() const { return m_depthmap; }
  void set_3dview(std::string filename_3dview) { m_filename_3dview = filename_3dview; }
  std::string get_3dview() const { return m_filename_3dview; }
  void set_depthmap_smoothing(float smoothing) { m_depthmap_smoothing = smoothing; }
  void set_disable_opencl(bool disable) { m_disable_opencl = disable; }
  void set_save_steps(bool save) { m_save_steps = save; }
  void set_align_only(bool align_only) { m_align_only = align_only; }
  void set_verbose(bool verbose);
  void set_threads(int threads) { m_threads = threads; }
  void set_batchsize(int batchsize) { m_batchsize = batchsize; }
  void set_reference(int refidx) { m_reference = refidx; }
  void set_jpgquality(int level) { m_jpgquality = level; }
  void set_consistency(int level) { m_consistency = level; }
  void set_denoise(float level) { m_denoise = level; }
  void set_align_flags(int flags) { m_align_flags = static_cast<align_flags_t>(flags); }
  void set_3dviewpoint(float x, float y, float z, float zscale) { m_3dviewpoint = cv::Vec3f(x,y,z); m_3dzscale = zscale; }
  void set_3dviewpoint(std::string value) {
    std::istringstream is(value);
    is >> m_3dviewpoint[0];
    is.ignore(1,':');
    is >> m_3dviewpoint[1];
    is.ignore(1,':');
    is >> m_3dviewpoint[2];
    is.ignore(1,':');
    is >> m_3dzscale;
  }

  // Set callback function to use for log messages.
  // Note that callbacks may come from any thread, but only one at a time.
  void set_log_callback(std::function<void(log_level_t level, std::string)> callback);

  // Blocking interface, equivalent to reset(); start(); do_final_merge(); wait_done();
  bool run();

  // Streaming interface, normal usage:
  // 1. Call add_image() if you have any pre-existing images.
  // 2. Call start() to start worker threads running.
  // 3. Call add_image() whenever new images arrive.
  // 4. Call do_final_merge() when all images have been added.
  // 5. Call wait_done() to wait for completion and retrieve success/fail status.
  //
  // Note that this class should be accessed only from one thread at a time.
  // With the exception of wait_done(), the functions return immediately.
  //
  void start(); // Start worker threads.
  void add_image(std::string filename); // Add image from file, filename must remain valid until loading completes.
  void add_image(const cv::Mat &image); // Add image from memory, buffer can be reused after add_image() returns.
  void do_final_merge(); // Do final merge operations.
  void get_status(int &total_tasks, int &completed_tasks); // Query status on running tasks
  bool wait_done(bool &status, std::string &errmsg, int timeout_ms = -1); // Wait until all tasks have completed and retrieve status
  void reset(bool keep_results = false); // Release memory buffers and clear state for next run.

  // Access result images in memory (without saving to files)
  // To enable generation of depthmap, call set_depthmap(":memory:");
  const cv::Mat &get_result_image() const;
  const cv::Mat &get_result_depthmap() const;
  const cv::Mat &get_result_3dview() const;

private:
  std::vector<std::string> m_inputs;
  std::string m_output;
  std::string m_depthmap;
  std::string m_filename_3dview;
  float m_depthmap_smoothing;
  bool m_disable_opencl;
  bool m_save_steps;
  bool m_align_only;
  std::shared_ptr<Logger> m_logger;
  align_flags_t m_align_flags;

  cv::Vec3f m_3dviewpoint;
  float m_3dzscale;

  int m_threads;
  int m_batchsize;
  int m_reference;
  int m_consistency;
  int m_jpgquality;
  float m_denoise;

  // Runtime variables
  bool m_have_opencl;
  int m_scheduled_image_count;
  int m_refidx;
  std::unique_ptr<Worker> m_worker;
  std::vector<std::shared_ptr<Task_LoadImg> > m_input_images; // Queued input images
  std::vector<std::shared_ptr<ImgTask> > m_grayscale_imgs;
  std::vector<std::shared_ptr<Task_Align> > m_aligned_imgs;
  std::vector<std::shared_ptr<ImgTask> > m_aligned_grayscales;
  std::shared_ptr<Task_LoadImg> m_refcolor; // Alignment reference image
  std::shared_ptr<Task_Grayscale> m_refgray; // Grayscaled reference image
  std::shared_ptr<Task_Merge> m_prev_merge;

  std::vector<std::shared_ptr<ImgTask> > m_merge_batch;
  std::vector<std::shared_ptr<ImgTask> > m_reassign_batch_grays;
  std::vector<std::shared_ptr<ImgTask> > m_reassign_batch_colors;
  std::shared_ptr<Task_Reassign_Map> m_reassign_map;

  // Result variables
  std::shared_ptr<ImgTask> m_result_image;
  std::shared_ptr<ImgTask> m_result_depthmap;
  std::shared_ptr<ImgTask> m_result_3dview;

  // Queue worker tasks for new images in m_input_images
  void schedule_queue_processing();
  void schedule_alignment(int i);
  void schedule_single_image_processing(int i);
  void schedule_batch_merge();

  // Release temporary images that are no longer needed
  void release_temporaries();

  // Schedule the last merge task and saving of final image
  void schedule_final_merge();
};

}
