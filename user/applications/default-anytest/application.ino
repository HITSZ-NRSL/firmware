/*
 * anytest 默认程序
 */
// 测试板程序
#if     1
#include "Adafruit_SSD1306.h"
#include "ajson.h"

#define OLED_RESET  D16
#define OLED_DC     D17
#define OLED_CS     D21

#define NONE_PIN  254

#define W323_TOTAL_TEST_PIN   34
#define FIG_TOTAL_TEST_PIN    40
#define W67_TOTAL_TEST_PIN    11
#define L6_TOTAL_TEST_PIN     19

//测试板与 被测板 引脚接法 前面是测试板引脚，后面是被测板引脚
/****W323
D1-GPIO21    A6-GPIO32
D4-GPIO22    A7-GPIO33
D5-GPIO19    A8-GPIO25
D6-GPIO23    A9-GPIO26
D7-GPIO18    A10-GPIO27
D8-GPIO5     A11-GPIO14
D9-GPIO17    A12-GPIO12
D11-GPIO16   A13-GPIO13
D12-GPIO4    A14-GPIO15
D13-GPIO0
D14-GPIO2
****/
const uint8_t W323PinMapping[W323_TOTAL_TEST_PIN] = {D13,0xff,D14,0xff,D12,D8,0xff,0xff,0xff,0xff,0xff,0xff,A12,A13,A11,A14,D11,D9,D7,D5,0xff,D1,D4,D6,0xff,A8,A9,A10,0xff,0xff,0xff,0xff,A6,A7};
const char *W323PinName[] = {"GPIO0","NONE","GPIO2","NONE","GPIO4","GPIO5","NONE","NONE","NONE","NONE","NONE","NONE","GPIO12","GPIO13","GPIO14","GPIO15","GPIO16","GPIO17","GPIO18","GPIO19","NONE","GPIO21","GPIO22","GPIO23","NONE","GPIO25","GPIO26","GPIO27","NONE","NONE","NONE","NONE","NONE","GPIO32","GPIO33"};

//测试板    Fig
//D4-D10接D0-D6
//A5-A11接A3-A9 一一对应
const uint8_t figPinMapping[FIG_TOTAL_TEST_PIN] = {D4,D5,D6,D7,D8,D9,D10,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,A5,A6,A7,A8,A9,A10,A11};
const char *figPinName[] = {"D0","D1","D2","D3","D4","D5","D6","NONE","NONE","NONE","NONE","NONE","NONE","NONE","NONE","NONE","NONE","NONE","NONE","NONE","NONE","NONE","NONE","NONE","NONE","NONE","NONE","NONE","NONE","NONE","NONE","NONE","NONE","A3","A4","A5","A6","A7","A8","A9"};

//w67 pin
//D4-GPIO5  A5-GPIO16
//D5-GPIO4  A6-GPIO14
//D6-GPIO0  A7-GPIO12
//D7-GPIO2  A8-GPIO13
//D8-GPIO15 *GPIO0 不读取
const uint8_t W67PinMapping[W67_TOTAL_TEST_PIN] = {0xff,0xff,D7,0xff,D5,D4,A7,A8,A6,D8,A5};
const char *W67PinName[] = {"NONE","NONE","GPIO2","NONE","GPIO4","GPIO5","GPIO12","GPIO13","GPIO14","GPIO15","GPIO16"};

//L6 PA2 PA3 is serial PA13 PA14 is SWDIO SWCLK
const uint8_t L6PinMapping[L6_TOTAL_TEST_PIN] = {A13,A14,A15,D15,D14,D13,D12,A2,A3,0xff,0xff,A6,A7,A8,A9,D11,D10,D9,D8};
const char *L6PinName[] = {"PB2","PB10","PB11","PB12","PB13","PB14","PB15","PA0","PA1","NONE","NONE","PA4","PA5","PA6","PA7","PA9","PA10","PA11","PA12"};

Adafruit_SSD1306 display(OLED_DC, OLED_RESET, OLED_CS);  // Hareware SPI

aJsonClass aJson;
uint8_t boardType;
uint8_t step;
bool testResult = true;
uint32_t currentTime;
uint8_t helloCnt;
uint8_t pinNo;
uint8_t errorType;
uint8_t errorPin;
int8_t snrVal = 0;
int8_t rssiVal = 0;
int8_t txRssiVal = 0;

enum ErrorType
{
    ERROR_PIN_HIGH = 0,
    ERROR_PIN_LOW,
    ERROR_ANALOG_READ,
    ERROR_SELF_TEST,
    ERROR_RF_CHECK,
    ERROR_SENSOR_DATA
};


enum BoardType
{
    ATOM = 1,
    NEUTRON,
    NUT,
    W67,
    FIG,
    W323,
    LORA,
    L6,
    UNKOWN_BOARD_TYPE = 0xfe,
    ERROR_COMMAND = 0xff
};

typedef enum TestItem
{
    TEST_DIGITAL_WRITE_HIGH = 0,
    TEST_DIGITAL_WRITE_LOW,
    TEST_ANALOG_READ,
    TEST_SELF_TEST,
    TEST_RF_CHECK,
    TEST_SENSOR_DATA
}testItem_t;

