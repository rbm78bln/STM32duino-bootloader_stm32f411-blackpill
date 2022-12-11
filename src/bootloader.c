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

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "config.h"
#include "bootloader.h"
#include "stm32.h"
#include "usb.h"
#include "usb_dfu.h"
#include "descriptors.h"
#include "flash.h"
#include "crypto.h"

/* Checking for the EEPROM */
#if defined(DATA_EEPROM_BASE) && defined(DATA_EEPROM_END)
    #define _EE_START    DATA_EEPROM_BASE
    #define _EE_LENGTH   (DATA_EEPROM_END - DATA_EEPROM_BASE + 1)
#elif defined(DATA_EEPROM_BASE) && defined(DATA_EEPROM_BANK2_END)
    #define _EE_START    DATA_EEPROM_BASE
    #define _EE_LENGTH   (DATA_EEPROM_BANK2_END - DATA_EEPROM_BASE + 1)
#elif defined(FLASH_EEPROM_BASE)
    #define _EE_START    FLASH_EEPROM_BASE
    #define _EE_LENGTH   (FLASH_EEPROM_END - FLASH_EEPROM_BASE + 1 )
#endif

#if (DFU_INTF_EEPROM == _ENABLE) && !defined(_EE_START)
    #error No EEPROM found. Check config !!
#elif ((DFU_INTF_EEPROM == _AUTO) || (DFU_INTF_EEPROM == _ENABLE)) && defined(_EE_START)
    #define _EEPROM_ENABLED
#endif

/* Checking for application start address */
#if (DFU_APP_START == _AUTO)
    #define _APP_START  ((size_t)&__app_start)
#elif ((DFU_APP_START & 0x000007FF) == 0)
    #define _APP_START  DFU_APP_START
#else
    #error DFU_APP_START must be 2k aligned. Check config !!
#endif

/* Checking for application size */
#if (DFU_APP_SIZE == _AUTO)
    #define _APP_LENGTH ((size_t)&__romend - _APP_START)
#else
    #define _APP_LENGTH DFU_APP_SIZE
#endif

#if (DFU_BOOTKEY_ADDR == _AUTO) || (DFU_BOOTKEY_ADDR == _DISABLE)
    #define _KEY_ADDR   __stack
#else
    #define _KEY_ADDR   DFU_BOOTKEY_ADDR
#endif

/* DFU request buffer size data + request header */
#define DFU_BUFSZ  ((DFU_BLOCKSZ + 3 + 8) >> 2)

extern uint8_t  __app_start;
extern uint8_t  __romend;
extern uint32_t __stack;

static uint32_t dfu_buffer[DFU_BUFSZ];
static usbd_device dfu;

uint32_t dfu_timeout;

static struct dfu_data_s {
    uint8_t     (*flash)(void *romptr, const void *buf, size_t blksize);
    void        *dptr;
    size_t      remained;
    uint8_t     interface;
    uint8_t     bStatus;
    uint8_t     bState;
} dfu_data;

/** Processing DFU_SET_IDLE request */
static usbd_respond dfu_set_idle(void) {
    aes_init();
    dfu_data.bState = USB_DFU_STATE_DFU_IDLE;
    dfu_data.bStatus = USB_DFU_STATUS_OK;
    switch (dfu_data.interface){
#if defined(_EEPROM_ENABLED)
    case 1:
        dfu_data.dptr = (void*)_EE_START;
        dfu_data.remained = _EE_LENGTH;
        dfu_data.flash = program_eeprom;
        break;
#endif
    default:
        dfu_data.dptr = (void*)_APP_START;
        dfu_data.remained = _APP_LENGTH;
        dfu_data.flash = program_flash;
        break;
    }
    return usbd_ack;
}

extern void System_Reset(void);
extern void System_try_Reboot_into_Application(void);
extern void System_Reboot_into_Bootloader(void);

#if (DFU_CHECK_RTC_MAGIC_NUMBER != _DISABLE)
#ifndef RCC_APB1ENR_BKPEN
#define RCC_APB1ENR_BKPEN 0
#endif

