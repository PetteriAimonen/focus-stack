#pragma once

#include <string>
#include <vector>
#include <memory>
#include <type_traits>
#include <opencv2/opencv.hpp>

namespace FocusStackSDK {

// Error codes for SDK operations
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

// Simple result type for C++14 compatibility
template<typename T>
class Result {
public:
    Result(const T& value) : has_value_(true), error_(ErrorCode::Success) {
        new (&storage_) T(value);
    }
    Result(T&& value) : has_value_(true), error_(ErrorCode::Success) {
        new (&storage_) T(std::move(value));
    }
    Result(ErrorCode error) : has_value_(false), error_(error) {}
    
    ~Result() {
        if (has_value_) {
            reinterpret_cast<T*>(&storage_)->~T();
        }
    }
    
    Result(const Result& other) : has_value_(other.has_value_), error_(other.error_) {
        if (has_value_) {
            new (&storage_) T(*other);
        }
    }
    
    Result& operator=(const Result& other) {
        if (this != &other) {
            if (has_value_) {
                reinterpret_cast<T*>(&storage_)->~T();
            }
            has_value_ = other.has_value_;
            error_ = other.error_;
            if (has_value_) {
                new (&storage_) T(*other);
            }
        }
        return *this;
    }
    
    bool has_value() const { return has_value_; }
    explicit operator bool() const { return has_value_; }
    
    const T& operator*() const { return *reinterpret_cast<const T*>(&storage_); }
    T& operator*() { return *reinterpret_cast<T*>(&storage_); }
    const T* operator->() const { return reinterpret_cast<const T*>(&storage_); }
    T* operator->() { return reinterpret_cast<T*>(&storage_); }
    
    ErrorCode error() const { return error_; }
    
private:
    bool has_value_;
    ErrorCode error_;
    typename std::aligned_storage<sizeof(T), alignof(T)>::type storage_;
};

class VoidResult {
public:
    VoidResult() : has_value_(true) {}
    VoidResult(ErrorCode error) : has_value_(false), error_(error) {}
    
    bool has_value() const { return has_value_; }
    explicit operator bool() const { return has_value_; }
    
    ErrorCode error() const { return error_; }
    
private:
    bool has_value_;
    ErrorCode error_;
};

// Processing progress information
struct ProcessingProgress {
    int totalTasks = 0;
    int completedTasks = 0;
    std::string currentTaskName;
    float progressPercentage = 0.0f;
    bool isCompleted = false;
    bool hasError = false;
    std::string errorMessage;
};

// Forward declarations
class FocusStackOptions;
class ThreadSafeFocusStacker;

// Configuration class for focus stacking options
class FocusStackOptions {
public:
    class Config;
    class Impl;

    FocusStackOptions();
    ~FocusStackOptions();

    // Copy and move constructors/operators
    FocusStackOptions(const FocusStackOptions& other);
    FocusStackOptions& operator=(const FocusStackOptions& other);
    FocusStackOptions(FocusStackOptions&& other) noexcept;
    FocusStackOptions& operator=(FocusStackOptions&& other) noexcept;

    // Basic output settings
    VoidResult setOutput(const std::string& path);
    VoidResult setDepthMap(const std::string& path);
    VoidResult setPreview3D(const std::string& path);

    // Quality and processing settings
    VoidResult setSaveIntermediateSteps(bool save);
    VoidResult setJpgQuality(int quality);
    VoidResult setNoCrop(bool noCrop);
    VoidResult setReferenceImageIndex(int index);

    // Alignment settings
    VoidResult setGlobalAlignment(bool global);
    VoidResult setFullResolutionAlignment(bool fullRes);
    VoidResult setNoWhiteBalance(bool noWB);
    VoidResult setNoContrast(bool noContrast);
    VoidResult setAlignOnly(bool alignOnly);
    VoidResult setAlignKeepSize(bool keepSize);

    // Advanced processing settings
    VoidResult setConsistencyLevel(int level);
    VoidResult setDenoiseLevel(float level);

    // Depth map settings
    VoidResult setDepthMapThreshold(int threshold);
    VoidResult setDepthMapSmoothingXY(float smoothing);
    VoidResult setDepthMapSmoothingZ(float smoothing);

    // Background and effects
    VoidResult setBackgroundRemoval(int level);
    VoidResult setHaloRadius(float radius);

    // 3D view settings
    VoidResult setViewpoint(const std::string& viewpoint);

    // Performance settings
    VoidResult setThreadCount(int threads);
    VoidResult setBatchSize(int batchSize);
    VoidResult setOpenCL(bool enable);
    VoidResult setWaitImages(float seconds);

    // Debug settings
    VoidResult setVerbose(bool verbose);

    // Build final configuration
    Result<Config> build() const;

    // Configuration class that holds validated settings
    class Config {
    public:
        Config(const Impl& impl);
        ~Config();
        
        // Copy and move constructors/operators
        Config(const Config& other);
        Config& operator=(const Config& other);
        Config(Config&& other) noexcept;
        Config& operator=(Config&& other) noexcept;

        // Internal access for implementation
        const Impl& getImpl() const;

    private:
        std::unique_ptr<Impl> m_impl;
    };

private:
    std::unique_ptr<Impl> m_impl;
};

// Thread-safe focus stacker class
class ThreadSafeFocusStacker {
public:
    class Impl;

    ThreadSafeFocusStacker();
    ~ThreadSafeFocusStacker();

    // Non-copyable but movable
    ThreadSafeFocusStacker(const ThreadSafeFocusStacker&) = delete;
    ThreadSafeFocusStacker& operator=(const ThreadSafeFocusStacker&) = delete;
    ThreadSafeFocusStacker(ThreadSafeFocusStacker&& other) noexcept;
    ThreadSafeFocusStacker& operator=(ThreadSafeFocusStacker&& other) noexcept;

    // Batch processing interface
    VoidResult processImages(
        const std::vector<std::string>& imagePaths,
        const FocusStackOptions::Config& config);

    // Streaming interface
    VoidResult startProcessing(const FocusStackOptions::Config& config);
    VoidResult addImage(const std::string& imagePath);
    VoidResult addImage(const cv::Mat& image);
    VoidResult finishProcessing();

    // Status and control
    ProcessingProgress getStatus() const;
    VoidResult waitForCompletion(int timeoutMs = -1);

    // Result access
    Result<cv::Mat> getResultImage() const;
    Result<cv::Mat> getDepthMap() const;
    Result<cv::Mat> get3DPreview() const;
    Result<cv::Mat> getForegroundMask() const;

    // Result saving
    VoidResult saveResult(const std::string& path) const;
    VoidResult saveDepthMap(const std::string& path) const;
    VoidResult save3DPreview(const std::string& path) const;

    // Special operations
    VoidResult alignImagesOnly(
        const std::vector<std::string>& imagePaths,
        const std::string& outputPrefix,
        const FocusStackOptions::Config& config);

    // Regeneration functions
    VoidResult regenerateDepthMap();
    VoidResult regenerate3DPreview();

    // Reset and cleanup
    void reset(bool keepResults = false);

private:
    std::unique_ptr<Impl> m_impl;
};

// Easy-to-use interface for simple cases
class EasyFocusStacker {
public:
    // Simple processing with default settings
    static VoidResult processWithDefaults(
        const std::vector<std::string>& imagePaths,
        const std::string& outputPath);

    // Processing with custom options
    static VoidResult processWithOptions(
        const std::vector<std::string>& imagePaths,
        const FocusStackOptions::Config& config);
};

} // namespace FocusStackSDK