typedef enum TestStep
{
    STEP_GET_BOARD_TYPE = 0,
    STEP_CONFIRM_GET_BOARD_TYPE,
    STEP_DIGITAL_WRITE_HIGH,
    STEP_CONFIRM_DIGITAL_WRITE_HIGH_RESULT,
    STEP_DIGITAL_WRITE_LOW,
    STEP_CONFIRM_DIGITAL_WRITE_LOW_RESULT,

    STEP_SINGLE_DIGITAL_WRITE_HIGH,
    STEP_SINGLE_CONFIRM_DIGITAL_WRITE_HIGH_RESULT,
    STEP_SINGLE_DIGITAL_WRITE_LOW,
    STEP_SINGLE_CONFIRM_DIGITAL_WRITE_LOW_RESULT,

    STEP_ANALOG_READ,
    STEP_CONFIRM_ANALOG_READ_RESULT,
    STEP_SELF_TEST,
    STEP_CONFIRM_SELF_TEST_RESULT,
    STEP_RF_CHECK,
    STEP_CONFIRM_RF_CHECK_RESULT,
    STEP_SENSOR_DATA,
    STEP_CONFIRM_SENSOR_DATA_RESULT,
    STEP_TEST_END
}testStep_t;

String readString(void)
{
    return Serial1.readString();
}

int SerialAvailable(void)
{
    return Serial1.available();
}

void SerialPrint(const char *c)
{
    Serial1.println(c);
}

void queryBoard(void)
{
    aJsonObject* root = aJson.createObject();
    aJson.addStringToObject(root, "command", "hello");
    char *boardPtr = aJson.print(root);
    SerialPrint(boardPtr);
    free(boardPtr);
    aJson.deleteItem(root);
    currentTime = timerGetId();
}

uint8_t getBoardType(void)
{
    char *boardPtr = nullptr;
    if(timerIsEnd(currentTime,1000)) // 5s timeout
    {
        display.clearDisplay();
        display.setCursor(0,0);
        display.println("Board is not in the config mode");
        display.display();
        return ERROR_COMMAND;
    }

    while(SerialAvailable())
    {
            String tmp = readString();

            aJsonObject *root = NULL;
            root = aJson.parse((char *)tmp.c_str());
            if (root == NULL)
            {
                aJson.deleteItem(root);
                return ERROR_COMMAND;
            }

            aJsonObject* statusObject = aJson.getObjectItem(root, "status");
            if(statusObject == NULL)
            {
                aJson.deleteItem(root);
                return ERROR_COMMAND;
            }
            aJsonObject* versionObject = aJson.getObjectItem(root, "version");
            if(versionObject == NULL)
            {
                aJson.deleteItem(root);
                return ERROR_COMMAND;
            }
            aJsonObject* boardObject = aJson.getObjectItem(root, "board");
            if(boardObject == NULL)
            {
                aJson.deleteItem(root);
                return ERROR_COMMAND;
            }

            boardPtr = boardObject->valuestring;
            if(strcmp(boardPtr,"888004") == 0 || strcmp(boardPtr,"887004") == 0) {aJson.deleteItem(root);return ATOM;}         // atom
            else if(strcmp(boardPtr,"888002") == 0 || strcmp(boardPtr,"887002") == 0) {aJson.deleteItem(root);return NEUTRON;}//neutron
            else if(strcmp(boardPtr,"888003") == 0 || strcmp(boardPtr,"887003") == 0) {aJson.deleteItem(root);return NUT;}   //nut
            else if(strcmp(boardPtr,"888005") == 0 || strcmp(boardPtr,"887005") == 0) {aJson.deleteItem(root);return FIG;}   //fig
            else if(strcmp(boardPtr,"888006") == 0 || strcmp(boardPtr,"887006") == 0) {aJson.deleteItem(root);return LORA;} //lora
            else if(strcmp(boardPtr,"888101") == 0 || strcmp(boardPtr,"887101") == 0) {aJson.deleteItem(root);return W67;}   //w6/7
            else if(strcmp(boardPtr,"888102") == 0 || strcmp(boardPtr,"887102") == 0) {aJson.deleteItem(root);return W323;} //w32/33
            else if(strcmp(boardPtr,"888103") == 0 || strcmp(boardPtr,"887103") == 0) {aJson.deleteItem(root);return L6;}   //l6
            else
            {
                boardType = UNKOWN_BOARD_TYPE;
                aJson.deleteItem(root);
                return UNKOWN_BOARD_TYPE;
            }

            aJson.deleteItem(root);
    }
}

void TestInit(void)
{
    display.begin(SSD1306_SWITCHCAPVCC);
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    display.clearDisplay();
    display.println("check board type ...");
    display.display();
}

// void OLEDDisplay(uint8_t row, uint8_t column, const char *itemName, const char *result)
void OLEDDisplay(const char *itemName, const char *result)
{
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0,0);
    // display.setCursor(row,column);
    display.println(itemName);
    display.setTextSize(1);
    display.setCursor(0,32);
    // display.setCursor(row,column);
    display.println(result);
    display.display();
}