void rtcmagic_to_dfubootkey(uint32_t* key_addr) {
    // enable RTC backup domain
    RCC->APB1ENR |= (RCC_APB1ENR_PWREN | RCC_APB1ENR_BKPEN);
    PWR->CR |= PWR_CR_DBP;
    RCC->BDCR |= RCC_BDCR_RTCEN;

    switch (RTC->BKP4R)
    {
        case RTC_MAGIC_NUMBER_BOOTLOADER:
            (*key_addr) = (uint32_t)DFU_BOOTKEY;
            RTC->BKP4R = 0;
            break;

        case RTC_MAGIC_NUMBER_USERAPP:
            (*key_addr) = ~(uint32_t)DFU_BOOTKEY;
            RTC->BKP4R = 0;
            break;

        default:
            break;
    }

    // restore previous state
    RCC->BDCR &=~ RCC_BDCR_RTCEN;
    PWR->CR &=~ PWR_CR_DBP;
    RCC->APB1ENR &=~ (RCC_APB1ENR_PWREN | RCC_APB1ENR_BKPEN);
    return;
}

#pragma GCC diagnostic ignored "-Wunused-function"
static uint32_t get_rtc_magic_number() {
    uint32_t _RCC_APB1ENR, _PWR_CR, _RCC_BDCR;
    uint32_t value;

    _RCC_APB1ENR = (RCC->APB1ENR & (RCC_APB1ENR_PWREN | RCC_APB1ENR_BKPEN));
    RCC->APB1ENR |= (RCC_APB1ENR_PWREN | RCC_APB1ENR_BKPEN);

    _PWR_CR = (PWR->CR & PWR_CR_DBP);
    PWR->CR |= PWR_CR_DBP;

    _RCC_BDCR = (RCC->BDCR & RCC_BDCR_RTCEN);
    RCC->BDCR |= RCC_BDCR_RTCEN;

    value = RTC->BKP4R;

    RCC->BDCR &= _RCC_BDCR;         // RCC->BDCR &=~ RCC_BDCR_RTCEN;
    PWR->CR &= _PWR_CR;             // PWR->CR &=~ PWR_CR_DBP;
    RCC->APB1ENR &= _RCC_APB1ENR;   // RCC->APB1ENR &=~ (RCC_APB1ENR_PWREN | RCC_APB1ENR_BKPEN);
    return (value);
}

static void set_rtc_magic_number(uint32_t value) {
    uint32_t _RCC_APB1ENR, _PWR_CR, _RCC_BDCR;

    _RCC_APB1ENR = (RCC->APB1ENR & (RCC_APB1ENR_PWREN | RCC_APB1ENR_BKPEN));
    RCC->APB1ENR |= (RCC_APB1ENR_PWREN | RCC_APB1ENR_BKPEN);

    _PWR_CR = (PWR->CR & PWR_CR_DBP);
    PWR->CR |= PWR_CR_DBP;

    _RCC_BDCR = (RCC->BDCR & RCC_BDCR_RTCEN);
    RCC->BDCR |= RCC_BDCR_RTCEN;

    RTC->BKP4R = value;

    RCC->BDCR &= _RCC_BDCR;         // RCC->BDCR &=~ RCC_BDCR_RTCEN;
    PWR->CR &= _PWR_CR;             // PWR->CR &=~ PWR_CR_DBP;
    RCC->APB1ENR &= _RCC_APB1ENR;   // RCC->APB1ENR &=~ (RCC_APB1ENR_PWREN | RCC_APB1ENR_BKPEN);
}
#pragma GCC diagnostic pop
#endif

#if (DFU_VERIFY_VTABLE != _DISABLE)
static inline uint32_t check_user_code() {
    if ( ( (*((uint32_t *)_APP_START)) & 0xFFFC0000 ) == 0x20000000) {
        return (1);
    } else {
        return (0);
    }
}
#endif

uint32_t have_valid_user_app() {
#if (DFU_VERIFY_VTABLE != _DISABLE)
    if(!check_user_code()) {
        dfu_timeout = 0;
        return (0);
    }
#endif
#if (DFU_TIMEOUT_DEFAULT == 1) // i.e. fastboot
    return 1;
#else
    dfu_timeout = DFU_TIMEOUT_DEFAULT;
    return 0;
#endif
}

