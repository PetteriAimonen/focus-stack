// Constructs and starts the various tasks involved in the stacking process.

#pragma once
#include <string>
#include <vector>

namespace focusstack {

class FocusStack {
public:
  FocusStack();

  enum align_flags_t
  {
    ALIGN_DEFAULT             = 0x00,
    ALIGN_NO_WHITEBALANCE     = 0x01,
    ALIGN_NO_CONTRAST         = 0x02,
    ALIGN_FULL_RESOLUTION     = 0x04,
    ALIGN_GLOBAL              = 0x08
  };

  void set_inputs(const std::vector<std::string> &files) { m_inputs = files; }
  void set_output(std::string output) { m_output = output; }
  std::string get_output() const { return m_output; }
  void set_disable_opencl(bool disable) { m_disable_opencl = disable; }
  void set_save_steps(bool save) { m_save_steps = save; }
  void set_align_only(bool align_only) { m_align_only = align_only; }
  void set_verbose(bool verbose) { m_verbose = verbose; }
  void set_threads(int threads) { m_threads = threads; }
  void set_batchsize(int batchsize) { m_batchsize = batchsize; }
  void set_reference(int refidx) { m_reference = refidx; }
  void set_jpgquality(int level) { m_jpgquality = level; }
  void set_consistency(int level) { m_consistency = level; }
  void set_denoise(float level) { m_denoise = level; }
  void set_align_flags(int flags) { m_align_flags = static_cast<align_flags_t>(flags); }

  bool run();

private:
  std::vector<std::string> m_inputs;
  std::string m_output;
  bool m_disable_opencl;
  bool m_save_steps;
  bool m_align_only;
  bool m_verbose;
  align_flags_t m_align_flags;

  int m_threads;
  int m_batchsize;
  int m_reference;
  int m_consistency;
  int m_jpgquality;
  float m_denoise;
};

}
