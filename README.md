Fast and easy focus stacking
============================

This project implements a cross-platform tool for focus stacking images.
The application takes a set of images captured at different focus distances
and combines them so that the complete subject is in focus.

Installation
------------
Binary packages for Windows 10 and Ubuntu 18.04 are available on the GitHub
releases tab.

Building on Ubuntu
------------------
The only dependency is OpenCV, 3.0 or newer, plus the basic build tools:

    sudo apt install libopencv-dev build-essential

To enable GPU acceleration, you additionally need OpenCL library and
GPU-specific driver:

    sudo apt install ocl-icd-opencl-dev beignet            # For Intel GPUs
    sudo apt install ocl-icd-opencl-dev nvidia-opencl-icd  # For NVidia GPUs

To build and install the application, simply type:

    make
    make install

Or to build a Debian/Ubuntu package and install it, type:

    make builddeb
    sudo dpkg -i DEBUILD/focus-stack*.deb

Building on Windows
-------------------
Download OpenCV binary package for Windows from OpenCV website.
Download Build Tools for Visual Studio 2019 from Microsoft.

In Visual Studio command prompt, execute:

    nmake -f Makefile.windows

Basic usage
-----------
In most cases just passing all the images is enough:

     build/focus-stack .../path/to/input/*.jpg

For advanced usage, see `--help` for list of all options:

    Usage: build/focus-stack [options] file1.jpg file2.jpg ...
    Options:
      --output=output.jpg           Set output filename
      --reference=0                 Set index of image used as alignment reference (default middle one)
      --global-align                Align directly against reference (default with neighbour image)
      --full-resolution-align       Use full resolution images in alignment (default max 2048 px)
      --no-whitebalance             Don't attempt to correct white balance differences
      --no-contrast                 Don't attempt to correct contrast and exposure differences
      --threads=2                   Select number of threads to use (default number of CPUs + 1)
      --no-opencl                   Disable OpenCL GPU acceleration (default enabled)
      --consistency=2               Set depth map consistency filter level 0..2 (default 2)
      --denoise=1.0                 Set image denoise level (default 1.0)
      --save-steps                  Save intermediate images from processing steps
      --verbose                     Verbose output from steps
      --version                     Show application version number
      --opencv-version              Show OpenCV library version and build info

On Windows you can additionally just select the photos and drag them
over `focus-stack.exe` to run with default settings.

Algorithms used
---------------
The focus stacking algorithm used was invented and first described in
[Complex Wavelets for Extended Depth-of-Field: A New Method for the Fusion of Multichannel Microscopy Images](http://bigwww.epfl.ch/publications/forster0404.html) by B. Forster, D. Van De Ville, J. Berent, D. Sage and M. Unser.

The application also uses multiple algorithms from OpenCV library.
Most importantly, [findTransformECC](https://docs.opencv.org/3.0-beta/modules/video/doc/motion_analysis_and_object_tracking.html#findtransformecc) is used to align
the source images.


