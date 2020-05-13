# How to use the Particle Debugger
1. Wire up: Solder wires to the pads on the back. Connect SWCLK, SWIO, and GND. None others are necessary for now. Plug in both the particle debugger and the Feather to your computer.
2. Clear Bootloader: Download this tool (https://github.com/ataradov/edbg) and run the following command to unlock the bootloader:
```
edbg -F wv0,2:0,7 -t samd21
```
3. Install PyOCD globally: `pip install -U pyOCD`. Ensure it is in your path. Run `pyocd pack -i ATSAMD21G18A`.
4. If using platformio, the following custom board profile is necessary to link against no bootloader:
```JSON
{
    "build": {
        "arduino": {
            "ldscript": "flash_without_bootloader.ld"
        },
        "core": "adafruit",
        "cpu": "cortex-m0plus",
        "extra_flags": "-DARDUINO_ARCH_SAMD -DARDUINO_SAMD_ZERO -DARDUINO_SAMD_FEATHER_M0 -DARM_MATH_CM0PLUS -D__SAMD21G18A__",
        "f_cpu": "48000000L",
        "hwids": [
            [
                "0x239A",
                "0x800B"
            ],
            [
                "0x239A",
                "0x000B"
            ],
            [
                "0x239A",
                "0x0015"
            ]
        ],
        "mcu": "samd21g18a",
        "system": "samd",
        "usb_product": "Adafruit Feather M0",
        "variant": "feather_m0",
        "zephyr": {
            "variant": "adafruit_feather_m0_basic_proto"
        }
    },
    "debug": {
        "jlink_device": "ATSAMD21G18",
        "openocd_chipname": "at91samd21g18",
        "openocd_target": "at91samdXX",
        "svd_path": "ATSAMD21G18A.svd"
    },
    "frameworks": [
        "arduino",
        "zephyr"
    ],
    "name": "Adafruit Feather M0",
    "upload": {
        "disable_flushing": true,
        "maximum_ram_size": 32768,
        "maximum_size": 262144,
        "native_usb": true,
        "offset_address": "0x0000",
        "protocol": "sam-ba",
        "protocols": [
            "sam-ba",
            "blackmagic",
            "jlink",
            "atmel-ice",
            "cmsis-dap"
        ],
        "require_upload_port": true,
        "use_1200bps_touch": true,
        "wait_for_upload_port": true
    },
    "url": "https://www.adafruit.com/product/2772",
    "vendor": "Adafruit"
}
```
Additionally, the following changes to your config are needed:
```
debug_build_flags = -O1 -ggdb3 -g3
upload_protocol = cmsis-dap
debug_tool = custom
debug_server = pyocd
    gdbserver
    --pack=boards/Keil.SAMD21_DFP.1.3.0.pack 
    -t=atsamd21e18a
```
You will also need to download Keil.SAMD21_DFP.1.3.0.pack from here: https://developer.arm.com/embedded/cmsis/cmsis-packs/devices/Microchip/ATSAMD21G18A.