#! /bin/bash
# Author: zhang baoxian[baoxianzhang] <baoxianzhang.github.io>

if [ "$1" == "-s" ];then
VERBOSE=$1
else
VERBOSE=""
fi

export INTOROBOT_DEVELOP=1

echo "github push check begin!!!"
echo "------------------------创客核心板-------------------------"
echo "!!!!!!!!!!!!!!!!!!!!!!!!!neutron!!!!!!!!!!!!!!!!!!!!!!!!!!!"
make $VERBOSE PLATFORM=neutron clean all COMPILE_LTO=n APP=smartLight-default

echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!nut!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
make $VERBOSE PLATFORM=nut clean all COMPILE_LTO=n APP=smartLight-default

echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!atom!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
make $VERBOSE PLATFORM=atom clean all COMPILE_LTO=n APP=smartLight-default

echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!fig!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
make $VERBOSE PLATFORM=fig clean all COMPILE_LTO=n APP=blink

echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!lora!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
make $VERBOSE PLATFORM=lora clean all COMPILE_LTO=n APP=blink

echo "!!!!!!!!!!!!!!!!!!!!!!!!littlebee!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
make $VERBOSE PLATFORM=littlebee clean all COMPILE_LTO=n APP=smartLight-default

echo "------------------------商业模块-------------------------"
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!w67!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
make $VERBOSE PLATFORM=w67 clean all COMPILE_LTO=n APP=smartLight-w67

echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!w323!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
make $VERBOSE PLATFORM=w323 clean all COMPILE_LTO=n APP=blank

echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!l6!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
make $VERBOSE PLATFORM=l6 clean all COMPILE_LTO=n APP=blink
echo "------------------------------------------------------------"

echo "------------------------其他-------------------------"
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!gcc!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
#make $VERBOSE PLATFORM=gcc clean all COMPILE_LTO=n

echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!neutron-net!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
make $VERBOSE PLATFORM=neutron-net clean all COMPILE_LTO=n

echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!anytest!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
make $VERBOSE PLATFORM=anytest clean all COMPILE_LTO=n APP=default-anytest

echo "------------------------------------------------------------"
echo "github push check success!!!"

exit 0
