# FOTA updates for IoT Devices

This repository contains source code to enable FOTA updates on an ESP32-WROOM development board. It consists of 4 different projects, each one a different stage of the finished product.

## Projects

### App-Blinky
This basic Zephyr project contains everything needed to build and flash a Blinky program onto an ESP32 WROOM develpoment board. This program will toggle an LED connected to GPIO pin 23 every second.

### App-MCUBoot
This project extends the Blinky program to be compatible with the MCUBoot bootloader. 

### App-MCUmgr
This project is an extension of App-MCUBoot. It provides a Blinky program compatible with MCUBoot, that also runs an MCUmgr server. Allowing communication to be made between the MCUmgr CLI and the board itself. This allows for 
firmware updates to be performed over UART, using the MCUmgr tool

### App-HawkBit
This is the final version f the project. It contains the Blnky application that is capable of establishing a wireless connection, and polling the hawkBit update server - as well as receiving and donwloading nay updates issued. 
It also contains safeguards to prevent against device bricking if something goes wrong - namely a Watchdog Timer and functionality to allow rollbacks to the most recent stable firmware version.

## Instructions for Building and Flashing
To build one of the projects, the directory of the chosen projectmust first be renamed to ```app```.

From the directory of ```app``` inside a command prompt, the following must be run to initialize the west workspace
```
west init -l
cd ../
west update
```
After the workspace has downloaded all dependencies. The chosen project can be built by running
```
cd app
west build -b esp32_devkitc_wroom --pritine
```
The binary image can thne be flashed to the target device, assuming a physical connection exists between it and the PC, by running
```
west flash
```

## Additional instructions for flashing MCUBoot
In order to run any of the projects tha are compatible with MCUBoot, the MCUBoot bootloader must first be built and flashed. This is done by running the following commands inside the ```app``` directory
```
west build -b esp32_devkitc_wroom -d build-mcuboot bootloader/mcuboot/boot/zephyr --prisitne -- -DEXTRA_CONF_FILE=’../../../../configurations/mcuboot.conf’
west flash -d build-mcuboot
```


