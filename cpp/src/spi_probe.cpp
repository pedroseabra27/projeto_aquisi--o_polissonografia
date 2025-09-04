#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <cstdint>
#include <cstring>

int main(int argc, char** argv) {
    const char* dev = (argc>1)? argv[1]: "/dev/spidev0.0";
    int fd = ::open(dev, O_RDWR);
    if (fd<0) { perror("open"); return 1; }
    uint8_t mode=0; uint8_t bits=8; uint32_t speed=1000000;
    ioctl(fd, SPI_IOC_WR_MODE, &mode);
    ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
    ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
    // Envia RESET (0xFE) e lê 3 bytes "lixo"
    uint8_t tx[4] = {0xFE, 0,0,0};
    uint8_t rx[4] = {0};
    struct spi_ioc_transfer tr{};
    tr.tx_buf = (unsigned long)tx;
    tr.rx_buf = (unsigned long)rx;
    tr.len = sizeof(tx);
    tr.speed_hz = speed;
    tr.bits_per_word = bits;
    if (ioctl(fd, SPI_IOC_MESSAGE(1), &tr) < 0) perror("xfer");
    printf("RESET enviado. Bytes recebidos: %02X %02X %02X %02X\n", rx[0],rx[1],rx[2],rx[3]);
    ::close(fd);
    return 0;
}
