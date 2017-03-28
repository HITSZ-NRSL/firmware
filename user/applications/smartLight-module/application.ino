/*
 * w67 默认程序
 */

//SerialUSBDebugOutput debugOutput(115200, ALL_LEVEL);
//SerialDebugOutput debugOutput(115200, ALL_LEVEL);

//PRODUCT_MODE(PRODUCT_MODE_SLAVE)   //模块处于主模式还是从模式
PRODUCT_ID(NTxkPo6yxW3UetAFnpaveYCdbHGSDAfd)     //产品ID
PRODUCT_SECRET(5e51611f582027dd5239f6f39d6c1eb4) //产品密钥
PRODUCT_VERSION(2)     //产品版本号

#define SMARTLIGHT_CMD_SWITCH    "channel/smartLight_0/cmd/switch"   //开关命令
#define SMARTLIGHT_DATA_STATUS   "channel/smartLight_0/data/status"  //开关状态

#if PLATFORM_ID == PLATFORM_W67
#define KEYPIN    GPIO0     //定义按键引脚
#define LEDPIN    GPIO16    //定义灯泡控制引脚
#elif PLATFORM_ID == PLATFORM_W323
#define KEYPIN    GPIO0     //定义按键引脚
#define LEDPIN    GPIO4     //定义灯泡控制引脚
#endif

void smartLightSwitchCb(uint8_t *payload, uint32_t len)
{
    if(payload[0] == '1') {
        digitalWrite(LEDPIN, LOW);     //nut 低电平 打开灯泡
        IntoRobot.publish(SMARTLIGHT_DATA_STATUS,"1");
    } else {
        digitalWrite(LEDPIN, HIGH);      //nut 高电平 关闭灯泡
        IntoRobot.publish(SMARTLIGHT_DATA_STATUS,"0");
    }
}

void systemConfigKeyDeal()
{
    if(SYSTEM_CONFIG_TYPE_IMLINK_SERIAL != System.configStatus()) {
        System.configBegin(SYSTEM_CONFIG_TYPE_IMLINK_SERIAL);
        digitalWrite(LEDPIN, LOW);
    } else {
        System.configEnd();
        digitalWrite(LEDPIN, HIGH);
    }
}

void setup()
{
    //初始化
    pinMode(LEDPIN, OUTPUT);
    if(SYSTEM_CONFIG_TYPE_IMLINK_SERIAL == System.configStatus()) {
        digitalWrite(LEDPIN, LOW);
    } else {
        digitalWrite(LEDPIN, HIGH);
    }
    attachInterrupt(KEYPIN, systemConfigKeyDeal, CHANGE);
    //接收灯开关命令
    IntoRobot.subscribe(SMARTLIGHT_CMD_SWITCH, NULL, smartLightSwitchCb);
}

void loop()
{
    digitalWrite(LEDPIN,1);
    delay(1000);
    digitalWrite(LEDPIN,0);
    delay(1000);
}

