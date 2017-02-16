#! /bin/bash
# Author: chenkaiyao <chenkaiyao@molmc.com>
source ./release-version.sh

function askForContinue() {
    read ANS
    case $ANS in
        n|N|no|NO)
            echo 0
            ;;
            *)
            echo 1
            ;;
    esac
}

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

RELEASE_DIR=$(cd "$(dirname "$0")"; pwd)
PROJECT_DIR=$RELEASE_DIR/..
BUILD_DIR=$PROJECT_DIR/build
RELEASE_ONLINE_PROGRAM_DIR=$RELEASE_DIR/release-online-program/base_intorobot_$SYSTEM_VERSION_STRING
RELEASE_COMMON_ONLINE_PROGRAM_DIR=$RELEASE_DIR/online-program-common

rm -rf $RELEASE_ONLINE_PROGRAM_DIR

release_online_program_package() {
    case $1 in
################创客核心板####################
        neutron )
            PLATFORM_ID=888002
            ;;

        nut )
            PLATFORM_ID=888003
            ;;

        atom )
            PLATFORM_ID=888004
            ;;

        fig )
            PLATFORM_ID=888005
            ;;

        lora )
            PLATFORM_ID=888007
            ;;

        #add 此处添加创客核心板

################商业模块####################
        w67 )
            PLATFORM_ID=888101
            ;;

        w323 )
            PLATFORM_ID=888102
            ;;

        l6 )
            PLATFORM_ID=888103
            ;;

        #add  此处添加模块

