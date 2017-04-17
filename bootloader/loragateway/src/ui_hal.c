#if 0
// pull in the sources from the HAL. It's a bit of a hack, but is simpler than trying to link the
// full hal library.
#include "../src/neutron/ui_hal.c"

#else

#include "hw_config.h"
#include "ui_hal.h"


#define RGB_R_GPIO_PIN       GPIO_PIN_8
#define RGB_R_GPIO_PORT      GPIOA

#define MODE_BOTTON_GPIO_PIN       GPIO_PIN_15
#define MODE_BOTTON_GPIO_PORT      GPIOA


volatile uint32_t BUTTON_last_state = 0;
volatile uint32_t TimingBUTTON=0;
volatile uint32_t TimingLED;
rgb_info_t rgb_info;

void Set_RGB_Color(uint32_t color) {
    uint8_t red;

    red = color>>16 & 0xFF;
    if(red)
        HAL_GPIO_WritePin(RGB_R_GPIO_PORT, RGB_R_GPIO_PIN, GPIO_PIN_RESET);
    else
        HAL_GPIO_WritePin(RGB_R_GPIO_PORT, RGB_R_GPIO_PIN, GPIO_PIN_SET);
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
    //三色灯管脚初始化
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef  GPIO_InitStruct;
    GPIO_InitStruct.Pin = RGB_R_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    HAL_GPIO_Init(RGB_R_GPIO_PORT, &GPIO_InitStruct);

    HAL_GPIO_WritePin(RGB_R_GPIO_PORT, RGB_R_GPIO_PIN, GPIO_PIN_SET);

    __HAL_RCC_GPIOA_CLK_ENABLE();
    //侧边配置按键管脚初始化
    GPIO_InitStruct.Pin = MODE_BOTTON_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    HAL_GPIO_Init(MODE_BOTTON_GPIO_PORT, &GPIO_InitStruct);
}

uint8_t HAL_UI_Mode_BUTTON_GetState(Button_TypeDef Button)
{
    return HAL_GPIO_ReadPin(MODE_BOTTON_GPIO_PORT, MODE_BOTTON_GPIO_PIN);
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
