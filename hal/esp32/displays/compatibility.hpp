#pragma once

#ifdef ARDUINO_ARCH_ESP32

#include "driver/spi_master.h"

// 检查并定义SPI_HOST常量，以兼容新旧版本的ESP-IDF
#if !defined(SPI2_HOST) && defined(VSPI_HOST)
  #define SPI2_HOST VSPI_HOST
#elif !defined(SPI2_HOST)
  #define SPI2_HOST ((spi_host_device_t)1)
#endif

#if !defined(SPI3_HOST) && defined(HSPI_HOST)
  #define SPI3_HOST HSPI_HOST
#elif !defined(SPI3_HOST)
  #define SPI3_HOST ((spi_host_device_t)2)
#endif

#endif