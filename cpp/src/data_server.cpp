#include "frame_protocol.h"
#include "ads1256_driver.h"
#include <thread>
#include <vector>
#include <atomic>
#include <iostream>
#include <cstring>
#include <csignal>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace acq {

class DataServer {
public:
    DataServer(uint16_t port) : port_(port) {}
    bool start() {
        listener_ = ::socket(AF_INET, SOCK_STREAM, 0);
        if (listener_ < 0) { perror("socket"); return false; }
        int opt=1; setsockopt(listener_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        sockaddr_in addr{}; addr.sin_family=AF_INET; addr.sin_addr.s_addr=INADDR_ANY; addr.sin_port=htons(port_);
        if (bind(listener_, (sockaddr*)&addr, sizeof(addr))<0){ perror("bind"); return false; }
        if (listen(listener_,1)<0){ perror("listen"); return false; }
        std::thread([this]{ accept_loop(); }).detach();
        return true;
    }
    void broadcast_frame(const FrameHeader& hdr, const std::vector<int32_t>& samples) {
        if (client_ < 0) return;
        // Enviar header + payload
        if (!send_all((const char*)&hdr, sizeof(hdr))) { close_client(); return; }
        if (!send_all((const char*)samples.data(), samples.size()*sizeof(int32_t))) { close_client(); return; }
    }
private:
    bool send_all(const char* data, size_t len) {
        size_t sent=0; while (sent < len) {
            ssize_t n = ::send(client_, data+sent, len-sent, MSG_NOSIGNAL);
            if (n<=0) return false; sent += (size_t)n;
        } return true;
    }
    void accept_loop() {
        while (true) {
            sockaddr_in caddr{}; socklen_t clen=sizeof(caddr);
            int c = ::accept(listener_, (sockaddr*)&caddr, &clen);
            if (c<0) { perror("accept"); continue; }
            close_client();
            client_ = c;
            std::cerr << "[DataServer] Cliente conectado." << std::endl;
        }
    }
    void close_client() { if (client_>=0) { ::close(client_); client_=-1; } }
    int listener_ = -1;
    std::atomic<int> client_{-1};
    uint16_t port_;
};

// Expor função factory simples para main
std::unique_ptr<DataServer> make_server(uint16_t port) {
    return std::make_unique<DataServer>(port);
}

} // namespace acq
