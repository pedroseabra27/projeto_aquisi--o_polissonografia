// DataServer implementation
#include "data_server.h"
#include <thread>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>

namespace acq {

DataServer::DataServer(uint16_t port) : port_(port) {}

bool DataServer::start() {
    listener_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listener_ < 0) { perror("socket"); return false; }
    int opt=1; setsockopt(listener_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    sockaddr_in addr{}; addr.sin_family=AF_INET; addr.sin_addr.s_addr=INADDR_ANY; addr.sin_port=htons(port_);
    if (bind(listener_, (sockaddr*)&addr, sizeof(addr))<0){ perror("bind"); return false; }
    if (listen(listener_,1)<0){ perror("listen"); return false; }
    std::thread([this]{ accept_loop(); }).detach();
    return true;
}

void DataServer::broadcast_frame(const FrameHeader& hdr, const std::vector<int32_t>& samples) {
    if (client_ < 0) return;
    if (!send_all(reinterpret_cast<const char*>(&hdr), sizeof(hdr))) { close_client(); return; }
    if (!send_all(reinterpret_cast<const char*>(samples.data()), samples.size()*sizeof(int32_t))) { close_client(); return; }
}

bool DataServer::send_all(const char* data, size_t len) {
    size_t sent=0; while (sent < len) {
        ssize_t n = ::send(client_, data+sent, len-sent, MSG_NOSIGNAL);
        if (n<=0) return false; sent += static_cast<size_t>(n);
    } return true;
}

void DataServer::accept_loop() {
    while (true) {
        sockaddr_in caddr{}; socklen_t clen=sizeof(caddr);
        int c = ::accept(listener_, reinterpret_cast<sockaddr*>(&caddr), &clen);
        if (c<0) { perror("accept"); continue; }
        close_client();
        client_ = c;
        std::cerr << "[DataServer] Cliente conectado." << std::endl;
    }
}

void DataServer::close_client() { if (client_>=0) { ::close(client_); client_=-1; } }

std::unique_ptr<DataServer> make_server(uint16_t port) { return std::make_unique<DataServer>(port); }

} // namespace acq

