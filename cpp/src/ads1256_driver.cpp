#include "ads1256_driver.h"
#include "frame_protocol.h"
#include <thread>
#include <cmath>
#include <random>
#include <memory>

using namespace std::chrono_literals;

namespace acq {

bool SimulatedAds1256::init(const DriverConfig& cfg) {
    cfg_ = cfg;
    return true;
}

bool SimulatedAds1256::start(SampleBatchCallback cb, uint32_t batch_samples_per_channel) {
    if (running_) return false;
    running_ = true;
    std::thread([this, cb, batch_samples_per_channel]() {
        const double two_pi = 6.283185307179586;
        std::mt19937 rng{std::random_device{}()};
        std::normal_distribution<double> noise(0.0, 1000.0);
        double phase[32] = {0};
        double freq_base = 10.0; // Hz de exemplo
        const double dt = 1.0 / cfg_.sample_rate_hz;
        std::vector<int32_t> interleaved;
        interleaved.resize(cfg_.channels * batch_samples_per_channel);
        while (running_) {
            auto t0 = now_epoch_ns();
            for (uint32_t s=0; s<batch_samples_per_channel; ++s) {
                for (uint32_t c=0; c<cfg_.channels; ++c) {
                    double val = std::sin(phase[c]) * (500000.0 + c*1000.0) + noise(rng);
                    // 24-bit range simulated -> clamp
                    if (val > 8388607) val = 8388607;
                    if (val < -8388608) val = -8388608;
                    interleaved[s*cfg_.channels + c] = static_cast<int32_t>(val);
                    phase[c] += two_pi * (freq_base + c) * dt;
                }
            }
            cb(interleaved, t0);
            // Dorme aproximadamente batch_samples / Fs
            auto sleep_dur = std::chrono::duration<double>(batch_samples_per_channel * dt);
            std::this_thread::sleep_for(sleep_dur);
        }
    }).detach();
    return true;
}

void SimulatedAds1256::stop() { running_ = false; }

std::unique_ptr<IAdcDriver> create_driver() {
    return std::make_unique<SimulatedAds1256>();
}

} // namespace acq
