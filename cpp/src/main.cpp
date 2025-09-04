#include "ads1256_driver.h"
#include "frame_protocol.h"
#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <atomic>
#include <csignal>
#include <thread>

#include "data_server.h"

static std::atomic<bool> g_running{true};

static void signal_handler(int){ g_running=false; }

struct Options {
    uint32_t rate=1000;
    uint32_t channels=8;
    uint32_t batch=50; // amostras por canal por frame
    uint16_t port=5555;
};

Options parse_args(int argc, char** argv) {
    Options o; for (int i=1;i<argc;++i) {
        std::string a=argv[i];
        auto next = [&]{ return (i+1<argc)? argv[++i]: (char*)nullptr; };
        if (a=="--rate") o.rate=std::stoul(next());
        else if (a=="--channels") o.channels=std::stoul(next());
        else if (a=="--batch") o.batch=std::stoul(next());
        else if (a=="--port") o.port=(uint16_t)std::stoul(next());
        else if (a=="--help") {
            std::cout << "Usage: adc_daemon [--rate N] [--channels N] [--batch N] [--port P]\n"; std::exit(0);
        }
    } return o;
}

int main(int argc, char** argv) {
    auto opt = parse_args(argc, argv);
    std::signal(SIGINT, signal_handler);
    std::cout << "Iniciando daemon: " << opt.channels << " canais @" << opt.rate << " Hz batch=" << opt.batch << " port=" << opt.port << "\n";
    acq::DriverConfig cfg; cfg.sample_rate_hz = opt.rate; cfg.channels = opt.channels;
    auto driver = acq::create_driver();
    if (!driver->init(cfg)) { std::cerr << "Falha init driver" << std::endl; return 1; }
    auto server = acq::make_server(opt.port);
    if (!server->start()) { std::cerr << "Falha iniciar servidor" << std::endl; return 1; }
    driver->start([&](const std::vector<int32_t>& interleaved, uint64_t t0){
        acq::FrameHeader hdr; hdr.timestamp_ns = t0; hdr.channels = cfg.channels; hdr.samples_per_channel = (uint32_t)(interleaved.size()/cfg.channels); hdr.sample_rate_hz = cfg.sample_rate_hz;
        server->broadcast_frame(hdr, interleaved);
    }, opt.batch);
    while (g_running.load()) std::this_thread::sleep_for(std::chrono::milliseconds(200));
    std::cout << "Encerrando..." << std::endl;
    return 0;
}
