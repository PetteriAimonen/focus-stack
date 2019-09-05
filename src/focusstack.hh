// Constructs and starts the various tasks involved in the stacking process.

#pragma once
#include <string>
#include <vector>

namespace focusstack {

class FocusStack {
public:
  FocusStack();

  void set_inputs(const std::vector<std::string> &files) { m_inputs = files; }
  void set_output(std::string output) { m_output = output; }
  std::string get_output() const { return m_output; }
  void set_save_steps(bool save) { m_save_steps = save; }
  void set_verbose(bool verbose) { m_verbose = verbose; }
  void set_threads(int threads) { m_threads = threads; }
  void set_reference(int refidx) { m_reference = refidx; }
  void set_consistency(int level) { m_consistency = level; }
  void set_denoise(float level) { m_denoise = level; }

  void run();

private:
  std::vector<std::string> m_inputs;
  std::string m_output;
  bool m_save_steps;
  bool m_verbose;
  int m_threads;
  int m_reference;
  int m_consistency;
  float m_denoise;
};

}
