
#include "hw_config.h"
#include "ui_hal.h"
#include "boot_debug.h"


#define RGB_R_GPIO_PIN       GPIO_PIN_13
#define RGB_R_GPIO_PORT      GPIOC
#define RGB_G_GPIO_PIN       GPIO_PIN_7
#define RGB_G_GPIO_PORT      GPIOB
#define RGB_B_GPIO_PIN       GPIO_PIN_0
#define RGB_B_GPIO_PORT      GPIOA

#define MODE_BOTTON_GPIO_PIN       GPIO_PIN_2
#define MODE_BOTTON_GPIO_PORT      GPIOB

#define USER_LED_GPIO_PIN       GPIO_PIN_14
#define USER_LED_GPIO_PORT      GPIOA


volatile uint32_t BUTTON_last_state = 0;
volatile uint32_t TimingBUTTON=0;
volatile uint32_t TimingLED;
rgb_info_t rgb_info;

void Set_RGB_Color(uint32_t color) {
    uint8_t red,green,blue;

    red = color>>16 & 0xFF;
    green = color>>8 & 0xFF;
    blue = color & 0xFF;
    if(red) {
        HAL_GPIO_WritePin(RGB_R_GPIO_PORT, RGB_R_GPIO_PIN, GPIO_PIN_RESET);
    } else {
        HAL_GPIO_WritePin(RGB_R_GPIO_PORT, RGB_R_GPIO_PIN, GPIO_PIN_SET);
    }

    if(green) {
        HAL_GPIO_WritePin(RGB_G_GPIO_PORT, RGB_G_GPIO_PIN, GPIO_PIN_RESET);
    } else {
        HAL_GPIO_WritePin(RGB_G_GPIO_PORT, RGB_G_GPIO_PIN, GPIO_PIN_SET);
    }

    if(blue) {
        HAL_GPIO_WritePin(RGB_B_GPIO_PORT, RGB_B_GPIO_PIN, GPIO_PIN_RESET);
    } else {
        HAL_GPIO_WritePin(RGB_B_GPIO_PORT, RGB_B_GPIO_PIN, GPIO_PIN_SET);
    }
}

void RGB_Color_Toggle(void) {
    if(rgb_info.rgb_last_color) {
        Set_RGB_Color(0);
        rgb_info.rgb_last_color = 0;
    } else {
        Set_RGB_Color(rgb_info.rgb_color);
        rgb_info.rgb_last_color = rgb_info.rgb_color;
    }
}

void HAL_UI_Initial(void)
{
    //三色灯管脚初始化
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef  GPIO_InitStruct;
    GPIO_InitStruct.Pin = RGB_R_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    HAL_GPIO_Init(RGB_R_GPIO_PORT, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = RGB_G_GPIO_PIN;
    HAL_GPIO_Init(RGB_G_GPIO_PORT, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = RGB_B_GPIO_PIN;
    HAL_GPIO_Init(RGB_B_GPIO_PORT, &GPIO_InitStruct);

    HAL_GPIO_WritePin(RGB_R_GPIO_PORT, RGB_R_GPIO_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(RGB_G_GPIO_PORT, RGB_G_GPIO_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(RGB_B_GPIO_PORT, RGB_B_GPIO_PIN, GPIO_PIN_SET);

    //侧边配置按键管脚初始化
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitStruct.Pin = MODE_BOTTON_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    HAL_GPIO_Init(MODE_BOTTON_GPIO_PORT, &GPIO_InitStruct);

    //用户例子灯
    #if 0
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitStruct.Pin = USER_LED_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    HAL_GPIO_Init(USER_LED_GPIO_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(USER_LED_GPIO_PORT, USER_LED_GPIO_PIN, GPIO_PIN_SET);
    #endif
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
    if(RGB_MODE_BLINK == rgb_info.rgb_mode) {
        if (TimingLED != 0x00) {
            TimingLED--;
        } else {
            RGB_Color_Toggle();
            TimingLED = rgb_info.rgb_period;
        }
    }
    //侧边按键处理
    if(!HAL_UI_Mode_BUTTON_GetState(BUTTON1)) {
        if(!BUTTON_last_state) {
            TimingBUTTON = 0;
            BUTTON_last_state = 1;
        } else {
            TimingBUTTON++;
        }
    } else {
        if(BUTTON_last_state) {
            TimingBUTTON = 0;
            BUTTON_last_state = 0;
        }
    }
}

