#! /bin/bash

# Color-echo.
# arg $1 = message
# arg $2 = Color
cecho() {
  echo -e "${2}${1}"
  tput sgr0
  # Reset # Reset to normal.
  return
}
# Set the colours you can use
red='\033[0;31m'
green='\033[0;32m'
yellow='\033[0;33m'
blue='\033[0;34m'

rm -rf gcc-arm-none-eabi
rm -rf xtensa-lx106-elf
rm -rf xtensa-esp32-elf
rm -rf stlink
rm -rf esptool
rm -rf esptool-py
rm -rf esp8266
rm -rf esp32

mkdir dist
cd dist

sysType=`uname -s`
cecho "Your system is $sysType " $blue
if [ $sysType = "Linux" ]; then
    cecho "--->install arm gnu toolchain " $blue
    wget https://launchpad.net/gcc-arm-embedded/4.9/4.9-2015-q3-update/+download/gcc-arm-none-eabi-4_9-2015q3-20150921-linux.tar.bz2
    tar -jxf gcc-arm-none-eabi-4_9-2015q3-20150921-linux.tar.bz2
    mv gcc-arm-none-eabi-4_9-2015q3 ../gcc-arm-none-eabi
    cecho "--->install success " $blue

    if [ $(getconf WORD_BIT) = '32' ] && [ $(getconf LONG_BIT) = '64' ] ; then
        cecho "Your system is 64 bit " $blue
        cecho "--->install xtensa lx106 gnu toolchain " $blue
        wget http://arduino.esp8266.com/linux64-xtensa-lx106-elf-gb404fb9.tar.gz
        tar -zxf linux64-xtensa-lx106-elf-gb404fb9.tar.gz
        mv  xtensa-lx106-elf ../xtensa-lx106-elf
        cecho "--->install success " $blue

        cecho "--->install xtensa esp32 toolchain " $blue
        wget https://dl.espressif.com/dl/xtensa-esp32-elf-linux64-1.22.0-61-gab8375a-5.2.0.tar.gz
        tar -zxf xtensa-esp32-elf-linux64-1.22.0-61-gab8375a-5.2.0.tar.gz
        mv  xtensa-esp32-elf ../xtensa-esp32-elf
        cecho "--->install success " $blue

        cecho "--->install st-flash " $blue
        wget http://www.intoyun.com/downloads/terminal/tools/stlink-1.2.0-linux64.tar.gz
        tar -zxf stlink-1.2.0-linux64.tar.gz
        mv stlink-1.2.0-linux64 ../stlink
        cecho "--->install success " $blue

        cecho "--->install esp8266 esptool " $blue
        wget http://www.intoyun.com/downloads/terminal/tools/esptool-0.4.11-linux64.tar.gz
        tar -zxf esptool-0.4.11-linux64.tar.gz
        mv esptool-0.4.11-linux64 ../esp8266
        cecho "--->install success " $blue

        cecho "--->install esp32 esptool " $blue
        wget http://www.intoyun.com/downloads/terminal/tools/esptool-fe69994-linux64.tar.gz
        tar -zxf esptool-fe69994-linux64.tar.gz
        mkdir ../esp32
        mv esptool ../esp32/
        cecho "--->install success " $blue

    else
        cecho "Your system is 32 bit " $blue
        cecho "--->install xtensa lx106 gnu toolchain " $blue
        wget http://arduino.esp8266.com/linux32-xtensa-lx106-elf.tar.gz
        tar -zxf linux32-xtensa-lx106-elf.tar.gz
        mv  xtensa-lx106-elf ../xtensa-lx106-elf
        cecho "--->install success " $blue

        cecho "--->install xtensa esp32 toolchain " $blue
        wget https://dl.espressif.com/dl/xtensa-esp32-elf-linux32-1.22.0-61-gab8375a-5.2.0.tar.gz
        tar -zxf xtensa-esp32-elf-linux32-1.22.0-61-gab8375a-5.2.0.tar.gz
        mv  xtensa-esp32-elf ../xtensa-esp32-elf
        cecho "--->install success " $blue

        cecho "--->install st-flash " $blue
        wget http://www.intoyun.com/downloads/terminal/tools/stlink-1.2.0-linux64.tar.gz
        tar -zxf stlink-1.2.0-linux32.tar.gz
        mv stlink-1.2.0-linux32 ../stlink
        cecho "--->install success " $blue

        cecho "--->install esp8266 esptool " $blue
        wget http://www.intoyun.com/downloads/terminal/tools/esptool-0.4.11-linux64.tar.gz
        tar -zxf esptool-0.4.11-linux64.tar.gz
        mv  esptool-0.4.11-linux64 ../esp8266
        cecho "--->install success " $blue

        cecho "--->install esp32 esptool " $blue
        wget http://www.intoyun.com/downloads/terminal/tools/esptool-fe69994-linux32.tar.gz
        tar -zxf esptool-fe69994-linux32.tar.gz
        mkdir ../esp32
        mv esptool ../esp32/
        cecho "--->install success " $blue

    fi
    cecho "--->install dfu-util " $blue
    sudo apt-get install dfu-util
    cecho "--->install success " $blue

