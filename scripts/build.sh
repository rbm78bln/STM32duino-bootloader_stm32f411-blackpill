#!/bin/sh -e

test -f Makefile || cd ..

make clean
# make DFU_USER_CONFIG=userconfig.h stm32f103x8 	# bluepill
# make DFU_USER_CONFIG=userconfig.h stm32f103xb 	# stink baite
# make DFU_USER_CONFIG=userconfig.h stm32f401xe 	# blackpill
# make DFU_USER_CONFIG=userconfig.h stm32f401xe_0x2000 	# blackpill
# make DFU_USER_CONFIG=userconfig.h stm32f411xe 	# blackpill
# make DFU_USER_CONFIG=userconfig.h stm32f411xe_0x2000 	# blackpill

ls -l build/