void JudgeBoardType(void)
{
    switch(boardType)
        {
        case UNKOWN_BOARD_TYPE:
            step = STEP_TEST_END;
            break;

        case ERROR_COMMAND:
            step = STEP_GET_BOARD_TYPE;
            break;

        case ATOM:
            display.clearDisplay();
            display.setCursor(0,0);
            display.println("Atom Test");
            display.display();
            delay(500);
            step = STEP_DIGITAL_WRITE_HIGH;
            break;

        case NEUTRON:
            display.clearDisplay();
            display.setCursor(0,0);
            display.println("Neutron Test");
            display.display();
            delay(500);
            step = STEP_DIGITAL_WRITE_HIGH;
            break;

        case NUT:
            display.clearDisplay();
            display.setCursor(0,0);
            display.println("Nut Test");
            display.display();
            delay(500);
            step = STEP_DIGITAL_WRITE_HIGH;
            break;

        case W67:
            display.clearDisplay();
            display.setCursor(0,0);
            display.println("W6/7 Test");
            display.display();
            delay(500);
            step = STEP_DIGITAL_WRITE_HIGH;
            break;

        case FIG:
            display.clearDisplay();
            display.setCursor(0,0);
            display.println("Fig Test");
            display.display();
            delay(500);
            step = STEP_DIGITAL_WRITE_HIGH;
            break;

        case W323:
            display.clearDisplay();
            display.setCursor(0,0);
            display.println("W32/33 Test");
            display.display();
            delay(500);
            step = STEP_DIGITAL_WRITE_HIGH;
            break;

        case LORA:
            display.clearDisplay();
            display.setCursor(0,0);
            display.println("Lora Test");
            display.display();
            delay(500);
            step = STEP_DIGITAL_WRITE_HIGH;
            break;

        case L6:
            display.clearDisplay();
            display.setCursor(0,0);
            display.println("L6 Test");
            display.display();
            delay(500);
            step = STEP_DIGITAL_WRITE_HIGH;
            break;

        default:
            break;
        }
}

bool JudgePinLevel(uint8_t pin,uint8_t val)
{
    if(val == 1)
    {
        if(digitalRead(pin) == 0)
        {
            errorPin = pin;
            errorType = ERROR_PIN_HIGH;
            return false;
        }
    }
    else
    {
        if(digitalRead(pin) == 1)
        {
            errorPin = pin;
            errorType = ERROR_PIN_LOW;
            return false;
        }
    }
    return true;
}

bool readAllPinLevel(uint8_t pinsCnt, const uint8_t *pinMap, uint8_t level)
{
    uint8_t pin;
    for(pin = 0;  pin < pinsCnt; pin++)
    {
        if(pinMap[pin] != 0xff)
        {
            if(digitalRead(pinMap[pin]) == level)
            {
                if(level == 1)
                    errorType = ERROR_PIN_LOW;
                else
                    errorType = ERROR_PIN_HIGH;
                errorPin = pinMap[pin];

                return false;
            }
        }
    }
    return true;
}

bool readSinglePinLevel(uint8_t singlePin, uint8_t pinsCnt, const uint8_t *pinMap,uint8_t level)
{
    uint8_t pin;
    if(digitalRead(pinMap[singlePin]) == level)
    {
        if(level == 1)
            errorType = ERROR_PIN_HIGH;
        else
            errorType = ERROR_PIN_LOW;
        errorPin = pinMap[singlePin];
        return false;
    }

    for(pin = pinsCnt-1; pin > singlePin; pin--)
    {
        if(pinMap[pin] != 0xff)
        {
            return JudgePinLevel(pinMap[pin],level);
        }
    }

    for(pin = 0; pin < singlePin;pin++)
    {
        if(pinMap[pin] != 0xff)
        {
            return JudgePinLevel(pinMap[pin],level);
        }
    }
    return true;
}



bool ReadBoardPinLevel(uint16_t gpio,uint8_t val)
{
    uint8_t pin;
    switch(boardType)
    {
        case ATOM:
            break;

        case NEUTRON:
            break;

        case NUT:
            break;

        case W67:
            for(pin = 0; pin < W67_TOTAL_TEST_PIN; pin++)
            {
                pinMode(W67PinMapping[pin],INPUT);
            }
            delay(1);
            if(gpio == 255)
            {
                return readAllPinLevel(W67_TOTAL_TEST_PIN,W67PinMapping,val);
            }
            else
            {
                return readSinglePinLevel(gpio,W67_TOTAL_TEST_PIN,W67PinMapping,val);
            }
            break;

        case FIG:
            for(pin = 0; pin < FIG_TOTAL_TEST_PIN; pin++)
            {
                pinMode(figPinMapping[pin],INPUT);
            }
            delay(1);
            if(gpio == 255)
            {
                return readAllPinLevel(FIG_TOTAL_TEST_PIN,figPinMapping,val);
            }
            else
            {
                return readSinglePinLevel(gpio,FIG_TOTAL_TEST_PIN,figPinMapping,val);
            }

            break;

        case W323:
            for(pin = 0; pin < W323_TOTAL_TEST_PIN; pin++)
            {
                pinMode(W323PinMapping[pin],INPUT);
            }
            delay(1);
            if(gpio == 255)
            {
                return readAllPinLevel(W323_TOTAL_TEST_PIN,W323PinMapping,val);
            }
            else
            {
                return readSinglePinLevel(gpio,W323_TOTAL_TEST_PIN,W323PinMapping,val);
            }
            break;

        case LORA:
            break;

        case L6:
            for(pin = 0;  pin < L6_TOTAL_TEST_PIN; pin++)
            {
                pinMode(L6PinMapping[pin],INPUT_PULLUP);
            }
            delay(1);
            if(gpio == 255)
            {
                return readAllPinLevel(L6_TOTAL_TEST_PIN,L6PinMapping,val);
            }
            else
            {
                return readSinglePinLevel(gpio,L6_TOTAL_TEST_PIN,L6PinMapping,val);
            }
            break;

        default:
            break;
    }
    return true;
}

