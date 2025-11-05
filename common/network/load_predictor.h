// Use of this source code is governed by a BSD 3-Clause License
// that can be found in the LICENSE file.

// Author: caozhiyi (caozhiyi5@gmail.com)

#ifndef COMMON_NETWORK_LOAD_PREDICTOR
#define COMMON_NETWORK_LOAD_PREDICTOR

#include <deque>
#include <atomic>
#include <mutex>

namespace cppnet {

class LoadPredictor {
public:
    LoadPredictor();
    ~LoadPredictor() = default;
    
    // Update with new load data
    void UpdateLoad(double load_score, uint64_t timestamp_ms);
    
    // Predict future load
    double PredictLoad(uint32_t future_time_ms = 300000) const; // Default: 5 minutes
    
    // Get current load statistics
    double GetCurrentLoad() const { return _current_load; }
    double GetAverageLoad() const { return _average_load; }
    double GetMaxLoad() const { return _max_load; }
    double GetMinLoad() const { return _min_load; }
    
    // Set prediction parameters
    void SetWindowSize(uint32_t size) { _window_size = size; }
    void SetSmoothingFactor(double alpha) { _smoothing_factor = alpha; }
    
private:
    // Load data point
    struct LoadPoint {
        double load_score;
        uint64_t timestamp_ms;
        
        LoadPoint(double load, uint64_t ts) : load_score(load), timestamp_ms(ts) {}
    };
    
    // Remove old data points outside the window
    void PruneOldData(uint64_t current_timestamp) const;
    
    // Calculate exponential moving average
    double CalculateEMA() const;
    
    // Calculate linear regression for trend prediction
    double CalculateLinearRegression(uint32_t future_time_ms) const;
    
    // Current load statistics
    std::atomic<double> _current_load{0.0};
    std::atomic<double> _average_load{0.0};
    std::atomic<double> _max_load{0.0};
    std::atomic<double> _min_load{0.0};
    
    // Sliding window parameters
    mutable std::mutex _window_mutex;
    std::deque<LoadPoint> _load_window;
    uint32_t _window_size{100}; // Default: 100 data points
    uint64_t _window_duration_ms{60000}; // Default: 1 minute window
    
    // Exponential smoothing parameters
    double _smoothing_factor{0.3}; // Default: 0.3 for EMA
    mutable double _last_ema{0.0};
    mutable bool _ema_initialized{false};
    
    // Linear regression parameters
    mutable double _slope{0.0};
    mutable double _intercept{0.0};
    mutable bool _regression_initialized{false};
};

} // namespace cppnet

#endif