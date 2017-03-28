/*
 * 智能灯程序
 */

SerialDebugOutput debugOutput(115200, ALL_LEVEL);

#define SMARTLIGHT_CMD_SWITCH    "channel/smartLight_0/cmd/switch"   //开关命令
#define SMARTLIGHT_DATA_STATUS   "channel/smartLight_0/data/status"  //开关状态

#define LEDPIN    LED_USER    //定义灯泡控制引脚

void smartLightSwitchCb(uint8_t *payload, uint32_t len)
{
    if(payload[0] == '1')
    {
#if PLATFORM_ID == PLATFORM_ATOM || PLATFORM_ID == PLATFORM_NEUTRON
        digitalWrite(LEDPIN, HIGH);     // 打开灯泡
#else
        digitalWrite(LEDPIN, LOW);     // 打开灯泡
#endif
        IntoRobot.publish(SMARTLIGHT_DATA_STATUS,"1");
    }
    else
    {
#if PLATFORM_ID == PLATFORM_ATOM || PLATFORM_ID == PLATFORM_NEUTRON
        digitalWrite(LEDPIN, LOW);     // 打开灯泡
#else
        digitalWrite(LEDPIN, HIGH);     // 打开灯泡
#endif
        IntoRobot.publish(SMARTLIGHT_DATA_STATUS,"0");
    }
}

void setup()
{
    //初始化
    pinMode(LEDPIN, OUTPUT);
    //接收灯开关命令
    IntoRobot.subscribe(SMARTLIGHT_CMD_SWITCH, NULL, smartLightSwitchCb);
}

void loop()
{
}