bool ReceiveTestResult(testItem_t testItem, uint16_t gpio)
{
        while(SerialAvailable())
        {
            String tmp = readString();

            aJsonObject *root = NULL;
            root = aJson.parse((char *)tmp.c_str());
            if (root == NULL)
            {
                aJson.deleteItem(root);
                return false;
            }

            aJsonObject* statusObject = aJson.getObjectItem(root, "status");
            if(statusObject == NULL)
            {
                aJson.deleteItem(root);
                return false;
            }

            switch(testItem)
            {
                case TEST_DIGITAL_WRITE_HIGH:
                    if((statusObject->valueint == 200) && ReadBoardPinLevel(gpio,0))
                    {
                        OLEDDisplay("digitalWrite High:","OK");
                    }
                    else
                    {
                        OLEDDisplay("digitalWrite High:","NG");
                        testResult = false;
                    }
                    break;

                case TEST_DIGITAL_WRITE_LOW:
                    if((statusObject->valueint == 200) && ReadBoardPinLevel(gpio,1))
                    {
                        OLEDDisplay("digitalWrite Low:","OK");
                    }
                    else
                    {
                        OLEDDisplay("digitalWrite Low:","NG");
                        testResult = false;
                    }
                    break;

                case TEST_ANALOG_READ:
                    if(statusObject->valueint == 200)
                    {
                        aJsonObject* valueObject = aJson.getObjectItem(root, "value");
                        int16_t adVal = valueObject->valueint;
                        if((adVal >= 0) && (adVal <= 4095))
                        {
                            OLEDDisplay("analogRead:","OK");
                        }
                        else
                        {
                            OLEDDisplay("analogRead:","NG");
                            errorType = ERROR_ANALOG_READ;
                            testResult = false;
                        }
                    }
                    else
                    {
                        OLEDDisplay("analogRead:","NG");
                        errorType = ERROR_ANALOG_READ;
                        testResult = false;
                    }

                    break;

                case TEST_SELF_TEST:
                    switch(boardType)
                    {
                        case ATOM:
                            break;

                        case NEUTRON:
                            break;

                        case W67:
                        case W323:
                            if(statusObject->valueint == 200)
                             {
                                 OLEDDisplay("selfTest:","OK");
                             }
                            else
                            {
                                OLEDDisplay("selfTest:","NG");
                                errorType = ERROR_SELF_TEST;
                                testResult = false;
                            }
                            break;

                        case LORA:
                        case L6:
                            if(statusObject->valueint == 200)
                                {
                                    OLEDDisplay("selfTest:","OK");
                                }
                            else
                                {
                                    OLEDDisplay("selfTest:","NG");
                                    errorType = ERROR_SELF_TEST;
                                    testResult = false;
                                }

                            break;

                        default:
                            break;
                    }
                    break;

                case TEST_RF_CHECK:
                if((boardType == ATOM) || (boardType == NEUTRON)||(boardType == NUT)||(boardType == W67)||(boardType == FIG)||(boardType == W323))
                {
                    if(statusObject->valueint == 200)
                    {
                        aJsonObject* listnumtObject = aJson.getObjectItem(root, "listnum");
                        if(listnumtObject == NULL)
                        {
                           aJson.deleteItem(root);
                           return false;
                        }

                        aJsonObject* ssidlistArrayObject = aJson.getObjectItem(root, "ssidlist");
                        if(ssidlistArrayObject == NULL)
                        {
                            aJson.deleteItem(root);
                            return false;
                        }

                        uint8_t arrayNumber = aJson.getArraySize(ssidlistArrayObject);

                        WiFiAccessPoint ap[20];

                        for(uint8_t num = 0; num < arrayNumber; num++)
                        {
                            aJsonObject* ssidObject = aJson.getArrayItem(ssidlistArrayObject,num);
                            if(ssidObject == NULL)
                            {
                                aJson.deleteItem(root);
                                return false;
                            }

                            aJsonObject* ssidNameObject = aJson.getObjectItem(ssidObject,"ssid");
                            if(ssidNameObject == NULL)
                            {
                                aJson.deleteItem(root);
                                return false;
                            }

                            strcpy(ap[num].ssid,ssidNameObject->valuestring);

                            aJsonObject* entypeObject = aJson.getObjectItem(ssidObject,"entype");
                            if(entypeObject == NULL)
                            {
                                aJson.deleteItem(root);
                                return false;
                            }
                            // ap[num].security = (WLanSecurityCipher)entypeObject->valueint;

                            aJsonObject* signalObject = aJson.getObjectItem(ssidObject,"signal");
                            if(signalObject == NULL)
                            {
                                aJson.deleteItem(root);
                                return false;
                            }
                            ap[num].rssi = signalObject->valueint;
                        }

                        bool assignWifi = false;
                        for(uint8_t i = 0; i < arrayNumber; i++)
                        {
                            if(strcmp(ap[i].ssid,"IntoRobot-NETGEAR") == 0)
                            {
                                assignWifi = true;
                                if(ap[i].rssi >= (-65))
                                {
                                    OLEDDisplay("wifiCheck:","OK");
                                    break;
                                }
                                else
                                {
                                    OLEDDisplay("wifiCheck:","NG");
                                    errorType = ERROR_RF_CHECK;
                                    testResult = false;
                                    break;
                                }
                            }
                        }

                        if(!assignWifi)
                        {
                            OLEDDisplay("Not Open WiFi Network!!!","");
                            while(1);
                        }
                    }
                    else
                    {
                        OLEDDisplay("wifiCheck:","NG");
                        errorType = ERROR_RF_CHECK;
                        testResult = false;
                    }
                }
                else if((boardType == LORA) || (boardType == L6))
                {
                    uint8_t statusVal = statusObject->valueint;
                    aJsonObject* snrObject = aJson.getObjectItem(root, "snr");
                    snrVal = snrObject->valueint;
                    aJsonObject* rssiObject = aJson.getObjectItem(root, "rssi");
                    rssiVal = rssiObject->valueint;
                    aJsonObject* txrssiObject = aJson.getObjectItem(root, "txrssi");
                    txRssiVal = txrssiObject->valueint;


                    if(statusVal == 200)
                    {
                        if((rssiVal > -30) && (txRssiVal > -30))
                        {
                            OLEDDisplay("RFCheck:","OK");
                        }
                        else
                        {
                            OLEDDisplay("RFCheck:","NG");
                            errorType = ERROR_RF_CHECK;
                            testResult = false;
                        }
                    }
                    else
                    {
                        OLEDDisplay("RFCheck:","NG");
                        errorType = ERROR_RF_CHECK;
                        testResult = false;
                    }
                }
                break;

                case TEST_SENSOR_DATA:
                    break;

                default:
                    break;
            }

            aJson.deleteItem(root);
            return true;
        }
}

