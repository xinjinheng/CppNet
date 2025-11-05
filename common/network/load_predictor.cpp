// Use of this source code is governed by a BSD 3-Clause License
// that can be found in the LICENSE file.

// Author: caozhiyi (caozhiyi5@gmail.com)

#include "load_predictor.h"
#include <algorithm>
#include <numeric>
#include <cmath>

namespace cppnet {

LoadPredictor::LoadPredictor() {
}

void LoadPredictor::UpdateLoad(double load_score, uint64_t timestamp_ms) {
    // Update current load
    _current_load = load_score;
    
    // Add to sliding window
    { 
        std::unique_lock<std::mutex> lock(_window_mutex);
        
        // Remove old data first
        PruneOldData(timestamp_ms);
        
        // Add new data point
        _load_window.emplace_back(load_score, timestamp_ms);
        
        // Update max and min
        if (_load_window.size() == 1) {
            _max_load = _min_load = load_score;
        } else {
            if (load_score > _max_load) {
                _max_load = load_score;
            }
            if (load_score < _min_load) {
                _min_load = load_score;
            }
        }
        
        // Update average
        double sum = 0.0;
        for (const auto& point : _load_window) {
            sum += point.load_score;
        }
        _average_load = sum / _load_window.size();
        
        // Reset EMA and regression flags to force recalculation
        _ema_initialized = false;
        _regression_initialized = false;
    }
}

double LoadPredictor::PredictLoad(uint32_t future_time_ms) const {
    std::unique_lock<std::mutex> lock(_window_mutex);
    
    if (_load_window.empty()) {
        return 0.0;
    }
    
    // Get current timestamp from the latest data point
    uint64_t current_timestamp = _load_window.back().timestamp_ms;
    
    // Prune old data
    PruneOldData(current_timestamp);
    
    if (_load_window.size() < 2) {
        return _current_load;
    }
    
    // Calculate EMA for the current trend
    double ema = CalculateEMA();
    
    // Calculate linear regression for trend prediction
    double trend_prediction = CalculateLinearRegression(future_time_ms);
    
    // Combine EMA and linear regression for final prediction
    // Use a weighted average: 70% EMA, 30% trend prediction
    return 0.7 * ema + 0.3 * trend_prediction;
}

void LoadPredictor::PruneOldData(uint64_t current_timestamp) const {
    if (_load_window.empty()) {
        return;
    }
    
    uint64_t cutoff_time = current_timestamp - _window_duration_ms;
    
    // Remove all data points older than cutoff_time
    auto cutoff_iter = std::find_if(_load_window.begin(), _load_window.end(),
        [cutoff_time](const LoadPoint& point) {
            return point.timestamp_ms > cutoff_time;
        });
    
    if (cutoff_iter != _load_window.begin()) {
        _load_window.erase(_load_window.begin(), cutoff_iter);
    }
}

double LoadPredictor::CalculateEMA() const {
    if (_load_window.empty()) {
        return 0.0;
    }
    
    if (_ema_initialized) {
        return _last_ema;
    }
    
    double ema = _load_window[0].load_score;
    
    for (size_t i = 1; i < _load_window.size(); ++i) {
        ema = _smoothing_factor * _load_window[i].load_score + (1 - _smoothing_factor) * ema;
    }
    
    _last_ema = ema;
    _ema_initialized = true;
    
    return ema;
}

double LoadPredictor::CalculateLinearRegression(uint32_t future_time_ms) const {
    if (_load_window.size() < 2) {
        return _current_load;
    }
    
    if (_regression_initialized) {
        // Predict future load using precomputed slope and intercept
        return _intercept + _slope * future_time_ms;
    }
    
    size_t n = _load_window.size();
    
    // Calculate means
    double mean_x = 0.0;
    double mean_y = 0.0;
    
    for (const auto& point : _load_window) {
        mean_x += point.timestamp_ms;
        mean_y += point.load_score;
    }
    
    mean_x /= n;
    mean_y /= n;
    
    // Calculate slope and intercept
    double numerator = 0.0;
    double denominator = 0.0;
    
    for (const auto& point : _load_window) {
        double x_diff = point.timestamp_ms - mean_x;
        double y_diff = point.load_score - mean_y;
        
        numerator += x_diff * y_diff;
        denominator += x_diff * x_diff;
    }
    
    if (denominator == 0.0) {
        return mean_y;
    }
    
    _slope = numerator / denominator;
    _intercept = mean_y - _slope * mean_x;
    
    _regression_initialized = true;
    
    // Predict future load
    uint64_t future_timestamp = _load_window.back().timestamp_ms + future_time_ms;
    return _intercept + _slope * future_timestamp;
}

} // namespace cppnet