#ifndef DFU_LED_GPIO
static void gpio_enable(uint8_t state) {}
static void set_led(uint8_t state) {}
static uint8_t get_led() { return (0); }
#else
static void gpio_enable(uint8_t state) {
    if(state) {
        RCC->AHB1ENR         |=  DFU_LED_RCC;
        DFU_LED_GPIO->MODER  &=~ (0x3UL << (2*DFU_LED_PIN));
        DFU_LED_GPIO->MODER  |=  (0x1UL << (2*DFU_LED_PIN));
        DFU_LED_GPIO->OTYPER &=~ (0x1UL << (1*DFU_LED_PIN));
        DFU_LED_GPIO->PUPDR  &=~ (0x3UL << (2*DFU_LED_PIN));
#if (DFU_LED_ON_LEVEL == _LOW)
        DFU_LED_GPIO->BSRR    =  (0x1UL << (1*DFU_LED_PIN + 0x00UL)); // = _HIGH
#else
        DFU_LED_GPIO->BSRR    =  (0x1UL << (1*DFU_LED_PIN + 0x10UL)); // = _LOW
#endif
    } else {
#if (DFU_LED_ON_LEVEL == _LOW)
        DFU_LED_GPIO->BSRR    =  (0x1UL << (1*DFU_LED_PIN + 0x00UL)); // = _HIGH
#else
        DFU_LED_GPIO->BSRR    =  (0x1UL << (1*DFU_LED_PIN + 0x10UL)); // = _LOW
#endif
        DFU_LED_GPIO->MODER  &=~ (0x3UL << (2*DFU_LED_PIN));
        DFU_LED_GPIO->OTYPER &=~ (0x1UL << (1*DFU_LED_PIN));
        DFU_LED_GPIO->PUPDR  &=~ (0x3UL << (2*DFU_LED_PIN));
        DFU_LED_GPIO->PUPDR  |=  (0x1UL << (2*DFU_LED_PIN));
        RCC->AHB1ENR         &=~ DFU_LED_RCC;
    }
}

static void set_led(uint8_t state) {
    if(state) {
        DFU_LED_GPIO->MODER  &=~ (0x3UL << (2*DFU_LED_PIN));
        DFU_LED_GPIO->MODER  |=  (0x1UL << (2*DFU_LED_PIN));
        DFU_LED_GPIO->OTYPER &=~ (0x1UL << (1*DFU_LED_PIN));
        DFU_LED_GPIO->PUPDR  &=~ (0x3UL << (2*DFU_LED_PIN));
        DFU_LED_GPIO->BSRR    =  GPIO_BSRR_BR13;
#if (DFU_LED_ON_LEVEL == _LOW)
        DFU_LED_GPIO->BSRR    =  (0x1UL << (1*DFU_LED_PIN + 0x10UL)); // = _LOW
#else
        DFU_LED_GPIO->BSRR    =  (0x1UL << (1*DFU_LED_PIN + 0x00UL)); // = _HIGH
#endif
    } else {
#if (DFU_LED_ON_LEVEL == _LOW)
        DFU_LED_GPIO->BSRR    =  (0x1UL << (1*DFU_LED_PIN + 0x00UL)); // = _HIGH
#else
        DFU_LED_GPIO->BSRR    =  (0x1UL << (1*DFU_LED_PIN + 0x10UL)); // = _LOW
#endif
        DFU_LED_GPIO->MODER  &=~ (0x3UL << (2*DFU_LED_PIN));
        DFU_LED_GPIO->PUPDR  &=~ (0x3UL << (2*DFU_LED_PIN));
#if (DFU_LED_ON_LEVEL == _LOW)
        DFU_LED_GPIO->PUPDR  |=  (0x1UL << (2*DFU_LED_PIN)); // = _PULL_UP
#else
        DFU_LED_GPIO->PUPDR  |=  (0x2UL << (2*DFU_LED_PIN)); // = _PULL_DOWN
#endif
    }
}

static uint8_t get_led() {
#if (DFU_LED_ON_LEVEL == _LOW)
    return ((DFU_LED_GPIO->IDR & (0x1UL << (1*DFU_LED_PIN))) == 0);
#else
    return ((DFU_LED_GPIO->IDR & (0x1UL << (1*DFU_LED_PIN))) != 0);
#endif
}
#endif

static usbd_respond dfu_err_badreq(void) {
    dfu_data.bState  = USB_DFU_STATE_DFU_ERROR;
    dfu_data.bStatus = USB_DFU_STATUS_ERR_STALLEDPKT;
    return usbd_fail;
}

