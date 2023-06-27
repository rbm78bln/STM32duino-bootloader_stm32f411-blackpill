First things first: Don't forget to read the original [README.md](README_org.md) of the parent project.

# Secure STM32duino-compatible USB DFU1.1 bootloader for STM32F401(xE) and STM32F411(xE) (Blackpill)
This is a fork of [dmitrystu's](https://github.com/dmitrystu/) great [sboot_stm32](https://github.com/dmitrystu/sboot_stm32) bootloader, which is a beautiful piece of software, with a few modifications to make it act similarly like [rogerclarkmelbourne's](https://github.com/rogerclarkmelbourne/) [STM32duino-bootloader](https://github.com/rogerclarkmelbourne/STM32duino-bootloader), adapt the nifty little RTC magic number feature from [Serasidis'](https://github.com/Serasidis/) [STM32 HID-Bootloader](https://github.com/Serasidis/STM32_HID_Bootloader), and thus provide better compatiblity to the [STM32duino Arduino Core](https://github.com/stm32duino/Arduino_Core_STM32) - especially when developing with [PlatformIO](https://docs.platformio.org/en/latest/integration/ide/vscode.html) and VSCode.

## So why all this?
For downloading firmwares to your Blackpill via USB you can simply do the little finger dance and you're fine... kind of... unless you're some lazy dude like me, who doesn't want to do the extra [equilibristics](https://en.wiktionary.org/wiki/equilibristics) to fiddle around with the board all the time while coding. Having a pretty weak pull-down connected to BOOT1 with a far higher resistance than my thick clumsy fingers doesn't help neither.

Since the STM32F103 doesn't have a USB-capable bootloader in ROM, [Roger](https://github.com/rogerclarkmelbourne/) wrote the [STM32duino-bootloader](https://github.com/rogerclarkmelbourne/STM32duino-bootloader) for the Bluepill and [ST-Link Baite-Clones](https://www.google.com/search?q=STink+bait), which always starts up after resetting the board and then proceeds to the user application after a short timeout. This way the host can toggle the DTR "pin" of the USB-serial which is detected by the [STM32duino Arduino Core](https://github.com/stm32duino/Arduino_Core_STM32) and triggers a reset of the board like the [Maple](https://www.leaflabs.com/maple) boards first implemented this.

The beforementioned [HID Bootloader](https://github.com/Serasidis/STM32_HID_Bootloader), which is [only partly supported](https://docs.platformio.org/en/stable/boards/ststm32/blackpill_f411ce.html#uploading) by PlatformIO's [ST STM32-platform](https://docs.platformio.org/en/stable/platforms/ststm32.html), and [Roger's](https://github.com/rogerclarkmelbourne/) [STM32duino-bootloader](https://github.com/rogerclarkmelbourne/STM32duino-bootloader) additionally implement [checking a register of the RTC's backup domain](https://github.com/rogerclarkmelbourne/STM32duino-bootloader/blob/master/hardware.c#L241) to determine whether the bootloader should be [force-entered](https://github.com/Serasidis/STM32_HID_Bootloader/blob/master/bootloader/F4/Src/main.c#L136) or [skipped completely](https://github.com/rogerclarkmelbourne/STM32duino-bootloader/blob/master/main.c#L49) after the reset, which can be controlled by the user application and is also reflected in the [STM32duino Arduino Core](https://github.com/stm32duino/Arduino_Core_STM32). See its [bootloader.c](https://github.com/stm32duino/Arduino_Core_STM32/blob/main/libraries/SrcWrapper/src/stm32/bootloader.c) for more details.

To complete the whole and round it nicely [WeAct](https://github.com/WeActStudio/) especially made their [HID Bootloader](https://github.com/WeActStudio/WeAct_HID_Bootloader_F4x1) incompatible with [Serasidis'](https://github.com/Serasidis/) [STM32 HID-Bootloader](https://github.com/Serasidis/STM32_HID_Bootloader) from which it was forked and their Blackpills are usually shipped with. Yay! [I'm lovin' it!](https://www.youtube.com/watch?v=tfhlEP8LT4k)

## Bringing it all together
So what would be nice is a standard dfu-capable bootloader that can easily be entered automatically using your development IDE and is supported by the standard STM32-Arduino Framework.
So here you go!

### This bootloader implements
+ The DFU protocol (sboot)
+ Application offsets 0x2000 and 0x4000 in flash
(it can actually be quite tiny depending on the selected features)
+ Automatically entering the bootloader after power-on (STM32duino-bootloader)
+ Entering the bootloader by double-clicking the reset-button (sboot)
+ Entering the bootloader by holding down a user-button (all)
+ Force-entering the bootloader when no firmware is detected (STM32duino-bootloader)
+ Force-entering or force-skipping the bootloader by
  + RTC magic number (HID/STM32duino Bootloader) or
  + magic number in SRAM (sboot)
+ Automatically exiting the bootloader and running the firmware after various configurable timeouts
+ Encrypted firmware downloads (sboot)
+ Verifying the checksum of the firmware (sboot)
+ A nervously flashing LED while in bootloader
+ Each of the features above can be enabled/disabled separately
Not all combinations make sense! You choose! ([userconfig.h](https://github.com/rbm78bln/STM32duino-bootloader_stm32f411-blackpill/blob/master/userconfig-example.h))

### Supported MCU targets

| mcu_target    | MCU         | remarks |
|---------------|-------------|---------|
| stm32f401xe   | STM32F401xE | tested  |
| stm32f411xe   | STM32F411xE | tested  |

### This bootloader does not (yet?) implement
+ ST-DfuSe extensions
+ STM32F103 MCU targets (Bluepills, [Baite-STLinks](https://www.google.com/search?q=STink+bait))
+ Being badly incompatible to everything else ([WeAct HID-Bootloader](https://github.com/WeActStudio/WeAct_HID_Bootloader_F4x1))

## Activating the bootloader
+ Simply just power up your board
+ Toggle the DTR "pin" of the USB-serial interface
+ Write RTC_MAGIC_NUMBER_BOOTLOADER into register 4 of the RTC backup domain (RTC->BKP4R) and issue a (whatever) reset
+ Write DFU_BOOTKEY at DFU_BOOTKEY_ADDR (RAM top by default) and make a software reset
+ Assert DFU_BOOTSTRAP_PIN on DFU_BOOTSTRAP_PORT on startup
+ Make a double reset during the DFU_DBLRESET_MS period
+ Overwrite the first four bytes of your user firmware

## Using the bootloader
In [```examples/hello_world```](examples/hello_world) you can find a very basic example on how to use this bootloader for your daily work with [PlatformIO](https://docs.platformio.org/en/latest/integration/ide/vscode.html) and VSCode.
<br>

#### How can I use this bootloader with Arduino IDE?
What is [Arduino IDE](https://github.com/arduino/arduino-ide)?
<br>

![sboot_stm32f411](https://miunske.eu/github/?sboot_stm32f411)
