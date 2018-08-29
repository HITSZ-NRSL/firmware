/*
 * 闪灯程序
 */

#define LEDPIN    LED_BUILTIN    //例子灯

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

