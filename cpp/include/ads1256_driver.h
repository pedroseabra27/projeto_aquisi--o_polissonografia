#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <functional>

namespace acq {

struct DriverConfig {
    uint32_t sample_rate_hz {1000};
    uint32_t channels {8};
};

// Callback recebe vetor intercalado [c0,c1,...,cN-1,c0,c1,...]
using SampleBatchCallback = std::function<void(const std::vector<int32_t>&, uint64_t timestamp_ns)>;

class IAdcDriver {
public:
    virtual ~IAdcDriver() = default;
    virtual bool init(const DriverConfig&) = 0;
    virtual bool start(SampleBatchCallback cb, uint32_t batch_samples_per_channel) = 0;
    virtual void stop() = 0;
};

// Implementação simulada (seno + ruído) quando ADS1256_SIMULATOR está ativo.
class SimulatedAds1256 : public IAdcDriver {
public:
    bool init(const DriverConfig& cfg) override;
    bool start(SampleBatchCallback cb, uint32_t batch_samples_per_channel) override;
    void stop() override;
private:
    DriverConfig cfg_;
    bool running_ = false;
};

std::unique_ptr<IAdcDriver> create_driver();

} // namespace acq
