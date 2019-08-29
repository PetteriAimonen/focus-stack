#include <iostream>>
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
                 "  --threads=2                   Select number of threads to use\n"
                 "  --save_aligned                Save intermediate aligned images\n"
                 "  --verbose                     Verbose output from steps\n"
              << std::endl;
    return 1;
  }

  stack.set_inputs(options.get_filenames());
  stack.set_output(options.get_arg("--output", "output.jpg"));
  stack.set_save_aligned(options.has_flag("--save_aligned"));
  stack.set_verbose(options.has_flag("--verbose") or options.has_flag("-v"));

  if (options.has_flag("--threads"))
  {
    stack.set_threads(options.get_arg("--threads"));
  }

  stack.run();

  return 0;
}