void SendTestCommand(testItem_t itemCommand, uint16_t gpio)
{
    aJsonObject* root = aJson.createObject();
    aJson.addStringToObject(root, "command", "test");
    aJsonObject* valueObject = aJson.createObject();
    aJson.addItemToObject(root, "value", valueObject);

    char* strPtr = nullptr;

    switch(itemCommand)
    {
        case TEST_DIGITAL_WRITE_HIGH:
            aJson.addStringToObject(valueObject, "item", "digitalWrite");
            aJson.addNumberToObject(valueObject, "pin", gpio);
            aJson.addStringToObject(valueObject, "val", "HIGH");
            strPtr = aJson.print(root);
            SerialPrint(strPtr);
            aJson.deleteItem(root);

            break;

        case TEST_DIGITAL_WRITE_LOW:
            aJson.addStringToObject(valueObject, "item", "digitalWrite");
            aJson.addNumberToObject(valueObject, "pin", gpio);
            aJson.addStringToObject(valueObject, "val", "LOW");
            strPtr = aJson.print(root);
            SerialPrint(strPtr);
            aJson.deleteItem(root);

            break;


        case TEST_ANALOG_READ:
            aJson.addStringToObject(valueObject, "item", "analogRead");
            aJson.addNumberToObject(valueObject, "pin", gpio);
            strPtr = aJson.print(root);
            SerialPrint(strPtr);
            aJson.deleteItem(root);

            break;

        case TEST_SELF_TEST:
            aJson.addStringToObject(valueObject, "item", "selfTest");
            strPtr = aJson.print(root);
            SerialPrint(strPtr);
            aJson.deleteItem(root);

            break;

        case TEST_RF_CHECK:
            aJson.addStringToObject(valueObject, "item", "rfCheck");
            strPtr = aJson.print(root);
            SerialPrint(strPtr);
            aJson.deleteItem(root);
            break;

        case TEST_SENSOR_DATA:
            aJson.addStringToObject(valueObject, "item", "sensorData");
            strPtr = aJson.print(root);
            SerialPrint(strPtr);
            aJson.deleteItem(root);

            break;
        default:
            break;
    }

    free(strPtr);
    aJson.deleteItem(root);
}

//testLevel true:test high false:test low
//testEnd   true:test end  flase: no
void FigPinLoopTestHandle(testItem_t itemCommand, bool testHighLevel,bool testEnd)
{
    if(pinNo < FIG_TOTAL_TEST_PIN)
    {
        if(ReceiveTestResult(itemCommand,pinNo))
        {
            // delay(10);
            pinNo++;
            if(pinNo == FIG_TOTAL_TEST_PIN)
            {
                pinNo = 0;
                if(testEnd)
                {
                    step = STEP_TEST_END;
                }
                else
                {
                    if(testHighLevel)
                        step = STEP_SINGLE_DIGITAL_WRITE_LOW;
                    else
                        step = STEP_ANALOG_READ;
                }
            }
            else
            {
                if(pinNo == 7) pinNo = 33;
                if(testHighLevel)
                {
                    step = STEP_SINGLE_DIGITAL_WRITE_HIGH;
                }
                else
                {
                    step = STEP_SINGLE_DIGITAL_WRITE_LOW;
                }
            }

        }
    }
}

