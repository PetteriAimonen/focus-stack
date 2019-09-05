#include <iostream>
#include "options.hh"
#include "focusstack.hh"

using namespace focusstack;

int main(int argc, const char *argv[])
{
  Options options(argc, argv);
  FocusStack stack;

  if (options.has_flag("--help") or options.get_filenames().size() < 2)
  {
    std::cerr << "Usage: " << argv[0] << " [options] file1.jpg file2.jpg ...\n";
    std::cerr << "Options:\n"
                 "  --output=output.jpg           Set output filename\n"
                 "  --reference=0                 Set index of image used as alignment reference (default middle one)\n"
                 "  --threads=2                   Select number of threads to use (default number of CPUs)\n"
                 "  --consistency=2               Set depth map consistency filter level 0..2 (default 2)\n"
                 "  --denoise=1.0                 Set image denoise level (default 1.0)\n"
                 "  --save_steps                  Save intermediate images from processing steps\n"
                 "  --verbose                     Verbose output from steps\n"
              << std::endl;
    return 1;
  }

  stack.set_inputs(options.get_filenames());
  stack.set_output(options.get_arg("--output", "output.jpg"));
  stack.set_save_steps(options.has_flag("--save_steps"));
  stack.set_verbose(options.has_flag("--verbose") or options.has_flag("-v"));
  stack.set_denoise(std::stof(options.get_arg("--denoise", "1.0")));
  stack.set_consistency(std::stoi(options.get_arg("--consistency", "2")));

  if (options.has_flag("--threads"))
  {
    stack.set_threads(std::stoi(options.get_arg("--threads")));
  }

  if (options.has_flag("--reference"))
  {
    stack.set_reference(std::stoi(options.get_arg("--reference")));
  }

  stack.run();

  std::printf("\rSaved to %-40s\n", stack.get_output().c_str());

  return 0;
}
