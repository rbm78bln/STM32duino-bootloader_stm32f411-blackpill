#!/bin/sh

test -f Makefile || cd ..

openocd \
	-f interface/stlink.cfg -c "transport select hla_swd" \
	-f target/stm32f4x.cfg -c init \
	-c "reset halt" -c "wait_halt 2000" -c "stm32f2x unlock 0" -c "wait_halt 2000" \
	-c "reset halt" -c "wait_halt 2000" -c "stm32f2x mass_erase 0" -c "wait_halt 10000" \
	-c "reset halt" -c "wait_halt 2000" -c "program build/firmware.hex" \
	-c "reset run" -c shutdown

exit $?
