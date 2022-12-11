/* This file is the part of the STM32 secure bootloader
 *
 * Copyright ©2016 Dmitry Filimonchuk <dmitrystu[at]gmail[dot]com>
 * Copyright 2019 by Tsien (UK) Ltd.
 *
 * Author: Adrian Carpenter <tech[at]tsien[dot]com>
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
 *
 */

#ifndef _BOOTLOADER_H_
#define _BOOTLOADER_H_
#if defined(__cplusplus)
    extern "C" {
#endif

/**
 * @brief Transfer and translate RTC magic number to DFU bootkey
 * @return Nothing, just causes side-effects. :)
 */
void rtcmagic_to_dfubootkey(uint32_t* key_addr);

/**
 * @brief Check for having a valid user application in flash
 * @return uint32_t type of bootloader action defined by the user application:
 *         0 = preconditions for starting the app failed, enter bootloader and stay there
 *         1 = preconditions for starting the app passed, continue booting normally
 */
uint32_t have_valid_user_app();

#if defined(__cplusplus)
    }
#endif
#endif // _BOOTLOADER_H_