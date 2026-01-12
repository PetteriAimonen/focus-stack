#include "FocusStackSDK.h"
#include "focusstack.hh"
#include <mutex>
#include <condition_variable>
#include <thread>
#include <chrono>
#include <atomic>
#include <opencv2/imgcodecs.hpp>

namespace FocusStackSDK {

// Implementation class for FocusStackOptions
class FocusStackOptions::Impl {
public:
    std::string outputPath;
    std::string depthMapPath;
    std::string preview3DPath;
    bool saveIntermediateSteps = false;
    int jpegQuality = 95;
    bool noCrop = false;
    int referenceImageIndex = -1;
    bool globalAlignment = false;
    bool fullResolutionAlignment = false;
    bool noWhiteBalance = false;
    bool noContrast = false;
    bool alignOnly = false;
    bool alignKeepSize = false;
    int consistencyLevel = 2;
    float denoiseLevel = 1.0f;
    int depthMapThreshold = 10;
    float depthMapSmoothingXY = 20.0f;
    float depthMapSmoothingZ = 40.0f;
    int backgroundRemoval = 0;
    float haloRadius = 20.0f;
    std::string viewpoint = "1:1:1:2";
    int threadCount = -1;
    int batchSize = 8;
    bool openCL = true;
    float waitImages = 0.0f;
    bool verbose = false;
};

// FocusStackOptions implementation
FocusStackOptions::FocusStackOptions() : m_impl(std::make_unique<Impl>()) {}

FocusStackOptions::~FocusStackOptions() = default;

FocusStackOptions::FocusStackOptions(const FocusStackOptions& other) 
    : m_impl(std::make_unique<Impl>(*other.m_impl)) {}

FocusStackOptions& FocusStackOptions::operator=(const FocusStackOptions& other) {
    if (this != &other) {
        m_impl = std::make_unique<Impl>(*other.m_impl);
    }
    return *this;
}

FocusStackOptions::FocusStackOptions(FocusStackOptions&& other) noexcept 
    : m_impl(std::move(other.m_impl)) {}

FocusStackOptions& FocusStackOptions::operator=(FocusStackOptions&& other) noexcept {
    if (this != &other) {
        m_impl = std::move(other.m_impl);
    }
    return *this;
}

VoidResult FocusStackOptions::setOutput(const std::string& path) {
    if (path.empty()) {
        return ErrorCode::InvalidConfiguration;
    }
    m_impl->outputPath = path;
    return VoidResult();
}

VoidResult FocusStackOptions::setDepthMap(const std::string& path) {
    m_impl->depthMapPath = path;
    return VoidResult();
}

VoidResult FocusStackOptions::setPreview3D(const std::string& path) {
    m_impl->preview3DPath = path;
    return VoidResult();
}

VoidResult FocusStackOptions::setSaveIntermediateSteps(bool save) {
    m_impl->saveIntermediateSteps = save;
    return VoidResult();
}

VoidResult FocusStackOptions::setJpgQuality(int quality) {
    if (quality < 0 || quality > 100) {
        return ErrorCode::InvalidConfiguration;
    }
    m_impl->jpegQuality = quality;
    return VoidResult();
}

VoidResult FocusStackOptions::setNoCrop(bool noCrop) {
    m_impl->noCrop = noCrop;
    return VoidResult();
}

VoidResult FocusStackOptions::setReferenceImageIndex(int index) {
    m_impl->referenceImageIndex = index;
    return VoidResult();
}

VoidResult FocusStackOptions::setGlobalAlignment(bool global) {
    m_impl->globalAlignment = global;
    return VoidResult();
}

VoidResult FocusStackOptions::setFullResolutionAlignment(bool fullRes) {
    m_impl->fullResolutionAlignment = fullRes;
    return VoidResult();
}

VoidResult FocusStackOptions::setNoWhiteBalance(bool noWB) {
    m_impl->noWhiteBalance = noWB;
    return VoidResult();
}

VoidResult FocusStackOptions::setNoContrast(bool noContrast) {
    m_impl->noContrast = noContrast;
    return VoidResult();
}

VoidResult FocusStackOptions::setAlignOnly(bool alignOnly) {
    m_impl->alignOnly = alignOnly;
    return VoidResult();
}

VoidResult FocusStackOptions::setAlignKeepSize(bool keepSize) {
    m_impl->alignKeepSize = keepSize;
    return VoidResult();
}

VoidResult FocusStackOptions::setConsistencyLevel(int level) {
    if (level < 0 || level > 10) {
        return ErrorCode::InvalidConfiguration;
    }
    m_impl->consistencyLevel = level;
    return VoidResult();
}

VoidResult FocusStackOptions::setDenoiseLevel(float level) {
    if (level < 0.0f) {
        return ErrorCode::InvalidConfiguration;
    }
    m_impl->denoiseLevel = level;
    return VoidResult();
}

VoidResult FocusStackOptions::setDepthMapThreshold(int threshold) {
    if (threshold < 0) {
        return ErrorCode::InvalidConfiguration;
    }
    m_impl->depthMapThreshold = threshold;
    return VoidResult();
}

VoidResult FocusStackOptions::setDepthMapSmoothingXY(float smoothing) {
    if (smoothing < 0.0f) {
        return ErrorCode::InvalidConfiguration;
    }
    m_impl->depthMapSmoothingXY = smoothing;
    return VoidResult();
}

VoidResult FocusStackOptions::setDepthMapSmoothingZ(float smoothing) {
    if (smoothing < 0.0f) {
        return ErrorCode::InvalidConfiguration;
    }
    m_impl->depthMapSmoothingZ = smoothing;
    return VoidResult();
}

VoidResult FocusStackOptions::setBackgroundRemoval(int level) {
    if (level < 0) {
        return ErrorCode::InvalidConfiguration;
    }
    m_impl->backgroundRemoval = level;
    return VoidResult();
}

VoidResult FocusStackOptions::setHaloRadius(float radius) {
    if (radius < 0.0f) {
        return ErrorCode::InvalidConfiguration;
    }
    m_impl->haloRadius = radius;
    return VoidResult();
}

VoidResult FocusStackOptions::setViewpoint(const std::string& viewpoint) {
    if (viewpoint.empty()) {
        return ErrorCode::InvalidConfiguration;
    }
    m_impl->viewpoint = viewpoint;
    return VoidResult();
}

VoidResult FocusStackOptions::setThreadCount(int threads) {
    m_impl->threadCount = threads;
    return VoidResult();
}

VoidResult FocusStackOptions::setBatchSize(int batchSize) {
    if (batchSize <= 0) {
        return ErrorCode::InvalidConfiguration;
    }
    m_impl->batchSize = batchSize;
    return VoidResult();
}

VoidResult FocusStackOptions::setOpenCL(bool enable) {
    m_impl->openCL = enable;
    return VoidResult();
}

VoidResult FocusStackOptions::setWaitImages(float seconds) {
    if (seconds < 0.0f) {
        return ErrorCode::InvalidConfiguration;
    }
    m_impl->waitImages = seconds;
    return VoidResult();
}

VoidResult FocusStackOptions::setVerbose(bool verbose) {
    m_impl->verbose = verbose;
    return VoidResult();
}

Result<FocusStackOptions::Config> FocusStackOptions::build() const {
    // Validate configuration
    if (m_impl->outputPath.empty()) {
        return ErrorCode::InvalidConfiguration;
    }
    
    return Config(*m_impl);
}

// FocusStackOptions::Config implementation
FocusStackOptions::Config::Config(const Impl& impl) : m_impl(std::make_unique<Impl>(impl)) {}

FocusStackOptions::Config::~Config() = default;

FocusStackOptions::Config::Config(const Config& other) 
    : m_impl(std::make_unique<Impl>(*other.m_impl)) {}

FocusStackOptions::Config& FocusStackOptions::Config::operator=(const Config& other) {
    if (this != &other) {
        m_impl = std::make_unique<Impl>(*other.m_impl);
    }
    return *this;
}

FocusStackOptions::Config::Config(Config&& other) noexcept 
    : m_impl(std::move(other.m_impl)) {}

FocusStackOptions::Config& FocusStackOptions::Config::operator=(Config&& other) noexcept {
    if (this != &other) {
        m_impl = std::move(other.m_impl);
    }
    return *this;
}

const FocusStackOptions::Impl& FocusStackOptions::Config::getImpl() const {
    return *m_impl;
}

// Implementation class for ThreadSafeFocusStacker
class ThreadSafeFocusStacker::Impl {
public:
    std::unique_ptr<focusstack::FocusStack> focusStack;
    std::mutex mutex;
    std::condition_variable cv;
    std::atomic<bool> isProcessing{false};
    std::atomic<bool> isCompleted{false};
    std::atomic<bool> hasError{false};
    std::string errorMessage;
    ProcessingProgress progress;
    std::thread processingThread;

