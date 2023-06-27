Import('env')
import os
import re

build_base    = 0x08000000
build_offset  = 0x00002000
build_address = (build_base+build_offset)

board_config = env.BoardConfig()
board_config.update("build.hwids", [
  ["0x0483", "0xDF11"], # dfu-util will compare against the first one
  ["0x1EAF", "0x0003"], # maple_bootloader
  ["0x1EAF", "0x0004"]  # maple_runtime
])
board_config.update("build.address", "0x%08x" % build_address)
board_config.update("build.offset",  "0x%08x" % build_offset)
board_config.update("upload.offset_address",  "0x%08x" % build_address)

if "BUILD_UNFLAGS" not in env:
  env.Replace(BUILD_UNFLAGS=[])

build_unflags = env['BUILD_UNFLAGS']
build_unflags.append('-DVECT_TAB_OFFSET=0x0')
build_unflags.append('-Wl,--defsym=LD_FLASH_OFFSET=0x0')
env.Replace(BUILD_UNFLAGS=build_unflags)

if "BUILD_FLAGS" not in env:
  env.Replace(BUILD_FLAGS=[])

build_flags = env['BUILD_FLAGS']
build_flags.append('-D BL_HID')
build_flags.append('-D VECT_TAB_OFFSET=0x%x' % build_offset)
build_flags.append('-Wl,--defsym=LD_FLASH_OFFSET=0x%x' % build_offset)
env.Replace(BUILD_FLAGS=build_flags)

# board_config.update("upload.require_upload_port", "false")
# board_config.update("upload.use_1200bps_touch", "false")
# board_config.update("upload.wait_for_upload_port", "false")

# print(env.Dump())
# env.Exit(1)
