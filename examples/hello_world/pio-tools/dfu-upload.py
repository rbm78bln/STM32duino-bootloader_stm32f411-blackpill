Import('env')
import os
import re
from platformio.device.finder import SerialPortFinder

def before_upload(source, target, env):
    print("Looking for upload port...")
    env.Replace(
        UPLOAD_PORT=SerialPortFinder(
            board_config=env.BoardConfig() if "BOARD" in env else None,
            upload_protocol=None,
            prefer_gdb_port=None,
            verbose=int(ARGUMENTS.get("PIOVERBOSE", 0)),
        ).find(None)
    )
    if env['UPLOAD_PORT'] != None:
        print("Entering bootloader...")
        env.Execute(env['UPLOADER_RESETTER']+" "+env['UPLOAD_PORT']+" 1250")

env.AddPreAction("upload", before_upload)

# print(env.Dump())

uploader = env['UPLOADER']
resetter = re.sub( r"tool-dfuutil.*$", "tool-stm32duino/upload-reset\"", uploader )

flags = " ".join(env['UPLOADERFLAGS'])
flags  = re.sub( r"-s.*-D", "-D", flags )
flags  = re.sub( r",0x1EAF.*0x.... ", " ", flags )
flags  = uploader+" -R "+flags
flags = flags.split()

env.Replace(UPLOADER_ORG=uploader)
env.Replace(UPLOADER_RESETTER=resetter)
env.Replace(UPLOADER=os.path.join("pio-tools", "dfu-upload.sh"))
env.Replace(UPLOADERFLAGS=flags)

# print(env.Dump())
