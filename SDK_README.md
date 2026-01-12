# Focus Stack SDK

This SDK provides a modern C++ interface for the focus-stack library, offering both simple and advanced APIs for focus stacking operations.

## Features

- **Type-safe configuration**: Builder pattern with compile-time validation
- **Thread-safe operations**: Safe for use in multi-threaded applications
- **Progress monitoring**: Real-time progress updates and status information
- **Memory management**: Automatic resource management with RAII
- **Error handling**: Comprehensive error reporting with specific error codes
- **Multiple interfaces**: From simple one-line calls to advanced streaming APIs

## Quick Start

### Simple Usage

```cpp
#include "FocusStackSDK.h"

// Process images with default settings
std::vector<std::string> images = {"img1.jpg", "img2.jpg", "img3.jpg"};
auto result = FocusStackSDK::EasyFocusStacker::processWithDefaults(images, "output.jpg");
if (!result) {
    std::cerr << "Processing failed: " << static_cast<int>(result.error()) << std::endl;
}
```

### Custom Configuration

```cpp
#include "FocusStackSDK.h"

// Configure options
FocusStackSDK::FocusStackOptions options;
auto configResult = options.setOutput("result.jpg")
    .has_value() ? options.setJpgQuality(95) : options.setJpgQuality(95);

if (!configResult) {
    // Handle configuration error
    return;
}

// Set additional options
options.setDepthMap("depth.png");
options.setPreview3D("preview.jpg");
options.setConsistencyLevel(3);
options.setDenoiseLevel(1.5f);

// Build configuration
auto config = options.build();
if (!config) {
    // Handle build error
    return;
}

// Process with custom settings
std::vector<std::string> images = {"img1.jpg", "img2.jpg", "img3.jpg"};
auto result = FocusStackSDK::EasyFocusStacker::processWithOptions(images, *config);
```

### Advanced Usage with Progress Monitoring

```cpp
#include "FocusStackSDK.h"

FocusStackSDK::ThreadSafeFocusStacker stacker;

// Start processing
auto processResult = stacker.processImages(images, config);
if (!processResult) {
    // Handle error
    return;
}

// Monitor progress
while (true) {
    auto progress = stacker.getStatus();
    std::cout << "Progress: " << progress.progressPercentage << "%" << std::endl;
    
    if (progress.isCompleted) {
        if (progress.hasError) {
            std::cerr << "Error: " << progress.errorMessage << std::endl;
            return;
        }
        break;
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// Access results
auto resultImage = stacker.getResultImage();
auto depthMap = stacker.getDepthMap();
auto preview3D = stacker.get3DPreview();
```

### Streaming Interface

```cpp
FocusStackSDK::ThreadSafeFocusStacker stacker;

// Start processing
stacker.startProcessing(config);

// Add images as they become available
stacker.addImage("image1.jpg");
stacker.addImage("image2.jpg");
stacker.addImage("image3.jpg");

// Finish processing
stacker.finishProcessing();

// Wait for completion
auto waitResult = stacker.waitForCompletion(30000); // 30 second timeout
if (!waitResult) {
    // Handle timeout or error
}
```

## Configuration Options

### Basic Settings
- `setOutput(path)`: Set output file path
- `setDepthMap(path)`: Enable depth map generation
- `setPreview3D(path)`: Enable 3D preview generation
- `setJpgQuality(quality)`: Set JPEG quality (0-100)
- `setSaveIntermediateSteps(bool)`: Save intermediate processing steps

### Alignment Settings
- `setGlobalAlignment(bool)`: Use global alignment
- `setFullResolutionAlignment(bool)`: Use full resolution for alignment
- `setNoWhiteBalance(bool)`: Disable white balance correction
- `setNoContrast(bool)`: Disable contrast adjustment
- `setAlignOnly(bool)`: Only perform alignment, no stacking
- `setAlignKeepSize(bool)`: Keep original image size during alignment

### Processing Settings
- `setConsistencyLevel(level)`: Set consistency level (0-10)
- `setDenoiseLevel(level)`: Set denoise level (0.0+)
- `setReferenceImageIndex(index)`: Set reference image index

### Depth Map Settings
- `setDepthMapThreshold(threshold)`: Set depth map threshold
- `setDepthMapSmoothingXY(smoothing)`: Set XY smoothing
- `setDepthMapSmoothingZ(smoothing)`: Set Z smoothing

### Performance Settings
- `setThreadCount(threads)`: Set number of threads (-1 for auto)
- `setBatchSize(size)`: Set batch size for processing
- `setOpenCL(bool)`: Enable/disable OpenCL acceleration

## Error Handling

The SDK uses a Result type for error handling:

```cpp
enum class ErrorCode {
    Success = 0,
    InvalidConfiguration,
    FileNotFound,
    ProcessingFailed,
    TimeoutError,
    MemoryError,
    OpenCLError,
    InvalidInput,
    NotInitialized
};
```

All operations return either a `Result<T>` or `VoidResult` that can be checked:

```cpp
auto result = stacker.processImages(images, config);
if (!result) {
    switch (result.error()) {
        case ErrorCode::InvalidConfiguration:
            std::cerr << "Invalid configuration" << std::endl;
            break;
        case ErrorCode::FileNotFound:
            std::cerr << "Input file not found" << std::endl;
            break;
        // ... handle other errors
    }
}
```

## Building

The SDK is automatically included when building the focus-stack project:

```bash
mkdir build
cd build
cmake ..
make
```

This will build:
- `focus-stack`: Original command-line tool
- `sdk_example`: SDK usage example

## Thread Safety

- `FocusStackOptions`: Thread-safe for reading after construction
- `ThreadSafeFocusStacker`: Thread-safe for all operations
- `EasyFocusStacker`: Thread-safe (static methods)

## Memory Management

The SDK uses RAII and smart pointers for automatic memory management. All returned cv::Mat objects are cloned copies that can be safely used after the stacker is destroyed.

## Examples

See `examples/sdk_example.cc` for complete usage examples.