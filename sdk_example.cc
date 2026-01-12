#include "src/FocusStackSDK.h"
#include <iostream>
#include <vector>

using namespace FocusStackSDK;

int main() {
    std::cout << "Focus Stack SDK Example" << std::endl;
    
    // Example 1: Simple usage with defaults
    std::vector<std::string> imagePaths = {
        "examples/pcb/pcb_001.jpg", 
        "examples/pcb/pcb_002.jpg", 
        "examples/pcb/pcb_003.jpg"
    };
    
    std::cout << "\n1. Simple processing with defaults..." << std::endl;
    auto result = EasyFocusStacker::processWithDefaults(imagePaths, "sdk_output.jpg");
    if (result) {
        std::cout << "✓ Simple processing completed successfully!" << std::endl;
    } else {
        std::cerr << "✗ Simple processing failed: " << static_cast<int>(result.error()) << std::endl;
        return 1;
    }
    
    // Example 2: Advanced usage with custom options
    std::cout << "\n2. Advanced processing with custom options..." << std::endl;
    FocusStackOptions options;
    auto outputResult = options.setOutput("sdk_advanced_output.jpg");
    auto qualityResult = options.setJpgQuality(90);
    auto verboseResult = options.setVerbose(false);  // Quiet mode for cleaner output
    
    if (!outputResult || !qualityResult || !verboseResult) {
        std::cerr << "Failed to configure options" << std::endl;
        return 1;
    }
    
    auto config = options.build();
    if (!config) {
        std::cerr << "Failed to build config: " << static_cast<int>(config.error()) << std::endl;
        return 1;
    }
    
    ThreadSafeFocusStacker stacker;
    auto processResult = stacker.processImages(imagePaths, *config);
    if (processResult) {
        std::cout << "✓ Advanced processing completed!" << std::endl;
        
        // Get processing status
        auto status = stacker.getStatus();
        std::cout << "  Final status: " << status.progressPercentage << "% complete" << std::endl;
        
        // Get result image
        auto resultImage = stacker.getResultImage();
        if (resultImage) {
            std::cout << "  Result image size: " << resultImage->rows << "x" << resultImage->cols << std::endl;
        }
        
        // Save result with different name
        auto saveResult = stacker.saveResult("sdk_saved_result.jpg");
        if (saveResult) {
            std::cout << "  ✓ Result saved successfully" << std::endl;
        }
    } else {
        std::cerr << "✗ Advanced processing failed: " << static_cast<int>(processResult.error()) << std::endl;
        return 1;
    }
    
    std::cout << "\n✓ All SDK examples completed successfully!" << std::endl;
    return 0;
}