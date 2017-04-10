/*
 * 闪灯程序
 */

//SerialUSBDebugOutput debugOutput(115200, ALL_LEVEL);
SerialDebugOutput debugOutput(115200, ALL_LEVEL);

#define LEDPIN    LED_USER    //例子灯

void setup()
{
    pinMode(LEDPIN, OUTPUT);
}

void loop()
{
    digitalWrite(LEDPIN, LOW);
    delay(1000);
    digitalWrite(LEDPIN, HIGH);
    delay(1000);
}

