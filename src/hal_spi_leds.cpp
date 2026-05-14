#include "hal_spi_leds.h"
#include "output_scale.h"
#include <cstring>
#include <esp_heap_caps.h>

void LedDriver::recomputeScale(uint16_t numLeds, bool radialBalance) {
    applyScale_ = compute_output_scale(outputScale_, maxLeds_, numLeds, radialBalance);
}

bool LedDriver::init(uint8_t clkPin, uint8_t mosiPin, uint8_t clockMhz, uint16_t maxLeds) {
    maxLeds_ = maxLeds;
    memset(outputScale_, 255, sizeof(outputScale_));

    spi_bus_config_t bus = {};
    bus.mosi_io_num   = mosiPin;
    bus.miso_io_num   = -1;
    bus.sclk_io_num   = clkPin;
    bus.quadwp_io_num = -1;
    bus.quadhd_io_num = -1;
    bus.max_transfer_sz = (4 + maxLeds * 4 + ((maxLeds + 15) / 16)) ;

    if (spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO) != ESP_OK)
        return false;

    spi_device_interface_config_t dev = {};
    dev.clock_speed_hz = clockMhz * 1000000;
    dev.mode           = 0;
    dev.spics_io_num   = -1;
    dev.queue_size     = 1;

    if (spi_bus_add_device(SPI2_HOST, &dev, &spi_) != ESP_OK)
        return false;

    size_t bufSz = 4 + maxLeds * 4 + ((maxLeds + 15) / 16);
    txBuf_ = (uint8_t*)heap_caps_malloc(bufSz, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    return txBuf_ != nullptr;
}

uint16_t LedDriver::buildFrame(const Pixel* pixels, uint16_t count) {
    memset(txBuf_, 0x00, 4);
    uint16_t pos = 4;

    if (applyScale_) {
        for (uint16_t i = 0; i < count; i++) {
            uint16_t s = outputScale_[i];
            txBuf_[pos++] = pixels[i].brightness;
            txBuf_[pos++] = (uint8_t)((pixels[i].blue  * s) >> 8);
            txBuf_[pos++] = (uint8_t)((pixels[i].green * s) >> 8);
            txBuf_[pos++] = (uint8_t)((pixels[i].red   * s) >> 8);
        }
    } else {
        memcpy(txBuf_ + pos, pixels, count * sizeof(Pixel));
        pos += count * sizeof(Pixel);
    }

    uint16_t endBytes = (count + 15) / 16;
    memset(txBuf_ + pos, 0xFF, endBytes);
    pos += endBytes;

    return pos;
}

void LedDriver::sendSlice(const Pixel* pixels, uint16_t count) {
    uint16_t len = buildFrame(pixels, count);

    spi_transaction_t t = {};
    t.length    = len * 8;
    t.tx_buffer = txBuf_;
    spi_device_transmit(spi_, &t);
}

void LedDriver::allOff(uint16_t count) {
    memset(txBuf_, 0x00, 4);
    uint16_t pos = 4;
    for (uint16_t i = 0; i < count; i++) {
        txBuf_[pos++] = 0xE0;  // brightness=0
        txBuf_[pos++] = 0;
        txBuf_[pos++] = 0;
        txBuf_[pos++] = 0;
    }
    uint16_t endBytes = (count + 15) / 16;
    memset(txBuf_ + pos, 0xFF, endBytes);
    pos += endBytes;

    spi_transaction_t t = {};
    t.length    = pos * 8;
    t.tx_buffer = txBuf_;
    spi_device_transmit(spi_, &t);
}