void W323PinLoopTestHandle(testItem_t itemCommand, bool testHighLevel,bool testEnd)
{
    if(pinNo < W323_TOTAL_TEST_PIN)
    {
        if(ReceiveTestResult(itemCommand,pinNo))
        {
            // delay(10);
            pinNo++;
            if(pinNo == W323_TOTAL_TEST_PIN)
            {
                pinNo = 0;
                if(testEnd)
                {
                    step = STEP_TEST_END;
                }
                else
                {
                    if(testHighLevel)
                        step = STEP_SINGLE_DIGITAL_WRITE_LOW;
                    else
                        step = STEP_ANALOG_READ;
                }
            }
            else
            {
                if(pinNo == 1)
                {
                    pinNo = 2;
                }
                else if(pinNo == 3)
                {
                    pinNo = 4;
                }
                else if(pinNo == 6)
                {
                    pinNo = 12;
                }
                else if(pinNo == 20)
                {
                    pinNo = 21;
                }
                else if(pinNo == 24)
                {
                    pinNo = 25;
                }
                else if(pinNo == 28)
                {
                    pinNo = 32;
                }

                if(testHighLevel)
                {
                    step = STEP_SINGLE_DIGITAL_WRITE_HIGH;
                }
                else
                {
                    step = STEP_SINGLE_DIGITAL_WRITE_LOW;
                }
            }

        }
    }
}

void L6PinLoopTestHandle(testItem_t itemCommand, bool testHighLevel,bool testEnd)
{
    if(pinNo < L6_TOTAL_TEST_PIN)
    {
        if(ReceiveTestResult(itemCommand,pinNo))
        {
            // delay(10);
            pinNo++;
            if(pinNo == L6_TOTAL_TEST_PIN)
            {
                pinNo = 0;
                if(testEnd)
                {
                    step = STEP_TEST_END;
                }
                else
                {
                    if(testHighLevel)
                        step = STEP_SINGLE_DIGITAL_WRITE_LOW;
                    else
                        step = STEP_ANALOG_READ;
                }
            }
            else
            {
                if(pinNo == 9) pinNo = 11;
                if(testHighLevel)
                {
                    step = STEP_SINGLE_DIGITAL_WRITE_HIGH;
                }
                else
                {
                    step = STEP_SINGLE_DIGITAL_WRITE_LOW;
                }
            }
        }
    }
}

void W67PinLoopTestHandle(testItem_t itemCommand, bool testHighLevel,bool testEnd)
{
    if(pinNo < W67_TOTAL_TEST_PIN)
    {
        if(ReceiveTestResult(itemCommand,pinNo))
        {
            // delay(10);
            pinNo++;
            if(pinNo == W67_TOTAL_TEST_PIN)
            {
                pinNo = 0;
                if(testEnd)
                {
                    step = STEP_TEST_END;
                }
                else
                {
                    if(testHighLevel)
                    {
                        pinNo = 2;
                        step = STEP_SINGLE_DIGITAL_WRITE_LOW;
                    }
                    else
                    {
                        step = STEP_ANALOG_READ;
                    }
                }
            }
            else
            {
                if(pinNo == 3) pinNo = 4;
                if(testHighLevel)
                {
                    step = STEP_SINGLE_DIGITAL_WRITE_HIGH;
                }
                else
                {
                    step = STEP_SINGLE_DIGITAL_WRITE_LOW;
                }
            }
        }
    }
}


String  strError = "GPIO Error:";

void SearchErrorPin(uint8_t boardPins, char *pinMap, const char *pinName[])
{
    uint8_t index;
    for(index = 0; index < boardPins; index++)
    {
        if(errorPin == pinMap[index])
        {
            strError = strError + pinName[index];
            return;
        }
    }
}

void setup()
{
    Serial1.begin(115200);
    Serial1.setTimeout(50);
    delay(100);
    TestInit();
    step = STEP_GET_BOARD_TYPE;
}

uint32_t ctimeout;

