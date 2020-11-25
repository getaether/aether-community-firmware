import sys
import os

from zipfile import ZipFile

assert(len(sys.argv) == 3)

version = sys.argv[1]
root_folder = sys.argv[2]

files = [
	"ota_data_initial.bin",
	"bootloader/bootloader.bin",
	"esp-aether.bin",
	"partitions.bin",
]

script_path = root_folder + "flash.sh"

command = '''
python /Users/pastre/esp/esp-idf/components/esptool_py/esptool/esptool.py \
  --chip esp32\
  --port $1\
  --baud 115200\
  --before default_reset \
  --after hard_reset write_flash \
  -z \
  --flash_mode dio \
  --flash_freq 40m \
  --flash_size detect \
  	0xd000 ./build/ota_data_initial.bin \
  	0x1000 ./build/bootloader/bootloader.bin \
  	0x10000 ./build/esp-aether.bin \
  	0x8000 ./build/partitions.bin
'''

zipObj = ZipFile(f'esp-aether-v{version}.zip', 'w')

open(script_path, 'w').write(command)

for fileName in files: zipObj.write(root_folder + "build/" + fileName )

zipObj.write(script_path)

zipObj.close()

os.remove(script_path)
