/*
 ************************************************************************
 * 作者:  IntoRobot Team
 * 版本:  V1.0.0
 * 日期:  03-30-15
 ************************************************************************
 功能描述：
 幻彩灯条：控制幻彩灯条的灯光效果

 所需器件:
 1. RGB LED灯条（灯珠型号为5050，灯条所需电流较大需单独供电，建议5V2A电源）

 接线说明
 RGB LED灯条           核心板
 VDD(红色)             接外置电源5V
 GND(白色)             GND
 DATA(绿色)            D0

 说明：带有“D”的为数字管脚，带有“A”的为模拟管脚，接线时请确认核心板引脚，避免接线错误。
 */
SerialDebugOutput debugOutput(115200, ALL_LEVEL);

#include "Adafruit_NeoPixel.h"

//幻彩灯条应用topic定义
#define  LEDLIGHTBAR_CMD_CONTROL    "channel/LEDLightBar_0/cmd/control"     //控制数据
#define  LEDLIGHTBAR_DATA_STATUS    "channel/LEDLightBar_0/data/status"  //控制状态

#define PIXEL_PIN       D0      //定义数据引脚
#define PIXEL_COUNT     30      //定义RGB LED的数量
#define PIXEL_TYPE      WS2812B //定义RGB LED的驱动类型

Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);

int bright = 10;  //设置亮度

typedef enum
{
    USR_CTL = 0, //用户控制模式
    HORSE_RACE,  //跑马灯模式
    RAINBOW,     //彩虹灯模式
    THUNDERBOLT, //霹雳灯模式
    CHRISTMAS,   //圣诞灯模式
    BLINK,       //闪烁模式
    RANDOM,      //随机模式
}run_type_t;

typedef struct  mo_colour_bar_s
{
    run_type_t m_run_type;	    //模式选择
    unsigned int m_colour;	    //RGB颜色
    unsigned char m_led_long;	//RGB LED数量
    unsigned char m_brightness;	//亮度值 0-255
}mo_colour_bar_t;

mo_colour_bar_t mo_colour_bar;

uint32_t Wheel(byte WheelPos)
{
    if(WheelPos < 85)
    {
        return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
    }
    else if(WheelPos < 170)
    {
        WheelPos -= 85;
        return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
    }
    else
    {
        WheelPos -= 170;
        return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
    }
}

//彩虹灯闪烁模式
void rainbow(uint8_t wait)
{
    uint16_t i, j;

    static int text_flag=0;

    if(text_flag==0)
    {
        for (j = 0; j < 64; j++)
        {
            for (i = 0; i < strip.numPixels(); i++)
            {
                strip.setPixelColor(i, Wheel((i + j) & 255));
            }

            strip.show();
            delay(wait);
        }

        text_flag=1;
    }
    else if(text_flag==1)
    {
        for (j = 64; j < 128; j++)
        {
            for (i = 0; i < strip.numPixels(); i++)
            {
                strip.setPixelColor(i, Wheel((i + j) & 255));
            }

            strip.show();
            delay(wait);
        }

        text_flag=2;
    }
    else if(text_flag==2)
    {
        for (j = 128; j < 192; j++)
        {
            for (i = 0; i < strip.numPixels(); i++)
            {
                strip.setPixelColor(i, Wheel((i + j) & 255));
            }

            strip.show();
            delay(wait);
        }

        text_flag=3;
    }
    else if(text_flag==3)
    {
        for (j = 192; j < 256; j++)
        {
            for (i = 0; i < strip.numPixels(); i++)
            {
                strip.setPixelColor(i, Wheel((i + j) & 255));
            }

            strip.show();
            delay(wait);
        }

        text_flag=0;
    }
}

//跑马灯闪烁模式
void larson(int wait)
{
    for (int d = 0; d < 1 ; d++)
    {
        // how many times it flashe
        for (int i = 0; i < strip.numPixels(); i++)
        {
            strip.setPixelColor(i, 255, 0, 0);
            strip.show();
            delay(wait);
            strip.setPixelColor(i, 0, 0, 0);
            strip.show();
            delay(wait);
        }

        for (int i = 2; i < strip.numPixels(); i++)
        {
            strip.setPixelColor(strip.numPixels() - i, 255, 0, 0);
            strip.show();
            delay(wait);
            strip.setPixelColor(strip.numPixels() - i, 0, 0, 0);
            strip.show();
            delay(wait);
        }
    }
}