void loop()
{
    switch(step)
    {
        case STEP_GET_BOARD_TYPE:
            queryBoard();
            step = STEP_CONFIRM_GET_BOARD_TYPE;
            break;

        case STEP_CONFIRM_GET_BOARD_TYPE:
            boardType = getBoardType();
            JudgeBoardType();
            break;

        case STEP_DIGITAL_WRITE_HIGH:
            OLEDDisplay("test 1:","digitalWrite High");
            SendTestCommand(TEST_DIGITAL_WRITE_HIGH,255);
            step = STEP_CONFIRM_DIGITAL_WRITE_HIGH_RESULT;
            break;

        case STEP_CONFIRM_DIGITAL_WRITE_HIGH_RESULT:
            if(ReceiveTestResult(TEST_DIGITAL_WRITE_HIGH,255))
            {
                delay(100);
                step = STEP_DIGITAL_WRITE_LOW;
            }
            break;
        case STEP_DIGITAL_WRITE_LOW:
            OLEDDisplay("test 2:","digitalWrite Low");
            SendTestCommand(TEST_DIGITAL_WRITE_LOW,255);
            step = STEP_CONFIRM_DIGITAL_WRITE_LOW_RESULT;
            break;

        case STEP_CONFIRM_DIGITAL_WRITE_LOW_RESULT:
            if(ReceiveTestResult(TEST_DIGITAL_WRITE_LOW,255))
            {
                delay(100);
                step = STEP_SINGLE_DIGITAL_WRITE_HIGH;
                if(boardType == W67)
                    pinNo = 2;
                else
                    pinNo = 0;
            }
            break;

        case STEP_SINGLE_DIGITAL_WRITE_HIGH:
            {
             OLEDDisplay("test 3:","single digital high");
             SendTestCommand(TEST_DIGITAL_WRITE_LOW,255);
             ctimeout = timerGetId();
             while(1)
             {
                 if(ReceiveTestResult(TEST_DIGITAL_WRITE_LOW,255))
                 {
                     break;
                 }
                 if(timerIsEnd(ctimeout,1000))
                 {
                     break;
                 }
             }
             SendTestCommand(TEST_DIGITAL_WRITE_HIGH,pinNo);
             step = STEP_SINGLE_CONFIRM_DIGITAL_WRITE_HIGH_RESULT;
             }
            break;

        case STEP_SINGLE_CONFIRM_DIGITAL_WRITE_HIGH_RESULT:
            if(boardType == W67)
            {
                W67PinLoopTestHandle(TEST_DIGITAL_WRITE_HIGH,true,false);
            }
            else if(boardType == FIG)
            {
                FigPinLoopTestHandle(TEST_DIGITAL_WRITE_HIGH,true,false);
            }
            else if(boardType == W323)
            {
                W323PinLoopTestHandle(TEST_DIGITAL_WRITE_HIGH,true,false);
            }
            else if(boardType == L6)
            {
                L6PinLoopTestHandle(TEST_DIGITAL_WRITE_HIGH,true,false);
            }
            break;

        case STEP_SINGLE_DIGITAL_WRITE_LOW:
            {
            OLEDDisplay("test 4:","single digital low");
            SendTestCommand(TEST_DIGITAL_WRITE_HIGH,255);
            ctimeout = timerGetId();
            while(1)
            {
                if(ReceiveTestResult(TEST_DIGITAL_WRITE_HIGH,255))
                {
                    break;
                }
                if(timerIsEnd(ctimeout,1000)) 
                {
                    break;
                }
            }
            SendTestCommand(TEST_DIGITAL_WRITE_LOW,pinNo);
            step = STEP_SINGLE_CONFIRM_DIGITAL_WRITE_LOW_RESULT;
            }
            break;

        case STEP_SINGLE_CONFIRM_DIGITAL_WRITE_LOW_RESULT:
            if(boardType == W67)
            {
                W67PinLoopTestHandle(TEST_DIGITAL_WRITE_LOW,false,false);
            }
            else if(boardType == FIG)
            {
                FigPinLoopTestHandle(TEST_DIGITAL_WRITE_LOW,false,true);
            }
            else if(boardType == W323)
            {
                W323PinLoopTestHandle(TEST_DIGITAL_WRITE_LOW,false,true);
            }
            else if(boardType == L6)
            {
                L6PinLoopTestHandle(TEST_DIGITAL_WRITE_LOW,false,false);
            }
            break;

        case STEP_ANALOG_READ:
            OLEDDisplay("test 5:","analogRead");
            if(boardType == L6 || boardType == LORA)
            {
                SendTestCommand(TEST_ANALOG_READ,14);
            }
            else
            {
                SendTestCommand(TEST_ANALOG_READ,30);
            }
            step = STEP_CONFIRM_ANALOG_READ_RESULT;
            break;

        case STEP_CONFIRM_ANALOG_READ_RESULT:
            {
                uint8_t analogPin = 30;
                if(boardType == L6 || boardType == LORA)
                {
                    analogPin = 14;
                }

                if(ReceiveTestResult(TEST_ANALOG_READ,analogPin))
                {
                    delay(100);
                    step = STEP_SELF_TEST;
                }
            }
            break;

        case STEP_SELF_TEST:
            OLEDDisplay("test 6:","selfTest");
            SendTestCommand(TEST_SELF_TEST,NONE_PIN);
            step = STEP_CONFIRM_SELF_TEST_RESULT;

            break;

        case STEP_CONFIRM_SELF_TEST_RESULT:
            if(ReceiveTestResult(TEST_SELF_TEST,NONE_PIN))
            {
                delay(100);
                step = STEP_RF_CHECK;
            }

            break;

        case STEP_RF_CHECK:
            OLEDDisplay("test 7:","RFCheck");
            SendTestCommand(TEST_RF_CHECK,NONE_PIN);
            step = STEP_CONFIRM_RF_CHECK_RESULT;
            break;

        case STEP_CONFIRM_RF_CHECK_RESULT:
            if(ReceiveTestResult(TEST_RF_CHECK,NONE_PIN))
            {
                if(boardType == NEUTRON || boardType == NUT || boardType == FIG)
                {
                    step = STEP_SENSOR_DATA;
                }
                else
                {
                    step = STEP_TEST_END;
                }
            }

            break;

        case STEP_SENSOR_DATA:
            SendTestCommand(TEST_SENSOR_DATA,NONE_PIN);
            break;

        case STEP_CONFIRM_SENSOR_DATA_RESULT:
            if(ReceiveTestResult(TEST_SENSOR_DATA,NONE_PIN))
            {
                step = STEP_TEST_END;
            }
            break;

        case STEP_TEST_END:

            delay(100);
            OLEDDisplay("Test End!!","");
            delay(500);
            if(boardType == UNKOWN_BOARD_TYPE)
            {
                OLEDDisplay("unkown board type","");
            }
            else
            {
                if(testResult)
                {
                    if(boardType == L6 || boardType == LORA)
                    {
                        String p;
                        p = p+"snr:";
                        char tmp[16];
                        sprintf(tmp,"%d",snrVal);
                        p = p+tmp;
                        p = p+ "\r\n";
                        p = p + "rssi:";
                        sprintf(tmp,"%d",rssiVal);
                        p = p+tmp;
                        p = p+ "\r\n";
                        p = p + "tx rssi:";
                        sprintf(tmp,"%d",txRssiVal);
                        p = p+tmp;
                        OLEDDisplay("PASS",p);
                    }
                    else
                    {
                        OLEDDisplay("PASS","");
                    }

                }
                else
                {
                    switch(errorType)
                    {
                        case ERROR_PIN_HIGH:
                        case ERROR_PIN_LOW:
                            {
                                if(boardType == W67)
                                {
                                    SearchErrorPin(W67_TOTAL_TEST_PIN,W67PinMapping,W67PinName);
                                }
                                else if(boardType == FIG)
                                {
                                    SearchErrorPin(FIG_TOTAL_TEST_PIN,figPinMapping,figPinName);
                                }
                                else if(boardType == W323)
                                {
                                    SearchErrorPin(W323_TOTAL_TEST_PIN,W323PinMapping,W323PinName);
                                }
                                else if(boardType == L6)
                                {
                                    SearchErrorPin(L6_TOTAL_TEST_PIN,L6PinMapping,L6PinName);
                                }

                                OLEDDisplay("FAIL", strError);
                            }
                            break;
                        case ERROR_ANALOG_READ:
                            OLEDDisplay("FAIL","analogRead Fail");
                            break;
                        case ERROR_SELF_TEST:
                            if(boardType == L6 || boardType == LORA)
                            {
                                OLEDDisplay("FAIL","selfTest Fail:32.768KHz Fial");
                            }
                            else
                            {
                                OLEDDisplay("FAIL","selfTest Fail");
                            }
                            break;
                        case ERROR_RF_CHECK:
                            if(boardType == L6 || boardType == LORA)
                            {
                                String p = "RFCheck Fail";
                                p = p+"\r\n";
                                p = p+"snr:";
                                char tmp[16];
                                sprintf(tmp,"%d",snrVal);
                                p = p+tmp;
                                p = p+ "\r\n";
                                p = p + "rssi:";
                                sprintf(tmp,"%d",rssiVal);
                                p = p+tmp;
                                p = p+ "\r\n";
                                p = p + "tx rssi:";
                                sprintf(tmp,"%d",txRssiVal);
                                p = p+tmp;
                                OLEDDisplay("FAIL",p);
                            }
                            else
                            {
                                OLEDDisplay("FAIL","RFCheck Fail");
                            }
                            break;
                        case ERROR_SENSOR_DATA:
                            OLEDDisplay("FAIL","sensor Fail");
                            break;
                    }

                }
            }
            while(1);
            break;

        default:
            break;
    }
}
#endif

