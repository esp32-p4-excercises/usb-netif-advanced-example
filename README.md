## Compatibility
This app is compatible with esp32 SoC supporting hardware USB like S2/S3/P4.

## Description

It is base app with simple web UI which allow to flash binary into selected OTA partition (we have currently 2 ota partitions) and we can also upload files into fat partition, which is mostly designated to keep web UI files. This way it is possible to update web UI files without the need to reflash whole app.

In addition by default its enabled rollback feature which let to boot from factory app every time esp32 reboot.

## Usage
When we connect USB to PC/laptop esp32 is detected as ethernet and can be accessed from web browser on address [192.168.4.1](http://192.168.4.1). 

Currently index page is all-in-one to flash other binaries or to select which partition should we boot from.

When tool firmware is flashed into OTA 0 or 1 partition it satys in there until new binary is flashed on it and we can select to run that partition firmware anytime we want without need to flash it again.


## Links
- [index.html](http://192.168.4.1/)
- [upload](http://192.168.4.1/upload) or [upload.html](http://192.168.4.1/upload.html) - we have `/upload` file which can't be changed in flash, just in case we brake something in UI files
- [reset](http://192.168.4.1/reset) - we can reset device from web UI when we select to boot from `App 1` or `App 2`
- [boot partition app0](http://192.168.4.1/boot/app0) - can store tool firmware
- [boot partition app1](http://192.168.4.1/boot/app1) - can store another firmware
- an file uplaoded/stered in fat partition can be open with `http://192.168.4.1/file_name`, so we can upload html, js and css files if we want