Fast and easy focus stacking
============================

This project implements a cross-platform tool for focus stacking images.
The application takes a set of images captured at different focus distances
and combines them so that the complete subject is in focus.

Installation
------------
Binary packages for Windows 10, Ubuntu 20.04 and Mac OS X are available on the
[GitHub releases tab](https://github.com/PetteriAimonen/focus-stack/releases).

Note that Mac OS X may end up putting the application to quarantine because
it is unsigned. This gives misleading error message "focus-stack.app” is damaged and can’t be opened. You should move it to the Trash."
You can try this command to remove the protection:

    xattr -d com.apple.quarantine focus-stack.app

Basic usage
-----------
In most cases just passing all the images is enough:

     build/focus-stack .../path/to/input/*.jpg

For advanced usage, see `--help` for list of all options or [check the manual](docs/focus-stack.md):

    Usage: build/focus-stack [options] file1.jpg file2.jpg ...

    Output file options:
      --output=output.jpg           Set output filename
      --depthmap=depthmap.png       Write a depth map image (default disabled)
      --3dview=3dview.png           Write a 3D preview image (default disabled)
      --save-steps                  Save intermediate images from processing steps
      --jpgquality=95               Quality for saving in JPG format (0-100, default 95)
      --nocrop                      Save full image, including borders with partial data

    Image alignment options:
      --reference=0                 Set index of image used as alignment reference (default middle one)
      --global-align                Align directly against reference (default with neighbour image)
      --full-resolution-align       Use full resolution images in alignment (default max 2048 px)
      --no-whitebalance             Don't attempt to correct white balance differences
      --no-contrast                 Don't attempt to correct contrast and exposure differences
      --align-only                  Only align the input image stack and exit
      --align-keep-size             Keep original image size by not cropping alignment borders

    Image merge options:
      --consistency=2               Neighbour pixel consistency filter level 0..2 (default 2)
      --denoise=1.0                 Merged image denoise level (default 1.0)

    Depth map generation options:
      --depthmap-threshold=10       Threshold to accept depth points (0-255, default 10)
      --depthmap-smooth-xy=20       Smoothing of depthmap in X and Y directions (default 20)
      --depthmap-smooth-z=40        Smoothing of depthmap in Z direction (default 40)
      --remove-bg=0                 Positive value removes black background, negative white
      --halo-radius=20              Radius of halo effects to remove from depthmap
      --3dviewpoint=x:y:z:zscale    Viewpoint for 3D view (default 1:1:1:2)

    Performance options:
      --threads=2                   Select number of threads to use (default number of CPUs + 1)
      --batchsize=8                 Images per merge batch (default 8)
      --no-opencl                   Disable OpenCL GPU acceleration (default enabled)
      --wait-images=0.0             Wait for image files to appear (allows simultaneous capture and processing)

    Information options:
      --verbose                     Verbose output from steps
      --version                     Show application version number
      --opencv-version              Show OpenCV library version and build info

On Windows you can additionally just select the photos and drag them
over `focus-stack.exe` to run with default settings.

On Mac OS X, you can start the program directly and it will ask for files.
Alternatively you can drag the files over `focus-stack` application.
If you want to provide command line parameters, open terminal and call the
binary with path `focus-stack.app/Contents/MacOS/focus-stack`.

Algorithms used
---------------
The focus stacking algorithm used was invented and first described in
[Complex Wavelets for Extended Depth-of-Field: A New Method for the Fusion of Multichannel Microscopy Images](http://bigwww.epfl.ch/publications/forster0404.html) by B. Forster, D. Van De Ville, J. Berent, D. Sage and M. Unser.

The application also uses multiple algorithms from OpenCV library.
Most importantly, [findTransformECC](https://docs.opencv.org/3.0-beta/modules/video/doc/motion_analysis_and_object_tracking.html#findtransformecc) is used to align
the source images.

For more information, see the [detailed explanation of algorithms](docs/Algorithms.md).

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

    sudo apt install debhelper devscripts
    make builddeb
    sudo dpkg -i DEBUILD/focus-stack*.deb

Building on Windows
-------------------
Download [OpenCV binary package](https://opencv.org/releases/) for Windows from OpenCV website.
Download [Build Tools for Visual Studio 2019](https://visualstudio.microsoft.com/downloads/) from Microsoft.

In Visual Studio command prompt, execute:

    nmake -f Makefile.windows

Building on Mac OS X
--------------------
Install [Homebrew](https://brew.sh/) by running this in terminal:

    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install.sh)"

Then install build dependencies:

    brew install opencv pkg-config dylibbundler

Finally, build and install to `/usr/local/bin`:

    make
    make install

Alternatively you can build an application bundle that can be used on any machine
and that also includes a simple GUI for selecting the files:

    make build/focus-stack.app

