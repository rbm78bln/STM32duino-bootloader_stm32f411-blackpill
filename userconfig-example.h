/* This file is the part of the STM32 secure bootloader
 *
 * Copyright ©2016 Dmitry Filimonchuk <dmitrystu[at]gmail[dot]com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *   http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _DFU_USERCONFIG_H_
#define _DFU_USERCONFIG_H_

/* DEFAULT CONFIG STARTS HERE */
/* Skip unwanted dfuDNLOAD_SYNC phase. Slightly improve speed, but don't meets DFU1.1 state diagram */
//#define DFU_DNLOAD_NOSYNC   _ENABLE

/** Add extra DFU interface for EEPROM */
//#define DFU_INTF_EEPROM     _AUTO

/** Firmware can be uploaded from device */
//#define DFU_CAN_UPLOAD      _ENABLE

/** Handle DFU_DETACH request in DFU mode. System reset will be issued. */
//#define DFU_DETACH          _ENABLE

/** Whether application image is checked by its vector table */
//#define DFU_VERIFY_VTABLE     _ENABLE

/** Whether application image is verified by a checksum algorithm */
//#define DFU_VERIFY_CHECKSUM _DISABLE

/** Memory Readout Protection level **/
//#define DFU_SEAL_LEVEL      0

/* USB VID & PID */
//#define DFU_VENDOR_ID       0x0483
//#define DFU_DEVICE_ID       0xDF11

/* USB manufacturer string */
//#define DFU_STR_MANUF       "Your company name"

/* USB product sting */
//#define DFU_STR_PRODUCT     "Secure bootloader"

/* USB string for DFU configuration string descriptor. */
//#define DFU_DSC_CONFIG      _ENABLE
//#define DFU_STR_CONFIG      "DFU"

/* USB string for DFU flash interface string descriptor. */
//#define DFU_DSC_FLASH       _ENABLE
//#define DFU_STR_FLASH       "Internal flash"

/* USB string for DFU EEPROM interface sreing descriptor */
//#define DFU_DSC_EEPROM      _ENABLE
//#define DFU_STR_EEPROM       "Internal EEPROM"

/* USB EP0 size. Must be 8 for USB FS */
//#define DFU_EP0_SIZE        8

/* DFU properties */
//#define DFU_POLL_TIMEOUT    20
//#define DFU_DETACH_TIMEOUT  200
//#define DFU_BLOCKSZ         0x800

/** Whether the RTC register is checked to enforce bootloader or user application mode */
//#define DFU_CHECK_RTC_MAGIC_NUMBER  _ENABLE
//#define RTC_MAGIC_NUMBER_BOOTLOADER 0x424c
//#define RTC_MAGIC_NUMBER_USERAPP    0x424d

/* 32 bit DFU bootkey value */
//#define DFU_BOOTKEY         0x157F32D4

/* DFU bootkey address. Top of the ram by default. _AUTO, _DISABLE or set address.
 * May be enabled internally. */
//#define DFU_BOOTKEY_ADDR    _AUTO

/* DFU bootstrap port/pin settings. Set GPIOx or _DISABLE */
//#define DFU_BOOTSTRAP_GPIO  GPIOA
//#define DFU_BOOTSTRAP_PIN   0

/* Active bootstrap pin logic level. _HIGH, _LOW */
//#define DFU_BOOTSTRAP_LEVEL _LOW

/* Pullup or pulldown settings for the bootstrap pin _AUTO, _DISABLE, _HIGH, _LOW */
//#define DFU_BOOTSTRAP_PULL  _AUTO

/* Double reset waiting time in mS. _DISABLE or time in mS */
//#define DFU_DBLRESET_MS     300

/* DFU bootstrap port/pin settings. Set GPIOx or _DISABLE */
//#define DFU_LED_GPIO  GPIOC
//#define DFU_LED_RCC   0x004 // (0x1UL << 'x'-'A') => A=0x001, B=0x002, C=0x004 etc
//#define DFU_LED_PIN   13
//#define DFU_LED_ON_LEVEL _LOW

/* Exit the bootloader into the user application after this many "time units" */
/* (0 = never, 1 = instantly/fastboot, 6 = 0.5s/minimum, 680 = 60s, ...32bit) */
//#define DFU_TIMEOUT_DEFAULT       6 // =  0.5s: use dfu-util -w
//#define DFU_TIMEOUT_RTC_MAGIC    35 // =  3s
//#define DFU_TIMEOUT_UPDOWNLOAD  680 // = 60s

/* Skip entering the bootloader after download even when fastboot is disabled */
//#define DFU_SKIP_BOOTLOADER_AFTER_DOWNLOAD _DISABLE

/* User application address. _AUTO or page aligned address.
 * for _AUTO check __app_start address in output linker map file*/
//#define DFU_APP_START       _AUTO

/* User application size. _AUTO or required size in bytes. */
//#define DFU_APP_SIZE        _AUTO

/* Microsoft WCID allows automatic driver (WinUSB) installation on device
 * connection. Use _ENABLE to make your device likeable by Windows. */
//#define DFU_WCID _DISABLE

/* Cipher to use. set _DISABLE or choose from implemented ciphers */
//#define DFU_CIPHER          DFU_CIPHER_RC5
//#define DFU_CIPHER_MODE     DFU_CIPHER_CBC

#endif // _DFU_USERCONFIG_H_