################其他####################
        * )
            cecho "ERROR: Invalid selection." $red
            exit 1
            ;;
    esac

    cecho "Release $1 online-program-package" $yellow
    cecho "---------------------------------------------------------------------------" $yellow

    RELEASE_BOARD_DIR=$RELEASE_ONLINE_PROGRAM_DIR/$1
    INC_HAL=$RELEASE_BOARD_DIR/inc/hal
    INC_PLATFORM=$RELEASE_BOARD_DIR/inc/platform
    INC_SERVICES=$RELEASE_BOARD_DIR/inc/services
    INC_SYSTEM=$RELEASE_BOARD_DIR/inc/system
    INC_WIRING=$RELEASE_BOARD_DIR/inc/wiring
    INC_WIRING_EX=$RELEASE_BOARD_DIR/inc/wiring_ex
    INC_USER=$RELEASE_BOARD_DIR/inc/user
    LINKER=$RELEASE_BOARD_DIR/linker
    STARTUP=$RELEASE_BOARD_DIR/startup
    LIB=$RELEASE_BOARD_DIR/lib

    mkdir -p $RELEASE_BOARD_DIR
    mkdir -p $INC_HAL
    mkdir -p $INC_PLATFORM
    mkdir -p $INC_SERVICES
    mkdir -p $INC_SYSTEM
    mkdir -p $INC_WIRING
    mkdir -p $INC_WIRING_EX
    mkdir -p $INC_USER
    mkdir -p $LINKER
    mkdir -p $STARTUP
    mkdir -p $LIB

    cd $PROJECT_DIR
    cecho "build $1-boot.bin default-$1.bin files" $yellow
    make -s PLATFORM=$1 clean all COMPILE_LTO=n APP=smartLight-default

    # public
    # cp lib
    cp -rf $BUILD_DIR/target/hal/platform-$PLATFORM_ID/libhal.a $LIB
    cp -rf $BUILD_DIR/target/platform/platform-$PLATFORM_ID/libplatform.a $LIB
    cp -rf $BUILD_DIR/target/services/platform-$PLATFORM_ID/libservices.a $LIB
    cp -rf $BUILD_DIR/target/system/platform-$PLATFORM_ID/libsystem.a $LIB
    cp -rf $BUILD_DIR/target/wiring/platform-$PLATFORM_ID/libwiring.a $LIB
    cp -rf $BUILD_DIR/target/wiring_ex/platform-$PLATFORM_ID/libwiring_ex.a $LIB

    # cp inc
    cp -rf $PROJECT_DIR/hal/inc/*.h $INC_HAL
    cp -rf $PROJECT_DIR/hal/inc/variants/$1/* $INC_HAL
    cp -rf $PROJECT_DIR/hal/shared/*.h $INC_HAL
    cp -rf $PROJECT_DIR/services/inc/* $INC_SERVICES
    cp -rf $PROJECT_DIR/system/inc/* $INC_SYSTEM
    cp -rf $PROJECT_DIR/wiring/inc/* $INC_WIRING
    cp -rf $PROJECT_DIR/wiring_ex/$1/inc/* $INC_WIRING_EX
    cp -rf $PROJECT_DIR/user/inc/* $INC_USER
    cp -rf $PROJECT_DIR/platform/shared/inc/* $INC_PLATFORM

    if [ "$1"x = "atom"x ];then
        # cp linker
        cp -rf $BUILD_DIR/linker/arm/atom/* $LINKER
        # cp startup.o
        cp -rf $BUILD_DIR/target/main/platform-$PLATFORM_ID/startup/arm/atom/* $STARTUP
        # cp platform inc
        cp -rf $PROJECT_DIR/platform/MCU/shared/STM32/inc/* $INC_PLATFORM
        cp -rf $PROJECT_DIR/platform/MCU/STM32F1xx/CMSIS/Include/*  $INC_PLATFORM
        cp -rf $PROJECT_DIR/platform/MCU/STM32F1xx/CMSIS/Device/ST/STM32F1xx/Include/* $INC_PLATFORM
        cp -rf $PROJECT_DIR/platform/MCU/STM32F1xx/IntoRobot_Firmware_Driver/inc/* $INC_PLATFORM
        cp -rf $PROJECT_DIR/platform/MCU/STM32F1xx/STM32F1xx_HAL_Driver/Inc/* $INC_PLATFORM
        cp -rf $PROJECT_DIR/platform/MCU/STM32F1xx/STM32_USB_Device_Library/Class/CDC/Inc/* $INC_PLATFORM
        cp -rf $PROJECT_DIR/platform/MCU/STM32F1xx/STM32_USB_Device_Library/Class/DFU/Inc/* $INC_PLATFORM
        cp -rf $PROJECT_DIR/platform/MCU/STM32F1xx/STM32_USB_Device_Library/Core/Inc/* $INC_PLATFORM

    elif [ "$1"x = "neutron"x ]; then
        # cp startup.o
        cp -rf $BUILD_DIR/target/main/platform-$PLATFORM_ID/startup/arm/neutron/* $STARTUP
        # cp linker
        cp -rf $BUILD_DIR/linker/arm/neutron/* $LINKER
        cp -rf $PROJECT_DIR/newlib_nano/src/custom-nano.specs $LINKER
        # cp platform inc
        cp -rf $PROJECT_DIR/platform/MCU/shared/STM32/inc/* $INC_PLATFORM
        cp -rf $PROJECT_DIR/platform/MCU/STM32F4xx/CMSIS/Include/*  $INC_PLATFORM
        cp -rf $PROJECT_DIR/platform/MCU/STM32F4xx/CMSIS/Device/ST/STM32F4xx/Include/* $INC_PLATFORM
        cp -rf $PROJECT_DIR/platform/MCU/STM32F4xx/IntoRobot_Firmware_Driver/inc/* $INC_PLATFORM
        cp -rf $PROJECT_DIR/platform/MCU/STM32F4xx/STM32F4xx_HAL_Driver/Inc/* $INC_PLATFORM
        cp -rf $PROJECT_DIR/platform/MCU/STM32F4xx/STM32_USB_Device_Library/Class/CDC/Inc/* $INC_PLATFORM
        cp -rf $PROJECT_DIR/platform/MCU/STM32F4xx/STM32_USB_Device_Library/Class/DFU/Inc/* $INC_PLATFORM
        cp -rf $PROJECT_DIR/platform/MCU/STM32F4xx/STM32_USB_Device_Library/Core/Inc/* $INC_PLATFORM

        cp -rf $PROJECT_DIR/wiring_ex/neutron/src/sensors/inc/* $INC_WIRING_EX

        # freeRTOS header files. Todo remove
        #cp -rf $PROJECT_DIR/hal/src/stm32f4xx-share/rtos/FreeRTOSv9.0.0/FreeRTOS/Source/include/* $INC_HAL
        #cp -rf $PROJECT_DIR/hal/src/stm32f4xx-share/rtos/FreeRTOSv9.0.0/FreeRTOS/Source/portable/GCC/ARM_CM4F/*.h $INC_HAL

        # cp lib
        cp -rf $BUILD_DIR/target/newlib_nano/platform-$PLATFORM_ID/libnewlib_nano.a $LIB
        cp -rf $PROJECT_DIR/wiring_ex/neutron/src/lib/libPDMFilter_CM4_GCC.a $LIB/

    elif [ "$1"x = "nut"x ]; then
        # cp linker
        cp -rf $BUILD_DIR/linker/esp8266/nut/* $LINKER
        # cp platform inc
        cp -rf $PROJECT_DIR/platform/MCU/ESP8266-Arduino/IntoRobot_Firmware_Driver/inc/* $INC_PLATFORM
        cp -rf $PROJECT_DIR/platform/MCU/ESP8266-Arduino/sdk/include/* $INC_PLATFORM
        cp -rf $PROJECT_DIR/platform/MCU/ESP8266-Arduino/sdk/lwip/include/* $INC_PLATFORM

        # esp8266 private. Todo remove
        #cp $PROJECT_DIR/hal/src/esp8266-share/*.h $INC_HAL
        cp $PROJECT_DIR/hal/src/esp8266-share/esp8266/*.h $INC_HAL
        cp $PROJECT_DIR/hal/src/esp8266-share/esp8266/umm_malloc/*.h $INC_HAL

        # cp esp8266 libs
        mkdir -p $LIB/esp8266
        cp $PROJECT_DIR/platform/MCU/ESP8266-Arduino/sdk/lib/*.a $LIB/esp8266

    fi
}

if [ $# != 1 ];then
    cecho "Please enter the board to release online-program-package(请选择需要发布在线编程包的创客核心板或者商业模块:):" $yellow
    cecho "------------------------------------------------------------------------------------------------------------" $yellow
    cecho "Maker Boards (创客核心板): neutorn     nut     atom     fig     lora" $yellow
    cecho "Business Boards(商业模组): w67         w323    l6                   " $yellow
    cecho "Release all please input : all" $yellow
    cecho "Clean all please input   : clean" $yellow
    cecho "---------------------------------------------------------------------------" $yellow
    cecho "example: ./release_online_program.sh nut                                      " $yellow
    cecho "---------------------------------------------------------------------------" $yellow
    exit 1
fi
case $1 in
    all )
        cecho "Release all boards online-program-package" $yellow
        cecho "---------------------------------------------------------------------------" $yellow
        release_online_program_package neutron
        release_online_program_package nut
        #release_online_program_package atom
        #release_online_program_package fig
        #release_online_program_package lora

        #release_online_program_package w67
        #release_online_program_package w323
        #release_online_program_package l6
        cp -rf $RELEASE_COMMON_ONLINE_PROGRAM_DIR/base_intorobot/* $RELEASE_ONLINE_PROGRAM_DIR
        ;;

    clean )
        cecho "Clean the $SYSTEM_VERSION_STRING online-program-package [Y/n]:" $blue
        ans=$(askForContinue)
        if [ $ans == 1 ];then
            rm -rf $RELEASE_ONLINE_PROGRAM_DIR
            cecho "Clean the $SYSTEM_VERSION_STRING online-program-package done!!!" $green
            exit 0
        fi
        cecho "Clean the $SYSTEM_VERSION_STRING online-program-package failed!!!" $green
        exit 0
        ;;

    * )
        release_online_program_package $1
        cp -rf $RELEASE_COMMON_ONLINE_PROGRAM_DIR/base_intorobot/* $RELEASE_ONLINE_PROGRAM_DIR
        ;;
esac

cecho "Release boards online-program-package done!!!" $green
exit 0
