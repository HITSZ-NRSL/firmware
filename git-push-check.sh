#! /bin/bash
# Author: zhang baoxian[baoxianzhang] <baoxianzhang.github.io>

if [ "$1" == "-s" ];then
VERBOSE=$1
else
VERBOSE=""
fi

export INTOROBOT_DEVELOP=1

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


cecho "github push check begin!!!" $yellow
cecho "------------------------创客核心板-------------------------" $yellow
cecho "---->neutron" $yellow
make $VERBOSE PLATFORM=neutron clean all COMPILE_LTO=n APP=smartLight-default

cecho "---->nut" $yellow
make $VERBOSE PLATFORM=nut clean all COMPILE_LTO=n APP=smartLight-default

cecho "---->atom" $yellow
make $VERBOSE PLATFORM=atom clean all COMPILE_LTO=n APP=smartLight-default

cecho "---->fig" $yellow
make $VERBOSE PLATFORM=fig clean all COMPILE_LTO=n APP=smartLight-default

cecho "---->ant" $yellow
make $VERBOSE PLATFORM=ant clean all COMPILE_LTO=n APP=blink

cecho "---->fox" $yellow
make $VERBOSE PLATFORM=fox clean all COMPILE_LTO=n APP=smartLight-default

cecho "------------------------商业模块-------------------------" $yellow
cecho "---->w67" $yellow
make $VERBOSE PLATFORM=w67 clean all COMPILE_LTO=n APP=smartLight-module

cecho "---->w323" $yellow
make $VERBOSE PLATFORM=w323 clean all COMPILE_LTO=n APP=smartLight-module

cecho "---->l6" $yellow
make $VERBOSE PLATFORM=l6 clean all COMPILE_LTO=n APP=blink
cecho "------------------------------------------------------------" $yellow

cecho "------------------------其他-------------------------" $yellow
cecho "---->gcc" $yellow
#make $VERBOSE PLATFORM=gcc clean all COMPILE_LTO=n

cecho "---->neutron-net" $yellow
make $VERBOSE PLATFORM=neutron-net clean all COMPILE_LTO=n

cecho "---->anytest" $yellow
make $VERBOSE PLATFORM=anytest clean all COMPILE_LTO=n APP=default-anytest

cecho "---->gl1000" $yellow
make $VERBOSE PLATFORM=gl1000 clean all COMPILE_LTO=n APP=blank

cecho "------------------------------------------------------------" $yellow
cecho "github push check success!!!" $yellow

exit 0