#if (DFU_CAN_UPLOAD == _ENABLE)
static usbd_respond dfu_upload(usbd_device *dev, size_t blksize) {
    switch (dfu_data.bState) {
    case USB_DFU_STATE_DFU_IDLE:
    case USB_DFU_STATE_DFU_UPLOADIDLE:
        dfu_timeout = DFU_TIMEOUT_UPDOWNLOAD;
        if (dfu_data.remained == 0) {
            dev->status.data_count = 0;
            return dfu_set_idle();
        } else if (dfu_data.remained < DFU_BLOCKSZ) {
            blksize = dfu_data.remained;
        }
        aes_encrypt(dev->status.data_ptr, dfu_data.dptr, blksize);
        dev->status.data_count = blksize;
        dfu_data.remained -= blksize;
        dfu_data.dptr += blksize;
        return usbd_ack;
    default:
        return dfu_err_badreq();
    }
}
#endif

static usbd_respond dfu_dnload(void *buf, size_t blksize) {
    switch(dfu_data.bState) {
    case    USB_DFU_STATE_DFU_DNLOADIDLE:
    case    USB_DFU_STATE_DFU_DNLOADSYNC:
    case    USB_DFU_STATE_DFU_IDLE:
        dfu_timeout = DFU_TIMEOUT_UPDOWNLOAD;
        if (blksize == 0) {
            dfu_data.bState = USB_DFU_STATE_DFU_MANIFESTSYNC;
            return usbd_ack;
        }
        if (blksize > dfu_data.remained) {
            dfu_data.bStatus = USB_DFU_STATUS_ERR_ADDRESS;
            dfu_data.bState = USB_DFU_STATE_DFU_ERROR;
            return usbd_ack;
        }
        aes_decrypt(buf, buf, blksize );
        dfu_data.bStatus = dfu_data.flash(dfu_data.dptr, buf, blksize);

        if (dfu_data.bStatus == USB_DFU_STATUS_OK) {
            dfu_data.dptr += blksize;
            dfu_data.remained -= blksize;
#if (DFU_SKIP_BOOTLOADER_AFTER_DOWNLOAD != _DISABLE)
            set_rtc_magic_number(RTC_MAGIC_NUMBER_USERAPP);
#endif
#if (DFU_DNLOAD_NOSYNC == _ENABLE)
            dfu_data.bState = USB_DFU_STATE_DFU_DNLOADIDLE;
#else
            dfu_data.bState = USB_DFU_STATE_DFU_DNLOADSYNC;
#endif
            return usbd_ack;
        } else {
            dfu_data.bState = USB_DFU_STATE_DFU_ERROR;
            return usbd_ack;
        }
    default:
        return dfu_err_badreq();
    }
}

static usbd_respond dfu_getstatus(void *buf) {
    /* make answer */
    struct usb_dfu_status *stat = buf;
    stat->bStatus = dfu_data.bStatus;
    stat->bState = dfu_data.bState;
    stat->bPollTimeout = (DFU_POLL_TIMEOUT & 0xFF);
    stat->wPollTimeout = (DFU_POLL_TIMEOUT >> 8);
    stat->iString = NO_DESCRIPTOR;

    switch (dfu_data.bState) {
    case USB_DFU_STATE_DFU_IDLE:
    case USB_DFU_STATE_DFU_DNLOADIDLE:
    case USB_DFU_STATE_DFU_UPLOADIDLE:
    case USB_DFU_STATE_DFU_ERROR:
        return usbd_ack;
    case USB_DFU_STATE_DFU_DNLOADSYNC:
        dfu_data.bState = USB_DFU_STATE_DFU_DNLOADIDLE;
        return usbd_ack;
    case USB_DFU_STATE_DFU_MANIFESTSYNC:
        return dfu_set_idle();
    default:
        return dfu_err_badreq();
    }
}

static usbd_respond dfu_getstate(uint8_t *buf) {
    *buf = dfu_data.bState;
    return usbd_ack;
}

static usbd_respond dfu_abort() {
    switch (dfu_data.bState) {
    case USB_DFU_STATE_DFU_IDLE:
    case USB_DFU_STATE_DFU_DNLOADSYNC:
    case USB_DFU_STATE_DFU_DNLOADIDLE:
    case USB_DFU_STATE_DFU_MANIFESTSYNC:
    case USB_DFU_STATE_DFU_UPLOADIDLE:
        return dfu_set_idle();
    default:
        return dfu_err_badreq();
    }
}

static usbd_respond dfu_clrstatus() {
    if (dfu_data.bState == USB_DFU_STATE_DFU_ERROR)  {
        return dfu_set_idle();
    } else {
        return dfu_err_badreq();
    }
}

