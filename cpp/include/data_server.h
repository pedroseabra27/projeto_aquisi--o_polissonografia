#pragma once
#include <cstdint>
#include <vector>
#include "frame_protocol.h"

namespace acq {

class DataServer {
public:
    explicit DataServer(uint16_t port);
    bool start();
    void broadcast_frame(const FrameHeader& hdr, const std::vector<int32_t>& samples);
private:
    bool send_all(const char* data, size_t len);
    void accept_loop();
    void close_client();
    int listener_ = -1;
    std::atomic<int> client_{-1};
    uint16_t port_;
};

std::unique_ptr<DataServer> make_server(uint16_t port);

} // namespace acq