    Impl() : focusStack(std::make_unique<focusstack::FocusStack>()) {
        // Keep it simple for now - no custom logging callback
    }

    ~Impl() {
        if (processingThread.joinable()) {
            processingThread.join();
        }
    }

    void configureFocusStack(const FocusStackOptions::Config& config) {
        const auto& impl = config.getImpl();
        
        // Start with minimal configuration like the direct test
        focusStack->set_output(impl.outputPath);
        
        // Set verbose based on config
        focusStack->set_verbose(impl.verbose);
    }

    void updateProgress() {
        int total, completed;
        std::string taskName;
        focusStack->get_status(total, completed, taskName);
        
        std::lock_guard<std::mutex> lock(mutex);
        progress.totalTasks = total;
        progress.completedTasks = completed;
        progress.currentTaskName = taskName;
        progress.progressPercentage = total > 0 ? (float)completed / total * 100.0f : 0.0f;
        progress.isCompleted = isCompleted;
        progress.hasError = hasError;
        progress.errorMessage = errorMessage;
    }
};

// ThreadSafeFocusStacker implementation
ThreadSafeFocusStacker::ThreadSafeFocusStacker() : m_impl(std::make_unique<Impl>()) {}

ThreadSafeFocusStacker::~ThreadSafeFocusStacker() = default;

ThreadSafeFocusStacker::ThreadSafeFocusStacker(ThreadSafeFocusStacker&& other) noexcept 
    : m_impl(std::move(other.m_impl)) {}

ThreadSafeFocusStacker& ThreadSafeFocusStacker::operator=(ThreadSafeFocusStacker&& other) noexcept {
    if (this != &other) {
        m_impl = std::move(other.m_impl);
    }
    return *this;
}

VoidResult ThreadSafeFocusStacker::processImages(
    const std::vector<std::string>& imagePaths,
    const FocusStackOptions::Config& config) {
    
    if (imagePaths.empty()) {
        return ErrorCode::InvalidInput;
    }
    
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    
    if (m_impl->isProcessing) {
        return ErrorCode::ProcessingFailed;
    }
    
    try {
        m_impl->configureFocusStack(config);
        m_impl->focusStack->set_inputs(imagePaths);
        
        m_impl->isProcessing = true;
        m_impl->isCompleted = false;
        m_impl->hasError = false;
        m_impl->errorMessage.clear();
        
        // Run processing synchronously
        bool success = m_impl->focusStack->run();
        
        m_impl->isProcessing = false;
        m_impl->isCompleted = true;
        
        if (!success) {
            m_impl->hasError = true;
            m_impl->errorMessage = "Processing failed";
            return ErrorCode::ProcessingFailed;
        }
        
        return VoidResult();
    } catch (const std::exception& e) {
        m_impl->isProcessing = false;
        m_impl->hasError = true;
        m_impl->errorMessage = e.what();
        return ErrorCode::ProcessingFailed;
    }
}

VoidResult ThreadSafeFocusStacker::startProcessing(const FocusStackOptions::Config& config) {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    
    if (m_impl->isProcessing) {
        return ErrorCode::ProcessingFailed;
    }
    
    try {
        m_impl->configureFocusStack(config);
        m_impl->focusStack->start();
        m_impl->isProcessing = true;
        m_impl->isCompleted = false;
        m_impl->hasError = false;
        m_impl->errorMessage.clear();
        
        return VoidResult();
    } catch (const std::exception& e) {
        return ErrorCode::ProcessingFailed;
    }
}

VoidResult ThreadSafeFocusStacker::addImage(const std::string& imagePath) {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    
    if (!m_impl->isProcessing) {
        return ErrorCode::NotInitialized;
    }
    
    try {
        m_impl->focusStack->add_image(imagePath);
        return VoidResult();
    } catch (const std::exception& e) {
        return ErrorCode::ProcessingFailed;
    }
}

VoidResult ThreadSafeFocusStacker::addImage(const cv::Mat& image) {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    
    if (!m_impl->isProcessing) {
        return ErrorCode::NotInitialized;
    }
    
    try {
        m_impl->focusStack->add_image(image);
        return VoidResult();
    } catch (const std::exception& e) {
        return ErrorCode::ProcessingFailed;
    }
}

VoidResult ThreadSafeFocusStacker::finishProcessing() {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    
    if (!m_impl->isProcessing) {
        return ErrorCode::NotInitialized;
    }
    
    try {
        m_impl->focusStack->do_final_merge();
        return VoidResult();
    } catch (const std::exception& e) {
        return ErrorCode::ProcessingFailed;
    }
}

ProcessingProgress ThreadSafeFocusStacker::getStatus() const {
    m_impl->updateProgress();
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    return m_impl->progress;
}

VoidResult ThreadSafeFocusStacker::waitForCompletion(int timeoutMs) {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    
    // Since we're using synchronous processing, just check the status
    if (m_impl->hasError) {
        return ErrorCode::ProcessingFailed;
    }
    
    if (!m_impl->isCompleted) {
        return ErrorCode::NotInitialized;
    }
    
    return VoidResult();
}

Result<cv::Mat> ThreadSafeFocusStacker::getResultImage() const {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    
    if (!m_impl->isCompleted) {
        return ErrorCode::NotInitialized;
    }
    
    try {
        const cv::Mat& result = m_impl->focusStack->get_result_image();
        if (result.empty()) {
            return ErrorCode::ProcessingFailed;
        }
        return result.clone();
    } catch (const std::exception& e) {
        return ErrorCode::ProcessingFailed;
    }
}

Result<cv::Mat> ThreadSafeFocusStacker::getDepthMap() const {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    
    if (!m_impl->isCompleted) {
        return ErrorCode::NotInitialized;
    }
    
    try {
        const cv::Mat& result = m_impl->focusStack->get_result_depthmap();
        if (result.empty()) {
            return ErrorCode::ProcessingFailed;
        }
        return result.clone();
    } catch (const std::exception& e) {
        return ErrorCode::ProcessingFailed;
    }
}

Result<cv::Mat> ThreadSafeFocusStacker::get3DPreview() const {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    
    if (!m_impl->isCompleted) {
        return ErrorCode::NotInitialized;
    }
    
    try {
        const cv::Mat& result = m_impl->focusStack->get_result_3dview();
        if (result.empty()) {
            return ErrorCode::ProcessingFailed;
        }
        return result.clone();
    } catch (const std::exception& e) {
        return ErrorCode::ProcessingFailed;
    }
}

Result<cv::Mat> ThreadSafeFocusStacker::getForegroundMask() const {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    
    if (!m_impl->isCompleted) {
        return ErrorCode::NotInitialized;
    }
    
    try {
        const cv::Mat& result = m_impl->focusStack->get_result_mask();
        if (result.empty()) {
            return ErrorCode::ProcessingFailed;
        }
        return result.clone();
    } catch (const std::exception& e) {
        return ErrorCode::ProcessingFailed;
    }
}

VoidResult ThreadSafeFocusStacker::saveResult(const std::string& path) const {
    auto result = getResultImage();
    if (!result) {
        return result.error();
    }
    
    try {
        if (!cv::imwrite(path, *result)) {
            return ErrorCode::ProcessingFailed;
        }
        return VoidResult();
    } catch (const std::exception& e) {
        return ErrorCode::ProcessingFailed;
    }
}

VoidResult ThreadSafeFocusStacker::saveDepthMap(const std::string& path) const {
    auto result = getDepthMap();
    if (!result) {
        return result.error();
    }
    
    try {
        if (!cv::imwrite(path, *result)) {
            return ErrorCode::ProcessingFailed;
        }
        return VoidResult();
    } catch (const std::exception& e) {
        return ErrorCode::ProcessingFailed;
    }
}

VoidResult ThreadSafeFocusStacker::save3DPreview(const std::string& path) const {
    auto result = get3DPreview();
    if (!result) {
        return result.error();
    }
    
    try {
        if (!cv::imwrite(path, *result)) {
            return ErrorCode::ProcessingFailed;
        }
        return VoidResult();
    } catch (const std::exception& e) {
        return ErrorCode::ProcessingFailed;
    }
}

VoidResult ThreadSafeFocusStacker::alignImagesOnly(
    const std::vector<std::string>& imagePaths,
    const std::string& outputPrefix,
    const FocusStackOptions::Config& config) {
    
    if (imagePaths.empty()) {
        return ErrorCode::InvalidInput;
    }
    
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    
    if (m_impl->isProcessing) {
        return ErrorCode::ProcessingFailed;
    }
    
    // For now, return not implemented
    // The align-only functionality would require more complex implementation
    // to save individual aligned images with the given prefix
    (void)outputPrefix;  // Suppress unused parameter warning
    (void)config;
    return ErrorCode::NotInitialized;
}

VoidResult ThreadSafeFocusStacker::regenerateDepthMap() {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    
    if (!m_impl->isCompleted) {
        return ErrorCode::NotInitialized;
    }
    
    try {
        m_impl->focusStack->regenerate_depthmap();
        return VoidResult();
    } catch (const std::exception& e) {
        return ErrorCode::ProcessingFailed;
    }
}

VoidResult ThreadSafeFocusStacker::regenerate3DPreview() {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    
    if (!m_impl->isCompleted) {
        return ErrorCode::NotInitialized;
    }
    
    try {
        m_impl->focusStack->regenerate_3dview();
        return VoidResult();
    } catch (const std::exception& e) {
        return ErrorCode::ProcessingFailed;
    }
}

void ThreadSafeFocusStacker::reset(bool keepResults) {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    
    if (m_impl->processingThread.joinable()) {
        m_impl->processingThread.join();
    }
    
    m_impl->focusStack->reset(keepResults);
    m_impl->isProcessing = false;
    m_impl->isCompleted = false;
    m_impl->hasError = false;
    m_impl->errorMessage.clear();
    m_impl->progress = ProcessingProgress{};
}

// EasyFocusStacker implementation
VoidResult EasyFocusStacker::processWithDefaults(
    const std::vector<std::string>& imagePaths,
    const std::string& outputPath) {
    
    FocusStackOptions options;
    auto result = options.setOutput(outputPath);
    if (!result) {
        return result.error();
    }
    
    auto configResult = options.build();
    if (!configResult) {
        return configResult.error();
    }
    
    ThreadSafeFocusStacker stacker;
    auto processResult = stacker.processImages(imagePaths, *configResult);
    if (!processResult) {
        return processResult.error();
    }
    
    auto waitResult = stacker.waitForCompletion();
    if (!waitResult) {
        return waitResult.error();
    }
    
    return VoidResult();
}

VoidResult EasyFocusStacker::processWithOptions(
    const std::vector<std::string>& imagePaths,
    const FocusStackOptions::Config& config) {
    
    ThreadSafeFocusStacker stacker;
    auto processResult = stacker.processImages(imagePaths, config);
    if (!processResult) {
        return processResult.error();
    }
    
    auto waitResult = stacker.waitForCompletion();
    if (!waitResult) {
        return waitResult.error();
    }
    
    return VoidResult();
}

} // namespace FocusStackSDK