static void dfu_reset(usbd_device *dev, uint8_t ev, uint8_t ep) {
    (void)dev;
    (void)ev;
    (void)ep;
    /** TODO : add firmware checkout */
    System_Reset();
}

static usbd_respond dfu_control (usbd_device *dev, usbd_ctlreq *req, usbd_rqc_callback *callback) {
    (void)callback;
    if ((req->bmRequestType  & (USB_REQ_TYPE | USB_REQ_RECIPIENT)) == (USB_REQ_STANDARD | USB_REQ_INTERFACE)) {
        switch (req->bRequest) {
        case USB_STD_SET_INTERFACE:
            if (req->wIndex != 0) return usbd_fail;
            switch (req->wValue) {
            case 0: break;
#if defined(_EEPROM_ENABLED)
            case 1: break;
#endif
            default:
                return usbd_fail;
            }
            dfu_data.interface = req->wValue;
            return dfu_set_idle();
        case USB_STD_GET_INTERFACE:
            req->data[0] = dfu_data.interface;
            return usbd_ack;
        default:
            return usbd_fail;
        }
    }
    if ((req->bmRequestType & (USB_REQ_TYPE | USB_REQ_RECIPIENT)) == (USB_REQ_CLASS | USB_REQ_INTERFACE)) {
        switch (req->bRequest) {
#if (DFU_DETACH == _ENABLE)
        case USB_DFU_DETACH:
            *callback = (usbd_rqc_callback)dfu_reset;
            return usbd_ack;
#endif
        case USB_DFU_DNLOAD:
            if (req->wLength <= DFU_BLOCKSZ) {
                return dfu_dnload(req->data, req->wLength);
            }
            break;
        case USB_DFU_UPLOAD:
#if (DFU_CAN_UPLOAD == _ENABLE)
            if (req->wLength <= DFU_BLOCKSZ) {
                return dfu_upload(dev, req->wLength);
            }
#endif
            break;
        case USB_DFU_GETSTATUS:
            return dfu_getstatus(req->data);
        case USB_DFU_CLRSTATUS:
            return dfu_clrstatus();
        case USB_DFU_GETSTATE:
            return dfu_getstate(req->data);
        case USB_DFU_ABORT:
            return dfu_abort();
        default:
            break;
        }
        return dfu_err_badreq();
    }
#if (DFU_WCID != _DISABLE)
    if ((req->bmRequestType & USB_REQ_TYPE) == USB_REQ_VENDOR) {
        return dfu_get_vendor_descriptor(req, &dev->status.data_ptr, &dev->status.data_count);
    }
#endif
    return usbd_fail;
}


static usbd_respond dfu_config(usbd_device *dev, uint8_t config) {
    switch (config) {
    case 0:
        usbd_reg_event(dev, usbd_evt_reset, 0);
        break;
    case 1:
        usbd_reg_event(dev, usbd_evt_reset, dfu_reset);
        break;
    default:
        return usbd_fail;
    }
    return usbd_ack;
}


static void dfu_init (void) {
    dfu_set_idle();
    usbd_init(&dfu, &usbd_hw, DFU_EP0_SIZE, dfu_buffer, sizeof(dfu_buffer));
    usbd_reg_config(&dfu, dfu_config);
    usbd_reg_control(&dfu, dfu_control);
    usbd_reg_descr(&dfu, dfu_get_descriptor);
    usbd_enable(&dfu, 1);
    usbd_connect(&dfu, 1);
}

int main (void) {
    uint32_t timeout;
    uint16_t i = 0;

    if(dfu_timeout == 1) {
        dfu_timeout = 0;
        System_try_Reboot_into_Application();
    }

    timeout = dfu_timeout;
    gpio_enable(1);
    set_led(1);

    dfu_init();
    do
    {
        while(timeout != 1) {
            if(!++i) {
                if(dfu_timeout == DFU_TIMEOUT_UPDOWNLOAD) {
                    timeout = dfu_timeout;
                    --dfu_timeout;
                }
                if(timeout>1) --timeout;
                set_led(!get_led());
            }
            usbd_poll(&dfu);
        }
        timeout = 0;
    } while (!check_user_code());

    set_led(0);
    gpio_enable(0);

    usbd_connect(&dfu, 0);
    usbd_enable(&dfu, 0);
    System_try_Reboot_into_Application();
    System_Reboot_into_Bootloader();
    while(1);
    return (0);
}