elif [ $sysType = "Darwin" ]; then
    cecho "--->install arm gnu toolchain " $blue
    wget https://launchpad.net/gcc-arm-embedded/4.9/4.9-2015-q2-update/+download/gcc-arm-none-eabi-4_9-2015q2-20150609-mac.tar.bz2
    tar -jxf  gcc-arm-none-eabi-4_9-2015q2-20150609-mac.tar.bz2
    mv gcc-arm-none-eabi-4_9-2015q2 ../gcc-arm-none-eabi
    rm -rf gcc-arm-none-eabi-4_9-2015q2
    cecho "--->install success " $blue

    cecho "--->install xtensa lx106 gnu toolchain " $blue
    wget http://arduino.esp8266.com/osx-xtensa-lx106-elf-gb404fb9-2.tar.gz
    tar -zxf osx-xtensa-lx106-elf-gb404fb9-2.tar.gz
    mv xtensa-lx106-elf ../xtensa-lx106-elf
    cecho "--->install success " $blue

    cecho "--->install xtensa esp32 toolchain " $blue
    wget https://dl.espressif.com/dl/xtensa-esp32-elf-osx-1.22.0-61-gab8375a-5.2.0.tar.gz
    tar -zxf xtensa-esp32-elf-osx-1.22.0-61-gab8375a-5.2.0.tar.gz
    mv  xtensa-esp32-elf ../xtensa-esp32-elf
    cecho "--->install success " $blue

    cecho "--->install st-flash " $blue
    wget http://www.intoyun.com/downloads/terminal/tools/stlink-1.2.0-osx.tar.gz
    tar -zxf stlink-1.2.0-osx.tar.gz
    mv stlink-1.2.0-osx ../stlink
    cecho "--->install success " $blue

    cecho "--->install esp8266 esptool " $blue
    wget http://www.intoyun.com/downloads/terminal/tools/esptool-0.4.11-osx.tar.gz
    tar -zxf esptool-0.4.11-osx.tar.gz
    mv esptool-0.4.11-osx ../esp8266
    cecho "--->install success " $blue

    cecho "--->install esp32 esptool " $blue
    wget http://www.intoyun.com/downloads/terminal/tools/esptool-9072736-macos.tar.gz
    tar -zxf esptool-9072736-macos.tar.gz
    mkdir ../esp32
    mv esptool ../esp32/
    cecho "--->install success " $blue

    cecho "--->install dfu-util " $blue
    brew install dfu-util
    cecho "--->install success " $blue

else
    cecho "unsupported system, exit " $red
fi

cd ..

cecho "---------end  install  tools-------------" $blue