void colorWipe_ThreeAround(int r, int g, int b, int wait)
{
    for (int i = 0; i < strip.numPixels(); i++)
    {
        int randr = random(0, 20);
        int randb = random(0, 20);
        strip.setPixelColor(i, strip.Color(255, 0, 0));
        strip.setPixelColor(i + 10, strip.Color(0, 255, 0));
        strip.setPixelColor((60 - (i)), strip.Color(0, 0, 255));
        strip.show();
        delay(wait);
        strip.setPixelColor(i + 15, strip.Color(0, 0, 0));
        strip.setPixelColor((i - 5), strip.Color(0, 0, 0));		// make sure the i-number is the amount of the trail
        strip.setPixelColor((60 - (i - 5)), strip.Color(0, 0, 0));
        strip.show();
        delay(wait);
    }
}

//霹雳灯闪烁模式
void thunderboltRGB(void)
{
#define DELAY_BOLT 2
    colorWipe_ThreeAround(255, 0, 0, DELAY_BOLT);	// Red
    colorWipe_ThreeAround(255, 0, 0, DELAY_BOLT);	// Red
    colorWipe_ThreeAround(255, 0, 0, DELAY_BOLT);	// Red
}

//点亮整个灯条
void colorAll(uint32_t c, uint8_t wait)
{
    uint16_t i;

    for (i = 0; i < strip.numPixels(); i++)
    {
        strip.setPixelColor(i, c);
    }

    strip.show();
    delay(wait);
}

//设置灯条亮度
void set_bright(unsigned char level)
{
    static unsigned s_level_old=0x00;

    if(level!=s_level_old)
    {
        strip.setBrightness(level);		//Because it has been determined whether the change here under no judgment
        strip.show();
    }

    s_level_old=level;
}

//用户控制模式
void color_n(uint32_t c, uint8_t n)
{
    static uint32_t s_colour_old = 0x00;
    static uint32_t s_num_led_old = 0x00;
    uint16_t i;

    if((c != s_colour_old) || (n != s_num_led_old))		//Only a change in color or amount of just set down to prevent flicker
    {
        for (i = n; i < strip.numPixels(); i++)
        {
            strip.setPixelColor(i, 0x00);
        }

        strip.show();
        delay(4);

        for (i = 0; i < n; i++)
        {
            strip.setPixelColor(i, c);
        }

        strip.show();
        delay(4);

        s_colour_old = c;
        s_num_led_old = n;
    }
}

//闪烁灯模式
void alternateRGB(u8 wait)
{
    colorAll(strip.Color(255, 0, 0), wait);
    colorAll(strip.Color(0, 255, 0), wait);
    colorAll(strip.Color(0, 0, 255), wait);
    colorAll(strip.Color(255, 255, 0), wait);
    colorAll(strip.Color(0, 255, 255), wait);
    colorAll(strip.Color(255, 0, 255), wait);
    colorAll(strip.Color(255, 255, 255), wait);
}


void colorWipe_xmas(uint32_t c, uint8_t wait)
{
    for (uint16_t i = 0; i < strip.numPixels(); i++)
    {
        strip.setPixelColor(i, c);
        strip.show();
        delay(wait);
    }
}

//圣诞灯模式
void xmas(void)
{
#define DELAY_XMAS 4
    static int text_flag=0;

    if(text_flag==0)
    {
        colorWipe_xmas(strip.Color(255, 0, 0), DELAY_XMAS);
        colorWipe_xmas(strip.Color(255, 127, 0), DELAY_XMAS);
        colorWipe_xmas(strip.Color(255, 255, 0), DELAY_XMAS);

        text_flag=1;
    }
    else if(text_flag==1)
    {
        colorWipe_xmas(strip.Color(0, 255, 0), DELAY_XMAS);
        colorWipe_xmas(strip.Color(0, 0, 255), DELAY_XMAS);
        colorWipe_xmas(strip.Color(75, 0, 130), DELAY_XMAS);

        text_flag=2;
    }
    else if(text_flag==2)
    {
        colorWipe_xmas(strip.Color(143, 0, 255), DELAY_XMAS);
        colorWipe_xmas(strip.Color(200, 200, 200), DELAY_XMAS);
        colorWipe_xmas(strip.Color(143, 0, 255), DELAY_XMAS);

        text_flag=3;
    }
    else if(text_flag==3)
    {
        colorWipe_xmas(strip.Color(75, 0, 130), DELAY_XMAS);
        colorWipe_xmas(strip.Color(0, 0, 255), DELAY_XMAS);
        colorWipe_xmas(strip.Color(0, 255, 0), DELAY_XMAS);

        text_flag=4;
    }
    else if(text_flag==4)
    {
        colorWipe_xmas(strip.Color(255, 255, 0), DELAY_XMAS);
        colorWipe_xmas(strip.Color(255, 127, 0), DELAY_XMAS);
        text_flag=0;
    }
}

