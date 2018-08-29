#if 0
// pull in the sources from the HAL. It's a bit of a hack, but is simpler than trying to link the
// full hal library.
#include "../src/nut/ui_hal.c"

#else

#include "hw_config.h"
#include "ui_hal.h"

#define RGB_GPIO_PIN               5
#define MODE_BOTTON_GPIO_PIN       0

//extern void espShow(uint8_t pin, uint32_t color, uint8_t brightness);
extern void ICACHE_RAM_ATTR espShow(uint8_t pin, uint32_t color, uint8_t brightness);

volatile uint32_t BUTTON_last_state = 0;
volatile uint32_t TimingBUTTON=0;
volatile uint32_t TimingLED;
rgb_info_t rgb_info;

void Set_RGB_Color(uint32_t color) {
    espShow(RGB_GPIO_PIN, color, 25);
}

void RGB_Color_Toggle(void) {
    if(rgb_info.rgb_last_color) {
        Set_RGB_Color(0);
        rgb_info.rgb_last_color = 0;
    }
    else {
        Set_RGB_Color(rgb_info.rgb_color);
        rgb_info.rgb_last_color = rgb_info.rgb_color;
    }
}

void HAL_UI_Initial(void)
{
    // initial rgb pin
    GPF(RGB_GPIO_PIN) = GPFFS(GPFFS_GPIO(RGB_GPIO_PIN));
    GPC(RGB_GPIO_PIN) = (GPC(RGB_GPIO_PIN) & (0xF << GPCI));
    GPES = (1 << RGB_GPIO_PIN); //Enable
    // gpio write low
    GPOC = (1 << RGB_GPIO_PIN);

    // initial button pin
    GPF(MODE_BOTTON_GPIO_PIN) = GPFFS(GPFFS_GPIO(MODE_BOTTON_GPIO_PIN));
    GPEC = (1 << MODE_BOTTON_GPIO_PIN); //Disable
    GPC(MODE_BOTTON_GPIO_PIN) = (GPC(MODE_BOTTON_GPIO_PIN) & (0xF << GPCI)) | (1 << GPCD);
}

uint8_t HAL_UI_Mode_BUTTON_GetState(Button_TypeDef Button)
{
    return GPIP(MODE_BOTTON_GPIO_PIN);
}

uint32_t HAL_UI_Mode_Button_Pressed(void)
{
    return TimingBUTTON;
}

int HAL_UI_RGB_Get_Info(rgb_info_t *pinfo)
{
    memcpy(pinfo, &rgb_info, sizeof(rgb_info_t));
    return 0;
}

int HAL_UI_RGB_Set_Info(rgb_info_t info)
{
    memcpy(&rgb_info, &info, sizeof(rgb_info_t));
    return 0;
}

void HAL_UI_RGB_Color(uint32_t color)
{
    memset(&rgb_info, 0, sizeof(rgb_info));
    rgb_info.rgb_mode = RGB_MODE_COLOR;
    rgb_info.rgb_color = color;
    Set_RGB_Color(color);
}

void HAL_UI_RGB_Blink(uint32_t color, uint16_t period)
{
    memset(&rgb_info, 0, sizeof(rgb_info));
    rgb_info.rgb_mode = RGB_MODE_BLINK;
    rgb_info.rgb_color = color;
    rgb_info.rgb_period = period>100 ? period >> 1 : 50;
}

void HAL_UI_RGB_Breath(uint32_t color, uint16_t period)
{
    HAL_UI_RGB_Blink(color, period);
}

void HAL_UI_UserLED_Control(uint8_t value)
{
}

void HAL_UI_SysTick_Handler(void)
{
    //三色灯处理
    if(RGB_MODE_BLINK == rgb_info.rgb_mode)
    {
        if (TimingLED != 0x00)
        {
            TimingLED--;
        }
        else
        {
            RGB_Color_Toggle();
            TimingLED = rgb_info.rgb_period;
        }
    }
    //侧边按键处理
    if(!HAL_UI_Mode_BUTTON_GetState(BUTTON1))
    {
        if(!BUTTON_last_state) {
            TimingBUTTON = 0;
            BUTTON_last_state = 1;
        }
        else {
            TimingBUTTON++;
        }
    }
    else {
        if(BUTTON_last_state)
        {
            TimingBUTTON = 0;
            BUTTON_last_state = 1;
        }
    }
}

#endif
