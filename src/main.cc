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
                 "  --global-align                Align directly against reference (default with neighbour image)\n"
                 "  --full-resolution-align       Use full resolution images in alignment (default max 2048 px)\n"
                 "  --no-whitebalance             Don't attempt to correct white balance differences\n"
                 "  --no-contrast                 Don't attempt to correct contrast and exposure differences\n"
                 "  --threads=2                   Select number of threads to use (default number of CPUs + 1)\n"
                 "  --no-opencl                   Disable OpenCL GPU acceleration (default enabled)\n"
                 "  --consistency=2               Set depth map consistency filter level 0..2 (default 2)\n"
                 "  --denoise=1.0                 Set image denoise level (default 1.0)\n"
                 "  --save-steps                  Save intermediate images from processing steps\n"
                 "  --verbose                     Verbose output from steps\n"
              << std::endl;
    return 1;
  }

  stack.set_inputs(options.get_filenames());
  stack.set_output(options.get_arg("--output", "output.jpg"));
  stack.set_save_steps(options.has_flag("--save-steps"));
  stack.set_disable_opencl(options.has_flag("--no-opencl"));
  stack.set_verbose(options.has_flag("--verbose") or options.has_flag("-v"));
  stack.set_denoise(std::stof(options.get_arg("--denoise", "1.0")));
  stack.set_consistency(std::stoi(options.get_arg("--consistency", "2")));

  int flags = FocusStack::ALIGN_DEFAULT;
  if (options.has_flag("--global-align"))             flags |= FocusStack::ALIGN_GLOBAL;
  if (options.has_flag("--full-resolution-align"))    flags |= FocusStack::ALIGN_FULL_RESOLUTION;
  if (options.has_flag("--no-whitebalance"))          flags |= FocusStack::ALIGN_NO_WHITEBALANCE;
  if (options.has_flag("--no-contrast"))              flags |= FocusStack::ALIGN_NO_CONTRAST;
  stack.set_align_flags(flags);

  if (options.has_flag("--threads"))
  {
    stack.set_threads(std::stoi(options.get_arg("--threads")));
  }

  if (options.has_flag("--reference"))
  {
    stack.set_reference(std::stoi(options.get_arg("--reference")));
  }

  std::vector<std::string> unparsed = options.get_unparsed();
  if (unparsed.size())
  {
    std::cerr << "Warning: unknown options: ";
    for (std::string arg: unparsed)
    {
      std::cerr << arg << " ";
    }
    std::cerr << std::endl;
  }


  if (!stack.run())
  {
    std::printf("\nError exit due to failed steps\n");
    return 1;
  }

  std::printf("\rSaved to %-40s\n", stack.get_output().c_str());

  return 0;
}