void colorWipe_Cylon(int r, int g, int b, int wait)
{
    for (int i = 0; i < strip.numPixels(); i++)
    {
        int randr1 = r - (5 * bright);
        if (randr1 < 1) randr1 = 5 + bright;
        int randg1 = g - (5 * bright);
        if (randg1 < 1) randg1 = 5 + bright;
        int randb1 = b - (5 * bright);
        if (randb1 < 1) randb1 = 5 + bright;
        int randr2 = r - (10 * bright);
        if (randr2 < 1) randr2 = 2 + bright;
        int randg2 = g - (10 * bright);
        if (randg2 < 1) randg2 = 2 + bright;
        int randb2 = b - (10 * bright);
        if (randb2 < 1) randb2 = 2 + bright;
        int randr3 = r - (20 * bright);
        if (randr3 < 1) randr3 = 1 + bright;
        int randg3 = g - (20 * bright);
        if (randg3 < 1) randg3 = 1 + bright;
        int randb3 = b - (20 * bright);
        if (randb3 < 1) randb3 = 1 + bright;
        strip.setPixelColor(i, r, g, b);
        strip.setPixelColor(i - 1, randr1, randg1, randb1);
        strip.setPixelColor(i - 2, randr2, randg2, randb2);
        strip.setPixelColor(i - 3, randr3, randg3, randb3);		// add more of these lines to make the trail longer
        strip.show();
        delay(wait);
        strip.setPixelColor(i - 4, strip.Color(0, 0, 0));		// make sure the i-number is the amount of the trail
        strip.show();
        delay(wait);
    }
}

//随机灯模式
void randomCylon(int wait)
{
    int randr = random(0, (25 * bright));
    int randg = random(0, (25 * bright));
    int randb = random(0, (25 * bright));
    int randi = random(1, strip.numPixels());
    int randii = random(1, strip.numPixels());
    colorWipe_Cylon(randr, randg, randb, wait);
}

//灯条设置回调
void neopixelSetCb(uint8_t * payload, uint32_t len)
{
    aJsonClass aJson;
    aJsonObject *root = aJson.parse((char *)payload);
    if(root == NULL)
    {
        aJson.deleteItem(root);
        return;
    }

    aJsonObject *_ledMode = aJson.getObjectItem(root, "theme");
    if(_ledMode == NULL)
    {
        aJson.deleteItem(root);
        return;
    }
    mo_colour_bar.m_run_type = (run_type_t)atoi(_ledMode->valuestring);

    aJsonObject *_ledLength = aJson.getObjectItem(root, "length");
    if(_ledLength == NULL)
    {
        aJson.deleteItem(root);
        return;
    }
    mo_colour_bar.m_led_long = atoi(_ledLength->valuestring);

    aJsonObject *_ledBrightness = aJson.getObjectItem(root, "brightness");
    if(_ledBrightness == NULL)
    {
        aJson.deleteItem(root);
        return;
    }
    mo_colour_bar.m_brightness = atoi(_ledBrightness->valuestring);

    aJsonObject *_ledRgb = aJson.getObjectItem(root, "color");
    if(_ledRgb == NULL)
    {
        aJson.deleteItem(root);
        return;
    }
    mo_colour_bar.m_colour = atoi(_ledRgb->valuestring);

    aJson.deleteItem(root);
    set_bright((mo_colour_bar.m_brightness) * 5/2);
}

void setup()
{
    //初始化灯条
    strip.begin();
    strip.setBrightness(100);
    strip.show();
    //初始设置灯条状态
    mo_colour_bar.m_run_type = HORSE_RACE;
    mo_colour_bar.m_colour = 0x00ff0000;
    mo_colour_bar.m_led_long = 50;
    mo_colour_bar.m_brightness = 40;
    //接收灯条控制
    IntoRobot.subscribe(LEDLIGHTBAR_CMD_CONTROL, NULL, neopixelSetCb);
}

void loop()
{
    switch(mo_colour_bar.m_run_type)
    {
        case HORSE_RACE:  //跑马灯
            larson(50);
            break;
        case RAINBOW:     //彩虹灯
            rainbow(10);
            break;
        case THUNDERBOLT: //霹雳灯
            thunderboltRGB();
            break;
        case CHRISTMAS:   //圣诞灯
            xmas();
            break;
        case BLINK:       //闪烁灯
            alternateRGB(500);
            break;
        case RANDOM:      //随机灯
            randomCylon(10);
            break;
        case USR_CTL:     //用户控制
            color_n(mo_colour_bar.m_colour, (mo_colour_bar.m_led_long)*3/5);
            break;
        default:
            break;
    }
}