#if  0
//L7 test code
#include "Adafruit_SSD1306.h"
#include "lora.h"

Serial1DebugOutput debugOutput(115200, ALL_LEVEL);

#define OLED_RESET  D16
#define OLED_DC     D17
#define OLED_CS     D21

Adafruit_SSD1306 display(OLED_DC, OLED_RESET, OLED_CS);  // Hareware SPI

int8_t loraData[64];
int8_t rssiVal;
int8_t snrVal;
int8_t txRssiVal;
uint32_t currTime;

void OLEDDisplay(const char *itemName, const char *result)
{
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0,0);
    display.println(itemName);
    display.setTextSize(1);
    display.setCursor(0,32);
    display.println(result);
    display.display();
}

void setup()
{
    Serial1.begin(115200);
    delay(100);

    display.begin(SSD1306_SWITCHCAPVCC);
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    display.clearDisplay();
    display.println("test L7");
    display.display();
    delay(1000);

    TestSX178Init();
    if(SX1278GetVersion() != 18)
    {
        OLEDDisplay("FAIL","SPI FAIL");
        while(1);
    }
    currTime = timerGetId();
}


void loop()
{
    switch(ProcessSX1278(rssiVal,snrVal,loraData))
    {
        case 0:
                OLEDDisplay("sx1278 send data...","");
                if(timerIsEnd(currTime,2000)) // 5s timeout
                {
                    OLEDDisplay("FAIL","sx1278 send timeout");
                }
            break;

        case 1:
            OLEDDisplay("sx1278 receive data...","");
            break;

        case 2:
            if(loraData[0] == 22)
            {
                txRssiVal = loraData[1];
                Serial1.println("data is ok");
                String p = "snr:";
                char tmp[16];
                sprintf(tmp,"%d",snrVal);
                p = p+tmp;
                p = p+ "\r\n";
                p = p + "rssi:";
                sprintf(tmp,"%d",rssiVal);
                p = p+tmp;
                p = p+ "\r\n";
                p = p + "tx rssi:";
                sprintf(tmp,"%d",txRssiVal);
                p = p+tmp;

                if(rssiVal > -40)
                    OLEDDisplay("PASS",p);
                else
                {
                    p = p + "\r\n";
                    p = p + "error: rssi < -40";
                    OLEDDisplay("FAIL",p);
                }

                while(1);
            }
            else
            {
                OLEDDisplay("FAIL","receive data is error");
                while(1);
            }

            break;

        default:
            OLEDDisplay("FAIL                 sx1278 receive timeout","please again or station is not open");
            // OLEDDisplay("sx1278 receive timeout","please again or station is not open");
            while(1);
            break;
    }
}
#endif

