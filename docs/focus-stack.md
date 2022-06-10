focus-stack - Combine photos to create extended depth of field image
====================================================================

## SYNOPSIS

`focus-stack [options ...] file1.jpg file2.jpg ...`

## DESCRIPTION

**focus-stack** takes a set of photos taken at different focus settings and
combines them into one, selecting the sharpest source image for each
pixel position. This is commonly called extended depth of field, or
focus stacking. Typical use is for microscope photography where it is
difficult to obtain large enough depth of field to cover the whole
subject.

Usually good results are obtained with the default settings, but
following options are available:

### Output file options

  * `--output`=output.jpg:
    Set output filename. If file already exists, it will be overwritten.
    Format is decided by file extension. The formats currently supported
    by OpenCV are .bmp, .jpg, .jp2, .png, .webp, .ppm, .pfm, .sr, .tif,
    .exr and .hdr.

  * `--depthmap`=depthmap.png:
    Generate depthmap image, which shows the depth layers determined
    from image stack. The brightness will be scaled from 0 to 255,
    where 0 is the first image given on command line and 255 is the
    last image.

  * `--3dview`=3dview.png:
    Based on depth map, generate a 3-dimensional preview image.

  * `--save-steps`:
    Save intermediate images from processing steps. This includes the
    aligned images and the final grayscale image before color
    reassignment.

  * `--jpgquality`=95:
    Set the level of quality of the JPEG files (final and intermediates
    if asked to be written). The value can go from 0 to 100 with a
    default at 95.

  * `--nocrop`:
    Keep the full size of input images. By default images are cropped
    to the area that is valid for all images in the input stack, to
    avoid distortion near the edges.

### Image alignment options

  * `--reference`=index:
    Select image index (starting from 0) to use as alignment reference.
    Images in a stack will usually vary in scale and position. All other
    images are resized and rotated to match with this image. This also
    determines the scale of the output image, which is important for
    performing measurements. By default middle image of the stack is
    used as reference.

  * `--global-align`:
    By default each image is aligned against its neighbour. This
    improves results in deep stacks, as blur at the extreme focus levels
    can make direct alignment to reference inaccurate. However, if the
    images given as argument are not in correct order, or if some of
    them are of poorer quality, this option can be specified to align
    directly against the reference image.

  * `--full-resolution-align`:
    By default the resolution of images used in alignment step is
    limited to 2048x2048 pixels. This improves performance, and because
    sub-pixel accuracy is used in computing the gradients, higher
    resolution rarely improves results. Specifying this option will
    force the use of full resolution images in alignment.

  * `--no-whitebalance`:
    The application tries to compensate for any white balance
    differences between photos automatically. If camera white balance is
    set manually, this option can be specified to skip the unnecessary
    white balance adjustment.

  * `--no-contrast`:
    If camera exposure is manually controlled, this option can be used
    to skip unnecessary exposure compensation.

  * `--align-only`:
    Only align the image stack and exit. Useful for further processing
    of the aligned images with external tools.

  * `--align-keep-size`:
    Keep original image size by not cropping alignment borders. The
    wavelet processing borders still get cropped, unlike with --nocrop.

### Image merge options

* `--consistency`=level:
  Set the level of consistency filter applied to the depth map, in
  range 0 to 2. Higher level reduces artefacts and noise in output
  image, but can also result in removing small objects that appear
  against a textured background. By default the highest filter level 2
  is used.

* `--denoise`=level:
  Set level of wavelet-based denoise filter applied to the result
  image. Because focus stacking selects the largest difference from
  source images, it has a tendency to increase noise. The denoising
  step reduces all wavelet values by this amount, which corresponds
  directly to pixel values. The default value of 1.0 removes noise
  that is on the order of +- 1 pixel value.

### Depth map generation options
* `--depthmap-threshold`=level:
  Minimum contrast in input image for accepting as data point for
  depth map building. Range 0-255, default 10.

* `--depthmap-smooth-xy`=level:
  Smoothing of depthmap in image plane. Value is radius in pixels.

* `--depthmap-smooth-z`=level:
  Smoothing of depthmap in depth direction. Value is in 0-255 units.

* `--remove-bg`=threshold:
  Add alpha channel to depthmap and remove constant colored background.
  Threshold is positive for black background, negative for white background.

* `--halo-radius`=level:
  Reduce halo effects in depthmap near sharp contrast edges.
  Value is radius in pixels.

* `--3dviewpoint`=x:y:z:zscale:
  Viewpoint used for the 3D preview. Specifies the x, y and z coordinates
  of the camera and the z scaling of the depthmap values.

### Performance options

* `--threads`=count:
  Set the number of parallel threads in use. By default uses the
  number of CPU cores detected plus one to feed possible GPU
  accelerator. Lower number of threads also reduces memory
  consumption.

* `--batchsize`=count:
  Set the batch size for image merging. Larger values may give
  slightly better performance on machines with large amount of memory,
  while smaller values reduce memory usage.
  Currently default value is 8 and maximum value is 32.

* `--no-opencl`:
  By default OpenCL-based GPU acceleration is used if available. This
  option can be specified to disable it.

* `--wait-images`=seconds:
  Wait for given time if any image files are missing. Specifying this
  option allows to start processing before all image files have been
  captured from camera.

### Information options

* `--verbose`:
  Report each step as it begins and ends, and also the alignment
  parameters and other detailed information.

* `--version`:
  Show application version number.

* `--opencv-version`:
  Show OpenCV library build information.

## EXAMPLES

* `focus-stack IMG*.JPG`:
  Combine all images in current directory, and write output to `output.jpg`

* `focus-stack --verbose --output=stacked.png IMG*.JPG`:
  Combine all images, giving detailed printout of steps and write output
  to `stacked.png`

* `focus-stack --jpgquality=100 IMG*.JPG`:
  Generate a JPEG with the maximum quality level.

## GPU ACCELERATION

This application uses OpenCV library and its OpenCL acceleration
interface. The GPU used for acceleration can be selected by environment
variable **OPENCV\_OPENCL\_DEVICE** which takes a value such as
**Intel:GPU:0** See OpenCV documentation for details.

## REFERENCES

The algorithm used for combining images is described in **Complex
Wavelets for Extended Depth-of-Field: A New Method for the Fusion of
Multichannel Microscopy Images** by B. Forster, D. Van De Ville, J.
Berent, D. Sage and M. Unser.

## REPORTING BUGS

Bugs can be reported at
https://github.com/PetteriAimonen/focus-stack/issues
