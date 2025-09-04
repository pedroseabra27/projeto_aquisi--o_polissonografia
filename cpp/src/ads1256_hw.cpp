#include "ads1256_driver.h"
#include "frame_protocol.h"
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <thread>
#include <cmath>
#include <vector>
#include <cstring>
#include <memory>
#include <iostream>

/*
 * Esqueleto inicial para driver real ADS1256.
 * Ajuste comandos conforme datasheet (reset, SDATAC, WREG, SELFCAL, RDATAC etc.).
 * Este arquivo só é compilado quando ENABLE_SIMULATOR=OFF.
 */

namespace acq {

namespace {
constexpr const char* DEFAULT_SPI_DEV = "/dev/spidev0.0"; // Ajustar se usar CE1 -> 0.1

// Comandos básicos ADS1256 (datasheet)
enum Cmd : uint8_t {
    CMD_WAKEUP = 0x00,
    CMD_RDATA  = 0x01,
    CMD_RDATAC = 0x03,
    CMD_SDATAC = 0x0F,
    CMD_RREG   = 0x10,
    CMD_WREG   = 0x50,
    CMD_SELFCAL= 0xF0,
    CMD_SYNC   = 0xFC,
    CMD_RESET  = 0xFE,
};

struct SpiDev {
    int fd{-1};
    bool open_dev(const char* path) {
        fd = ::open(path, O_RDWR);
        if (fd<0) { perror("open spi"); return false; }
        uint8_t mode=0; uint32_t speed=1000000; uint8_t bits=8;
        if (ioctl(fd, SPI_IOC_WR_MODE, &mode)<0) perror("SPI_IOC_WR_MODE");
        if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed)<0) perror("SPI_IOC_WR_SPEED");
        if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits)<0) perror("SPI_IOC_WR_BITS");
        return true;
    }
    bool xfer(uint8_t* tx, uint8_t* rx, size_t len) {
        struct spi_ioc_transfer tr{};
        tr.tx_buf = (unsigned long)tx;
        tr.rx_buf = (unsigned long)rx;
        tr.len = len;
        tr.speed_hz = 1000000;
        tr.bits_per_word = 8;
        tr.cs_change = 0;
        return ioctl(fd, SPI_IOC_MESSAGE(1), &tr) >= 0;
    }
    ~SpiDev(){ if (fd>=0) ::close(fd); }
};

} // anon

class HwAds1256 : public IAdcDriver {
public:
    bool init(const DriverConfig& cfg) override {
        cfg_ = cfg;
        if (!spi_.open_dev(DEFAULT_SPI_DEV)) return false;
        // Reset
        send_cmd(CMD_RESET); std::this_thread::sleep_for(std::chrono::milliseconds(5));
        send_cmd(CMD_SDATAC);
        // TODO: configurar registradores (STATUS, MUX, ADCON, DRATE) via WREG
        // Placeholder: iniciar selfcal
        send_cmd(CMD_SELFCAL);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return true;
    }
    bool start(SampleBatchCallback cb, uint32_t batch_samples_per_channel) override {
        if (running_) return false;
        batch_ = batch_samples_per_channel;
        running_ = true;
        // Usaremos RDATA único por canal sequencial (round-robin) simples para MVP
        worker_ = std::thread([this, cb]{ loop(cb); });
        return true;
    }
    void stop() override {
        running_ = false;
        if (worker_.joinable()) worker_.join();
    }
private:
    void send_cmd(uint8_t c) {
        uint8_t tx[1] = {c}; uint8_t rx[1] = {0};
        if (!spi_.xfer(tx, rx, 1)) perror("spi cmd");
    }
    void loop(SampleBatchCallback cb) {
        std::vector<int32_t> interleaved;
        interleaved.resize(cfg_.channels * batch_);
        while (running_) {
            auto t0 = now_epoch_ns();
            // Simplificação: ainda não configura MUX por canal; assume modo já setado
            for (uint32_t s=0; s<batch_; ++s) {
                for (uint32_t c=0; c<cfg_.channels; ++c) {
                    // Trocar canal: escrever registro MUX (WREG)
                    // TODO: implementar mudança de canal real
                    // Ler dado
                    int32_t sample = read_sample_blocking();
                    interleaved[s*cfg_.channels + c] = sample;
                }
            }
            cb(interleaved, t0);
        }
    }
    int32_t read_sample_blocking() {
        // Comando RDATA -> esperar DRDY baixo (ideal: via GPIO poll). Aqui: atraso fixo.
        send_cmd(CMD_RDATA);
        std::this_thread::sleep_for(std::chrono::milliseconds(1)); // placeholder
        uint8_t tx[3] = {0,0,0}; uint8_t rx[3] = {0};
        spi_.xfer(tx, rx, 3);
        int32_t v = (rx[0]<<16) | (rx[1]<<8) | rx[2];
        if (v & 0x800000) v |= 0xFF000000; // sign-extend 24->32
        return v;
    }
    DriverConfig cfg_;
    SpiDev spi_;
    bool running_ = false;
    uint32_t batch_{0};
    std::thread worker_;
};

std::unique_ptr<IAdcDriver> create_driver() {
    return std::make_unique<HwAds1256>();
}

} // namespace acq
