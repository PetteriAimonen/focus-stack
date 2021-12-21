#include <iostream>
#include "options.hh"
#include "focusstack.hh"
#include <opencv2/core.hpp>

#ifndef GIT_VERSION
#define GIT_VERSION "unknown"
#endif

using namespace focusstack;

int main(int argc, const char *argv[])
{
  Options options(argc, argv);
  FocusStack stack;

  if (options.has_flag("--version"))
  {
    std::cerr << "focus-stack 1.0, git version " GIT_VERSION ", built " __DATE__ " " __TIME__ "\n"
                 "Compiled with OpenCV version " CV_VERSION "\n"
                 "Copyright (c) 2019 Petteri Aimonen\n\n"

"Permission is hereby granted, free of charge, to any person obtaining a copy\n"
"of this software and associated documentation files (the \"Software\"), to\n"
"deal in the Software without restriction, including without limitation the\n"
"rights to use, copy, modify, merge, publish, distribute, sublicense, and/or\n"
"sell copies of the Software, and to permit persons to whom the Software is\n"
"furnished to do so, subject to the following conditions:\n\n"

"The above copyright notice and this permission notice shall be included in all\n"
"copies or substantial portions of the Software.\n\n"

"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n"
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n"
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n"
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\n"
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\n"
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE\n"
"SOFTWARE."
              << std::endl;
    return 0;
  }

  if (options.has_flag("--opencv-version"))
  {
    std::cerr << cv::getBuildInformation().c_str() << std::endl;
    return 0;
  }

  if (options.has_flag("--help") || options.get_filenames().size() < 2)
  {
    std::cerr << "Usage: " << argv[0] << " [options] file1.jpg file2.jpg ...\n";
    std::cerr << "\n";
    std::cerr << "Output file options:\n"
                 "  --output=output.jpg           Set output filename\n"
                 "  --depthmap=depthmap.png       Write a depth map image (default disabled)\n"
                 "  --3dview=3dview.png           Write a 3D preview image (default disabled)\n"
                 "  --save-steps                  Save intermediate images from processing steps\n"
                 "  --jpgquality=95               Set quality for saving in JPG format, 0..100 (default 95)\n";
    std::cerr << "\n";
    std::cerr << "Image alignment options:\n"
                 "  --reference=0                 Set index of image used as alignment reference (default middle one)\n"
                 "  --global-align                Align directly against reference (default with neighbour image)\n"
                 "  --full-resolution-align       Use full resolution images in alignment (default max 2048 px)\n"
                 "  --no-whitebalance             Don't attempt to correct white balance differences\n"
                 "  --no-contrast                 Don't attempt to correct contrast and exposure differences\n"
                 "  --align-only                  Only align the input image stack and exit\n";
    std::cerr << "\n";
    std::cerr << "Image merge options:\n"
                 "  --consistency=2               Neighbour pixel consistency filter level 0..2 (default 2)\n"
                 "  --denoise=1.0                 Merged image denoise level (default 1.0)\n";
    std::cerr << "\n";
    std::cerr << "Depth map generation options:\n"
                 "  --depthmap-smoothing=0.02     Smoothing level for depthmap output (default 0.02)\n"
                 "  --3dviewpoint=x:y:z:zscale    Viewpoint for 3D view (default 0.5:1.0:0.5:2.0)\n";
    std::cerr << "\n";
    std::cerr << "Performance options:\n"
                 "  --threads=2                   Select number of threads to use (default number of CPUs + 1)\n"
                 "  --batchsize=8                 Images per merge batch (default 8)\n"
                 "  --no-opencl                   Disable OpenCL GPU acceleration (default enabled)\n";
    std::cerr << "\n";
    std::cerr << "Information options:\n"
                 "  --verbose                     Verbose output from steps\n"
                 "  --version                     Show application version number\n"
                 "  --opencv-version              Show OpenCV library version and build info\n";
    return 1;
  }

  // Output file options
  stack.set_inputs(options.get_filenames());
  stack.set_output(options.get_arg("--output", "output.jpg"));
  stack.set_depthmap(options.get_arg("--depthmap", ""));
  stack.set_3dview(options.get_arg("--3dview", ""));
  stack.set_jpgquality(std::stoi(options.get_arg("--jpgquality", "95")));
  stack.set_save_steps(options.has_flag("--save-steps"));

  // Image alignment options
  int flags = FocusStack::ALIGN_DEFAULT;
  if (options.has_flag("--global-align"))             flags |= FocusStack::ALIGN_GLOBAL;
  if (options.has_flag("--full-resolution-align"))    flags |= FocusStack::ALIGN_FULL_RESOLUTION;
  if (options.has_flag("--no-whitebalance"))          flags |= FocusStack::ALIGN_NO_WHITEBALANCE;
  if (options.has_flag("--no-contrast"))              flags |= FocusStack::ALIGN_NO_CONTRAST;
  stack.set_align_flags(flags);

  if (options.has_flag("--reference"))
  {
    stack.set_reference(std::stoi(options.get_arg("--reference")));
  }

  if (options.has_flag("--align-only"))
  {
    stack.set_align_only(true);
    stack.set_output(options.get_arg("--output", "aligned_"));
  }

  // Image merge options
  stack.set_consistency(std::stoi(options.get_arg("--consistency", "2")));
  stack.set_denoise(std::stof(options.get_arg("--denoise", "1.0")));

  // Depth map generation options
  stack.set_depthmap_smoothing(std::stof(options.get_arg("--depthmap-smoothing", "0.02")));
  stack.set_3dviewpoint(options.get_arg("--3dviewpoint", "0.5:1.0:0.5:2.0"));

  // Performance options
  if (options.has_flag("--threads"))
  {
    stack.set_threads(std::stoi(options.get_arg("--threads")));
  }

  if (options.has_flag("--batchsize"))
  {
    stack.set_batchsize(std::stoi(options.get_arg("--batchsize")));
  }

  stack.set_disable_opencl(options.has_flag("--no-opencl"));

  // Information options (some are handled at beginning of this function)
  stack.set_verbose(options.has_flag("--verbose"));

  // Check for any unhandled options
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

  if (stack.get_depthmap() != "")
  {
    std::printf("\rSaved depthmap to %s\n", stack.get_depthmap().c_str());
  }

  if (stack.get_3dview() != "")
  {
    std::printf("\rSaved 3D preview to %s\n", stack.get_3dview().c_str());
  }

  return 0;
}
