/**
 ******************************************************************************
 Copyright (c) 2013-2113 IntoRobot Team.  All right reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation, either
 version 3 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

#include "lora_sx1276.h"
#include "gBase64.h"

// In order to make the CAD behaviour dynamic we set a variable
// when the CAD functions are defined.
#define NUM_HOPS 3

// Frequencies
// Set center frequency. If in doubt, choose the first one, comment all others
// Each "real" gateway should support the first 3 frequencies according to LoRa spec.
int freqs [] = {
    434000000, // Channel 0, 434.0 MHz primary
    434000000, // Channel 1, 434.2 MHz mandatory
    434000000, // Channel 2, 434.4 MHz mandatory
    434000000, // Channel 3, 868.1 MHz
    434000000,
    434000000,
    434000000,
    434000000,
    434000000,
    434000000,
    434000000,
    434000000,
    434000000          // Channel, for responses gateway (10%)
                       // TTN defines an additional channel at 869.525Mhz using SF9 for class B. Not used
};

uint32_t  freq = freqs[0];
uint8_t  ifreq = 0;        // Channel Index
bool _cad= (bool)_CAD;     // Set to true for Channel Activity Detection, only when dio1 connected
bool _hop=false;           // experimental; frequency hopping. Only use when dio2 connected
bool inHop=false;

// Our code should correct the server timing
long txDelay= 0x00;        // delay time on top of server TMST

// Interrupt variables
volatile uint8_t flags;
volatile uint8_t mask;

state_t _state;
struct pins pins;

bool sx1272 = true;         // Actually we use sx1276/RFM95
int debug=4;                // Debug level! 0 is no msgs, 1 normal, 2 extensive

unsigned long hopTime=0;
extern uint32_t stattime;   // last time we sent a stat message to server
extern uint32_t pulltime;   // last time we sent a pull_data request to server
extern uint32_t lastTmst;

// Set spreading factor (SF7 - SF12)
sf_t sf   = _SPREADING ;
sf_t sfi  = _SPREADING ;    // Initial value of SF

extern uint8_t MAC_array[6];

#define TX_BUFF_SIZE  2048              // Upstream buffer to send to MQTT
#define RX_BUFF_SIZE  1024              // Downstream received from MQTT
#define STATUS_SIZE   512               // Should(!) be enough based on the static text .. was 1024

extern uint32_t cp_nb_rx_rcv;
extern uint32_t cp_nb_rx_ok;
extern uint32_t cp_nb_rx_bad;
extern uint32_t cp_nb_rx_nocrc;
extern uint32_t cp_up_pkt_fwd;

extern uint8_t buff_up[TX_BUFF_SIZE];      // buffer to compose the upstream packet to backend server
extern uint8_t buff_down[RX_BUFF_SIZE];    // Buffer for downstream
extern uint16_t lastToken;

char b64[256];

void ftoa(float f, char *val, int p);
void sendUdp(uint8_t * msg, int length);


static void selectreceiver()
{
    digitalWrite(pins.ss, LOW);
}

static void unselectreceiver()
{
    digitalWrite(pins.ss, HIGH);
}

static uint8_t readRegister(uint8_t addr)
{
    selectreceiver();      // This is more or less standard
    //SPI.beginTransaction(SPISettings(SPISPEED, MSBFIRST, SPI_MODE0));
    SPI.beginTransaction();
    SPI.transfer(addr & 0x7F);
    uint8_t res = SPI.transfer(0x00);
    SPI.endTransaction();
    unselectreceiver();
    //Serial.printf("read addr %02x, value = %02x\r\n", addr, res);
    return res;
}

static void writeRegister(uint8_t addr, uint8_t value)
{
    unsigned char spibuf[2];

    spibuf[0] = addr | 0x80;
    spibuf[1] = value;
    selectreceiver();
    //SPI.beginTransaction(SPISettings(SPISPEED, MSBFIRST, SPI_MODE0));
    SPI.beginTransaction();
    SPI.transfer(spibuf[0]);
    SPI.transfer(spibuf[1]);
    SPI.endTransaction();
    unselectreceiver();
    //Serial.printf("write addr %02x, value = %02x\r\n", addr, value);
}

static void printState(uint8_t i)
{
    uint8_t intr = flags & ( ~ mask );       // Only react on non masked interrupts
    uint8_t rssi = readRegister(REG_RSSI);   // Read the RSSI

    if (i>= debug) {
        Serial.printf("state=%d, sf=%d, rssi=%d, flags=%02x, mask=%02x, intr=%02x\r\n",\
                _state, sf, rssi, flags, intr);
    }
}

// ----------------------------------------------------------------------------
//  setRate is setting rate and spreading factor and CRC etc. for transmission
//  Modem Config 1 (MC1) == 0x72 for sx1276
//  Modem Config 2 (MC2) == (CRC_ON) | (sf<<4)
//  Modem Config 3 (MC3) == 0x04 | (optional SF11/12 LOW DATA OPTIMIZE 0x08)
//  sf == SF7 default 0x07, (SF7<<4) == SX72_MC2_SF7
//  bw == 125 == 0x70
//  cr == CR4/5 == 0x02
//  CRC_ON == 0x04
// ----------------------------------------------------------------------------
void setRate(uint8_t sf, uint8_t crc)
{
    uint8_t mc1=0, mc2=0, mc3=0;
    // Set rate based on Spreading Factor etc
    if (sx1272) {
        mc1= 0x0A;    // SX1276_MC1_BW_250 0x80 | SX1276_MC1_CR_4_5 0x02
        mc2= (sf<<4) | crc;
        // SX1276_MC1_BW_250 0x80 | SX1276_MC1_CR_4_5 0x02 | SX1276_MC1_IMPLICIT_HEADER_MODE_ON 0x01
        if (sf == SF11 || sf == SF12) { mc1= 0x0B; }
    }
    else {
        mc1= 0x72;    // SX1276_MC1_BW_125==0x70 | SX1276_MC1_CR_4_5==0x02
        mc2= (sf<<4) | crc;  // crc is 0x00 or 0x04==SX1276_MC2_RX_PAYLOAD_CRCON
        mc3= 0x04;    // 0x04; SX1276_MC3_AGCAUTO
        if (sf == SF11 || sf == SF12) { mc3|= 0x08; }  // 0x08 | 0x04
    }

    // Implicit Header (IH), for class b beacons
    //if (getIh(LMIC.rps)) {
    //   mc1 |= SX1276_MC1_IMPLICIT_HEADER_MODE_ON;
    //    writeRegister(REG_PAYLOAD_LENGTH, getIh(LMIC.rps)); // required length
    //}
    writeRegister(REG_MODEM_CONFIG3, mc3);
    writeRegister(REG_MODEM_CONFIG1, mc1);
    writeRegister(REG_MODEM_CONFIG2, mc2);

    // Symbol timeout settings
    if (sf == SF10 || sf == SF11 || sf == SF12) {
        writeRegister(REG_SYMB_TIMEOUT_LSB,0x05);
    } else {
        writeRegister(REG_SYMB_TIMEOUT_LSB,0x08);
    }
    return;
}

// ----------------------------------------------------------------------------
// Set the frequency for our gateway
// The function has no parameter other than the freq setting used in init.
// Since we are usin a 1ch gateway this value is set fixed.
// ----------------------------------------------------------------------------
void setFreq(uint32_t freq)
{
    if (debug >= 4) {
        Serial.printf("setFreq:: freq %d\r\n", freq);
    }
    // set frequency
    uint64_t frf = ((uint64_t)freq << 19) / 32000000;
    writeRegister(REG_FRF_MSB, (uint8_t)(frf>>16) );
    writeRegister(REG_FRF_MID, (uint8_t)(frf>> 8) );
    writeRegister(REG_FRF_LSB, (uint8_t)(frf>> 0) );

    return;
}

// ----------------------------------------------------------------------------
// Set Power for our gateway
// ----------------------------------------------------------------------------
void setPow(uint8_t powe)
{
    if (powe >= 16) powe = 15;
    //if (powe >= 15) powe = 14;
    else if (powe < 2) powe =2;

    uint8_t pac = 0x80 | (powe & 0xF);
    writeRegister(REG_PAC,pac);        // set 0x09 to pac

    // XXX Power settings for CFG_sx1272 are different
    return;
}

// ----------------------------------------------------------------------------
// Used to set the radio to LoRa mode (transmitter)
// ----------------------------------------------------------------------------
static void opmodeLora()
{
    uint8_t u = OPMODE_LORA;
#ifdef CFG_sx1276_radio
    u |= 0x8;              // TBD: sx1276 high freq
#endif
    writeRegister(REG_OPMODE, u);
}

// ----------------------------------------------------------------------------
// Set the opmode to a value as defined on top
// Values are 0x00 to 0x07
// ----------------------------------------------------------------------------
static void opmode(uint8_t mode)
{
    writeRegister(REG_OPMODE, (readRegister(REG_OPMODE) & ~OPMODE_MASK) | mode);
}

// ----------------------------------------------------------------------------
// Hop to next frequency as defined by NUM_HOPS
// This function should only be used for receiver operation. The current
// receiver frequency is determined by ifreq index like so: freqs[ifreq]
// ----------------------------------------------------------------------------
void hop() {
    // If we are already in a hop function, do not proceed
    if (!inHop) {
        //if (debug>=2) {
        // Serial.print(F("HOP:: CH="));
        // Serial.print(ifreq);
        // Serial.print(F(", T="));
        // Serial.println((long)(nowTime-hopTime));
        //}
        inHop=true;
        opmode(OPMODE_STANDBY);
        ifreq = (ifreq + 1)% NUM_HOPS ;
        setFreq(freqs[ifreq]);
        hopTime = micros();        // record HOP moment
        opmode(OPMODE_CAD);
        inHop=false;
    }
    else {
        if (debug >= 3) Serial.println(F("Hop:: Re-entrance try"));
    }
}

// ----------------------------------------------------------------------------
// This DOWN function sends a payload to the LoRa node over the air
// Radio must go back in standby mode as soon as the transmission is finished
// ----------------------------------------------------------------------------
bool sendPkt(uint8_t *payLoad, uint8_t payLength, uint32_t tmst)
{
    writeRegister(REG_FIFO_ADDR_PTR, readRegister(REG_FIFO_TX_BASE_AD)); // 0x0D, 0x0E
    writeRegister(REG_PAYLOAD_LENGTH, payLength);    // 0x22
    for(int i = 0; i < payLength; i++)
    {
        writeRegister(REG_FIFO, payLoad[i]);     // 0x00
    }
    return true;
}

// ----------------------------------------------------------------------------
// loraWait()
// This function implements the wait protocol needed for downstream transmissions.
// Note: Timing of downstream and JoinAccept messages is VERY critical.
//
// As the ESP8266 watchdog will not like us to wait more than a few hundred
// milliseconds (or it will kick in) we have to implement a simple way to wait
// time in case we have to wait seconds before sending messages (e.g. for OTAA 5 or 6 seconds)
// Without it, the system is known to crash in half of the cases it has to wait for
// JOIN-ACCEPT messages to send.
//
// This function uses a combination of delay() statements and delayMicroseconds().
// As we use delay() only when there is still enough time to wait and we use micros()
// to make sure that delay() did not take too much time this works.
//
// Parameter: uint32-t tmst gives the micros() value when transmission should start.
// ----------------------------------------------------------------------------
void loraWait(uint32_t tmst)
{
    uint32_t startTime = micros();      // Start of the loraWait function
    tmst += txDelay;
    uint32_t waitTime = tmst - micros();

    while (waitTime > 16000) {
        delay(15);          // ms delay() including yield, slightly shorter
        waitTime= tmst - micros();
    }
    if (waitTime>0) delayMicroseconds(waitTime);
    else if ((waitTime+20) < 0) Serial.println(F("loraWait TOO LATE"));

    if (debug >=1) {
        Serial.print(F("start: "));
        Serial.print(startTime);
        Serial.print(F(", end: "));
        Serial.print(tmst);
        Serial.print(F(", waited: "));
        Serial.print(tmst - startTime);
        Serial.print(F(", delay="));
        Serial.print(txDelay);
        Serial.println();
    }
}

// ----------------------------------------------------------------------------
// txLoraModem
// Init the transmitter and transmit the buffer
// After successful transmission (dio0==1) TxDone re-init the receiver
//
// crc is set to 0x00 for TX
// iiq is set to 0x27 (or 0x40 based on ipol value in txpkt)
//
// 1. opmodeLora
// 2. opmode StandBY
// 3. Configure Modem
// 4. Configure Channel
// 5. write PA Ramp
// 6. config Power
// 7. RegLoRaSyncWord LORA_MAC_PREAMBLE
// 8. write REG dio mapping (dio0)
// 9. write REG IRQ flags
// 10. write REG IRQ flags mask
// 11. write REG LoRa Fifo Base Address
// 12. write REG LoRa Fifo Addr Ptr
// 13. write REG LoRa Payload Length
// 14. Write buffer (byte by byte)
// 15. Wait until the right time to transmit has arrived
// 16. opmode TX
// ----------------------------------------------------------------------------
static void txLoraModem(uint8_t *payLoad, uint8_t payLength, uint32_t tmst, uint8_t sfTx,
        uint8_t powe, uint32_t freq, uint8_t crc, uint8_t iiq)
{
    if (debug>=1) {
        // Make sure that all serial stuff is done before continuing
        Serial.print(F("txLoraModem::"));
        Serial.print(F("  powe: ")); Serial.print(powe);
        Serial.print(F(", freq: ")); Serial.print(freq);
        Serial.print(F(", crc: ")); Serial.print(crc);
        Serial.print(F(", iiq: 0X")); Serial.print(iiq,HEX);
        Serial.println();
        Serial.flush();
    }
    _state = S_TX;

    // 1. Select LoRa modem from sleep mode
    opmodeLora();            // set register 0x01 to 0x80

    // Assert the value of the current mode
    ASSERT((readRegister(REG_OPMODE) & OPMODE_LORA) != 0);

    // 2. enter standby mode (required for FIFO loading))
    opmode(OPMODE_STANDBY);          // set 0x01 to 0x01

    // 3. Init spreading factor and other Modem setting
    setRate(sfTx, crc);

    // Frquency hopping
    //writeRegister(REG_HOP_PERIOD, 0x00);      // set 0x24 to 0x00 only for receivers

    // 4. Init Frequency, config channel
    setFreq(freq);

    // 6. Set power level, REG_PAC
    setPow(powe);

    // 7. prevent node to node communication
    writeRegister(REG_INVERTIQ,iiq);       // 0x33, (0x27 or 0x40)

    // 8. set the IRQ mapping DIO0=TxDone DIO1=NOP DIO2=NOP (or lesss for 1ch gateway)
    writeRegister(REG_DIO_MAPPING_1, MAP_DIO0_LORA_TXDONE|MAP_DIO1_LORA_NOP|MAP_DIO2_LORA_NOP);

    // 9. clear all radio IRQ flags
    writeRegister(REG_IRQ_FLAGS, 0xFF);

    // 10. mask all IRQs but TxDone
    writeRegister(REG_IRQ_FLAGS_MASK, ~IRQ_LORA_TXDONE_MASK);

    // txLora
    opmode(OPMODE_FSTX);          // set 0x01 to 0x02 (actual value becomes 0x82)

    // 11, 12, 13, 14. write the buffer to the FiFo
    sendPkt(payLoad, payLength, tmst);

    // 15. wait extra delay out. The delayMicroseconds timer is accurate until 16383 uSec.
    loraWait(tmst);

    digitalWrite(pins.rxtx, HIGH);

    // 16. Initiate actual transmission of FiFo
    opmode(OPMODE_TX);          // set 0x01 to 0x03 (actual value becomes 0x83)

    // Reset the IRQ register
    writeRegister(REG_IRQ_FLAGS, 0xFF);      // set 0x12 to 0xFF
}

// ----------------------------------------------------------------------------
// Setup the LoRa receiver on the connected transceiver.
// - Determine the correct transceiver type (sx1272/RFM92 or sx1276/RFM95)
// - Set the frequency to listen to (1-channel remember)
// - Set Spreading Factor (standard SF7)
// The reset RST pin might not be necessary for at least the RGM95 transceiver
//
// 1. Put the radio in LoRa mode
// 2. Put modem in sleep or in standby
// 3. Set Frequency
// ----------------------------------------------------------------------------
void rxLoraModem()
{
#if 0
    Serial.printf("rxLoraModem\r\n");
    // 1. Put system in LoRa mode
    opmodeLora();

    // 2. Put the radio in sleep mode
    opmode(OPMODE_STANDBY);          // set 0x01 to 0x00
    // 4. Set spreading Factor and CRC
    //setRate(sf, 0x04);
    writeRegister(0x1d,0x72);
    writeRegister(0x1e,0x70);
    writeRegister(0x26,0x04);

    // 3. Set frequency based on value in freq
    setFreq(freqs[ifreq]);          // set to 434MHz

    writeRegister(REG_LNA, 0x21);        // 0x0C, 0x23
    writeRegister(REG_MAX_PAYLOAD_LENGTH,0x40);     // set 0x23 to 0x80
    // prevent node to node communication
    writeRegister(REG_INVERTIQ,0x67);       // 0x33, 0x27; to reset from TX
    writeRegister(REG_SYMB_TIMEOUT_LSB,0x05);       // 0x33, 0x27; to reset from TX

    writeRegister(REG_SYNC_WORD, 0x34);       // set 0x39 to 0x34 LORA_MAC_PREAMBLE
    writeRegister(REG_PADAC_SX1276,  0x84);      // set 0x4D (PADAC) to 0x84
    writeRegister(REG_DIO_MAPPING_1, 0xC0);
    writeRegister(REG_IRQ_FLAGS, 0xFF);
    writeRegister(REG_IRQ_FLAGS_MASK, 0x3f);
    digitalWrite(pins.rxtx, LOW);
    opmode(OPMODE_RX);        // 0x80 | 0x06 (listen one message)
    _state= S_RX;
    return;
#endif
#if 1
    Serial.printf("rxLoraModem\r\n");
    // 1. Put system in LoRa mode
    opmodeLora();

    // 2. Put the radio in sleep mode
    opmode(OPMODE_SLEEP);          // set 0x01 to 0x00

    // 3. Set frequency based on value in freq
    setFreq(freqs[ifreq]);          // set to 434MHz
    writeRegister(REG_SYNC_WORD, 0x34);       // set 0x39 to 0x34 LORA_MAC_PREAMBLE

    // 4. Set spreading Factor and CRC
    setRate(sf, 0x04);

    // prevent node to node communication
    writeRegister(REG_INVERTIQ,0x27);       // 0x33, 0x27; to reset from TX

    // Max Payload length is dependent on 256 byte buffer. At startup TX starts at
    // 0x80 and RX at 0x00. RX therefore maximized at 128 Bytes
    writeRegister(REG_MAX_PAYLOAD_LENGTH,0x80);     // set 0x23 to 0x80
    writeRegister(REG_PAYLOAD_LENGTH,PAYLOAD_LENGTH);   // 0x22, 0x40; Payload is 64Byte long
    writeRegister(REG_HOP_PERIOD,0x00);      // 0x24, 0x00 was 0xFF

    writeRegister(REG_FIFO_ADDR_PTR, readRegister(REG_FIFO_RX_BASE_AD)); // set 0x0D to 0x0F

    // Low Noise Amplifier used in receiver
    writeRegister(REG_LNA, LNA_MAX_GAIN);        // 0x0C, 0x23

    // 9. clear all radio IRQ flags
    //writeRegister(REG_IRQ_FLAGS, 0xFF);

    // Accept no interrupts except RXDONE
    //writeRegister(REG_IRQ_FLAGS_MASK, ~(IRQ_LORA_RXDONE_MASK | IRQ_LORA_RXTOUT_MASK));
    writeRegister(REG_IRQ_FLAGS_MASK, ~(IRQ_LORA_RXDONE_MASK));

    // set frequency hopping
    if (_hop) {
        if (debug >= 3) { Serial.print(F("rxLoraModem:: Hop, channel=")); Serial.println(ifreq); }
        writeRegister(REG_HOP_PERIOD,0x01);      // 0x24, 0x01 was 0xFF
        // Set RXDONE interrupt to dio0
        writeRegister(REG_DIO_MAPPING_1, MAP_DIO0_LORA_RXDONE | MAP_DIO1_LORA_RXTOUT | MAP_DIO1_LORA_FCC);
    }
    else {
        Serial.printf("1111\r\n");
        //writeRegister(REG_HOP_PERIOD,0x00);      // 0x24, 0x00 was 0xFF
        // Set RXDONE interrupt to dio0
        writeRegister(REG_DIO_MAPPING_1, MAP_DIO0_LORA_RXDONE | MAP_DIO1_LORA_RXTOUT);
    }

    digitalWrite(pins.rxtx, LOW);

    if (_cad) {
        Serial.printf("_cad\r\n");
        _state= S_RX;
        // Set Single Receive Mode, goes in STANDBY mode after receipt
        opmode(OPMODE_RX_SINGLE);        // 0x80 | 0x06 (listen one message)
    }
    else {
        _state= S_RX;
        // Set Continous Receive Mode
        opmode(OPMODE_RX);          // 0x80 | 0x05 (listen)
        readRegister(REG_OPMODE);
    }

    return;
#endif

#if 0
    Serial.printf("rxLoraModem\r\n");
    // 1. Put system in LoRa mode
    opmodeLora();

    // 2. Put the radio in sleep mode
    opmode(OPMODE_SLEEP);          // set 0x01 to 0x00

    // 3. Set frequency based on value in freq
    setFreq(freqs[ifreq]);          // set to 434MHz

    // 4. Set spreading Factor and CRC
    setRate(sf, 0x04);

    // prevent node to node communication
    writeRegister(REG_INVERTIQ,0x27);       // 0x33, 0x27; to reset from TX

    // Max Payload length is dependent on 256 byte buffer. At startup TX starts at
    // 0x80 and RX at 0x00. RX therefore maximized at 128 Bytes
    writeRegister(REG_MAX_PAYLOAD_LENGTH,0x80);     // set 0x23 to 0x80
    writeRegister(REG_PAYLOAD_LENGTH,PAYLOAD_LENGTH);   // 0x22, 0x40; Payload is 64Byte long

    writeRegister(REG_FIFO_ADDR_PTR, readRegister(REG_FIFO_RX_BASE_AD)); // set 0x0D to 0x0F

    // Low Noise Amplifier used in receiver
    writeRegister(REG_LNA, LNA_MAX_GAIN);        // 0x0C, 0x23

    // 9. clear all radio IRQ flags
    writeRegister(REG_IRQ_FLAGS, 0xFF);

    // Accept no interrupts except RXDONE
    writeRegister(REG_IRQ_FLAGS_MASK, ~(IRQ_LORA_RXDONE_MASK | IRQ_LORA_RXTOUT_MASK));

    // set frequency hopping
    if (_hop) {
        if (debug >= 3) { Serial.print(F("rxLoraModem:: Hop, channel=")); Serial.println(ifreq); }
        writeRegister(REG_HOP_PERIOD,0x01);      // 0x24, 0x01 was 0xFF
        // Set RXDONE interrupt to dio0
        writeRegister(REG_DIO_MAPPING_1, MAP_DIO0_LORA_RXDONE | MAP_DIO1_LORA_RXTOUT | MAP_DIO1_LORA_FCC);
    }
    else {
        Serial.printf("1111\r\n");
        writeRegister(REG_HOP_PERIOD,0x00);      // 0x24, 0x00 was 0xFF
        // Set RXDONE interrupt to dio0
        writeRegister(REG_DIO_MAPPING_1, MAP_DIO0_LORA_RXDONE | MAP_DIO1_LORA_RXTOUT);
    }

    digitalWrite(pins.rxtx, LOW);

    if (_cad) {
        Serial.printf("_cad\r\n");
        _state= S_RX;
        // Set Single Receive Mode, goes in STANDBY mode after receipt
        opmode(OPMODE_RX_SINGLE);        // 0x80 | 0x06 (listen one message)
    }
    else {
        _state= S_RX;
        // Set Continous Receive Mode
        opmode(OPMODE_RX);          // 0x80 | 0x05 (listen)
    }

    return;
#endif
}

// ----------------------------------------------------------------------------
// First time initialisation of the LoRa modem
// Subsequent changes to the modem state etc. done by txLoraModem or rxLoraModem
// After initialisation the modem is put in rxContinuous mode (listen)
// ----------------------------------------------------------------------------
void initLoraModem()
{
#if 1
    Serial.printf("initLoraModem\r\n");
    _state = S_INIT;
    digitalWrite(pins.rst, LOW);
    delay(100);
    digitalWrite(pins.rst, HIGH);
    delay(100);

    opmode(OPMODE_SLEEP);     // set register 0x01 to 0x00

    uint8_t version = readRegister(REG_VERSION);    // Read the LoRa chip version id
    if (version == 0x22) {
        // sx1272
        Serial.println(F("WARNING:: SX1272 detected"));
        sx1272 = true;
    }
    else if(version == 0x12) {
        // sx1276
        if (debug >=1) Serial.println(F("SX1276 detected, starting."));
        sx1272 = false;
    } else {
        Serial.print(F("Unrecognized transceiver, version: "));
        Serial.println(version,HEX);
    }
/*
    // 1 Set LoRa Mode
    opmodeLora();             // set register 0x01 to 0x80

    // 2. Set radio to sleep
    //opmode(OPMODE_SLEEP);     // set register 0x01 to 0x00
    opmode(OPMODE_STANDBY);     // set register 0x01 to 0x00

    // 3. Set frequency based on value in freq
    setFreq(freq);            // set to 434MHz

    // 4. Set spreading Factor
    setRate(sf, 0x04);

    // Low Noise Amplifier used in receiver
    writeRegister(REG_LNA, LNA_MAX_GAIN);        // 0x0C, 0x23

    // 7. set sync word
    writeRegister(REG_SYNC_WORD, 0x34);       // set 0x39 to 0x34 LORA_MAC_PREAMBLE

    // prevent node to node communication
    writeRegister(REG_INVERTIQ,0x27);       // 0x33, 0x27; to reset from TX

    // Max Payload length is dependent on 256 byte buffer. At startup TX starts at
    // 0x80 and RX at 0x00. RX therefore maximized at 128 Bytes
    writeRegister(REG_MAX_PAYLOAD_LENGTH,0x80);         // set 0x23 to 0x80
    writeRegister(REG_PAYLOAD_LENGTH,PAYLOAD_LENGTH);   // 0x22, 0x40; Payload is 64Byte long

    writeRegister(REG_FIFO_ADDR_PTR, readRegister(REG_FIFO_RX_BASE_AD)); // set reg 0x0D to 0x0F

    if (_hop) {
        writeRegister(REG_HOP_PERIOD,0x00);      // reg 0x24, set to 0x00 was 0xFF
        writeRegister(REG_HOP_CHANNEL,0x00);     // reg 0x1C, set to 0x00
    }
    else {
        writeRegister(REG_HOP_PERIOD,0x00);      // reg 0x24, set to 0x00 was 0xFF
    }

    // 5. Config PA Ramp up time        // set reg 0x0A
    writeRegister(REG_PARAMP, (readRegister(REG_PARAMP) & 0xF0) | 0x08); // set PA ramp-up time 50 uSec

    // Set 0x4D PADAC for SX1276 ; XXX register is 0x5a for sx1272
    writeRegister(REG_PADAC_SX1276,  0x84);      // set 0x4D (PADAC) to 0x84
    //writeRegister(REG_PADAC, readRegister(REG_PADAC)|0x4);

    // 9. clear all radio IRQ flags
    writeRegister(REG_IRQ_FLAGS, 0xFF);
    */
#endif
#if 0
    Serial.printf("initLoraModem\r\n");
    _state = S_INIT;
    digitalWrite(pins.rst, LOW);
    delay(100);
    digitalWrite(pins.rst, HIGH);
    delay(100);

    uint8_t version = readRegister(REG_VERSION);    // Read the LoRa chip version id
    if (version == 0x22) {
        // sx1272
        Serial.println(F("WARNING:: SX1272 detected"));
        sx1272 = true;
    }
    else if(version == 0x12) {
        // sx1276
        if (debug >=1) Serial.println(F("SX1276 detected, starting."));
        sx1272 = false;
    } else {
        Serial.print(F("Unrecognized transceiver, version: "));
        Serial.println(version,HEX);
    }

    // 1 Set LoRa Mode
    opmodeLora();             // set register 0x01 to 0x80

    // 2. Set radio to sleep
    opmode(OPMODE_SLEEP);     // set register 0x01 to 0x00

    // 3. Set frequency based on value in freq
    setFreq(freq);            // set to 434MHz

    // 4. Set spreading Factor
    setRate(sf, 0x04);

    Serial.printf("freq = %d, sf = %d\r\n", freq, sf);
    // Low Noise Amplifier used in receiver
    writeRegister(REG_LNA, LNA_MAX_GAIN);        // 0x0C, 0x23

    // 7. set sync word
    writeRegister(REG_SYNC_WORD, 0x34);       // set 0x39 to 0x34 LORA_MAC_PREAMBLE

    // prevent node to node communication
    writeRegister(REG_INVERTIQ,0x27);       // 0x33, 0x27; to reset from TX

    // Max Payload length is dependent on 256 byte buffer. At startup TX starts at
    // 0x80 and RX at 0x00. RX therefore maximized at 128 Bytes
    writeRegister(REG_MAX_PAYLOAD_LENGTH,0x80);         // set 0x23 to 0x80
    writeRegister(REG_PAYLOAD_LENGTH,PAYLOAD_LENGTH);   // 0x22, 0x40; Payload is 64Byte long

    writeRegister(REG_FIFO_ADDR_PTR, readRegister(REG_FIFO_RX_BASE_AD)); // set reg 0x0D to 0x0F

    if (_hop) {
        writeRegister(REG_HOP_PERIOD,0x00);      // reg 0x24, set to 0x00 was 0xFF
        writeRegister(REG_HOP_CHANNEL,0x00);     // reg 0x1C, set to 0x00
    }
    else {
        writeRegister(REG_HOP_PERIOD,0x00);      // reg 0x24, set to 0x00 was 0xFF
    }

    // 5. Config PA Ramp up time        // set reg 0x0A
    writeRegister(REG_PARAMP, (readRegister(REG_PARAMP) & 0xF0) | 0x08); // set PA ramp-up time 50 uSec

    // Set 0x4D PADAC for SX1276 ; XXX register is 0x5a for sx1272
    writeRegister(REG_PADAC_SX1276,  0x84);      // set 0x4D (PADAC) to 0x84
    //writeRegister(REG_PADAC, readRegister(REG_PADAC)|0x4);

    // 9. clear all radio IRQ flags
    writeRegister(REG_IRQ_FLAGS, 0xFF);
#endif
}

// ----------------------------------------------------------------------------
// Send DOWN a LoRa packet over the air to the node. This function does all the
// decoding of the server message and prepares a Payload buffer.
// The payload is actually transmitted by the sendPkt() function.
// This function is used for regular downstream messages and for JOIN_ACCEPT
// messages.
// ----------------------------------------------------------------------------
int sendPacket(uint8_t *buff_down, uint8_t length)
{
    // Received package with Meta Data:
    // codr : "4/5"
    // data : "Kuc5CSwJ7/a5JgPHrP29X9K6kf/Vs5kU6g==" // for example
    // freq : 868.1          // 868100000
    // ipol : true/false
    // modu : "LORA"
    // powe : 14          // Set by default
    // rfch : 0           // Set by default
    // size : 21
    // tmst : 1800642          // for example
    // datr : "SF7BW125"

    // 12-byte header;
    //  HDR (1 byte)
    //
    //
    // Data Reply for JOIN_ACCEPT as sent by server:
    //  AppNonce (3 byte)
    //  NetID (3 byte)
    //  DevAddr (4 byte) [ 31..25]:NwkID , [24..0]:NwkAddr
    //  DLSettings (1 byte)
    //  RxDelay (1 byte)
    //  CFList (fill to 16 bytes)
    int i=0;
    bool flag = true;
    aJsonClass aJson;

    if (debug >= 2) Serial.println((char *)buff_down);
    aJsonObject *root = aJson.parse((char *)buff_down);
    if (root == NULL)
    {
        Serial.print (F("sendPacket:: ERROR Json Decode "));
        if (debug>=2) {
            Serial.print(": ");
            Serial.println((char *)buff_down);
        }
        return(-1);
    }

    // Meta Data sent by server (example)
    // {"txpk":{"codr":"4/5","data":"YCkEAgIABQABGmIwYX/kSn4Y","freq":868.1,"ipol":true,"modu":"LORA","powe":14,"rfch":0,"size":18,"tmst":1890991792,"datr":"SF7BW125"}}
    aJsonObject* txpkObject = aJson.getObjectItem(root, "txpk");
    if (txpkObject == NULL)
    {flag = false;}

    // Used in the protocol:
    aJsonObject* dataObject = aJson.getObjectItem(txpkObject, "data");
    if (dataObject == NULL)
    {flag = false;}
    const char * data = dataObject->valuestring;

    aJsonObject* sizeObject = aJson.getObjectItem(txpkObject, "size");
    if (sizeObject == NULL)
    {flag = false;}
    uint8_t psize = sizeObject->valueint;

    aJsonObject* ipolObject = aJson.getObjectItem(txpkObject, "ipol");
    if (ipolObject == NULL)
    {flag = false;}
    bool ipol = ipolObject->valuebool;

    aJsonObject* poweObject = aJson.getObjectItem(txpkObject, "powe");
    if (poweObject == NULL)
    {flag = false;}
    uint8_t powe = poweObject->valueint;

    aJsonObject* tmstObject = aJson.getObjectItem(txpkObject, "tmst");
    if (tmstObject == NULL)
    {flag = false;}
    uint32_t tmst = (uint32_t)tmstObject->valueint;

    // Not used in the protocol:
    aJsonObject* datrObject = aJson.getObjectItem(txpkObject, "datr");
    if (datrObject == NULL)
    {flag = false;}
    const char * datr = datrObject->valuestring;   // eg "SF7BW125"

    aJsonObject* freqObject = aJson.getObjectItem(txpkObject, "freq");
    if (freqObject == NULL)
    {flag = false;}
    const float ff  = freqObject->valuefloat;   // eg 869.525

    aJsonObject* moduObject = aJson.getObjectItem(txpkObject, "modu");
    if (moduObject == NULL)
    {flag = false;}
    const char * modu = moduObject->valuestring;   // =="LORA"

    aJsonObject* codrObject = aJson.getObjectItem(txpkObject, "codr");
    if (codrObject == NULL)
    {flag = false;}
    const char * codr = codrObject->valuestring;

    if (false == flag)
    {
        aJson.deleteItem(root);
        return(-1);
    }

    if (data != NULL) {
        if (debug>=2) { Serial.print(F("data: ")); Serial.println((char *) data); }
    }
    else {
        Serial.println(F("sendPacket:: ERROR data is NULL"));
        return(-1);
    }

    uint8_t iiq = (ipol? 0x40: 0x27);     // if ipol==true 0x40 else 0x27
    uint8_t crc = 0x00;         // switch CRC off for TX
    uint8_t payLength = base64_dec_len((char *)data, strlen(data));
    uint8_t payLoad[payLength];       // Declare buffer
    base64_decode((char *) payLoad, (char *) data, strlen(data));

    // Compute wait time in microseconds
    uint32_t w = (uint32_t) (tmst - micros());

#if _STRICT_1CH == 1
    // Use RX1 timeslot as this is our frequency.
    // Do not use RX2 or JOIN2 as they contain other frequencies
    if ((w>1000000) && (w<3000000)) { tmst-=1000000; }
    else if ((w>6000000) && (w<7000000)) { tmst-=1000000; }

    const uint8_t sfTx = sfi;       // Take care, TX sf not to be mixed with SCAN
    const uint32_t fff = freq;
    _state = S_TX;          // _state set to transmit
    txLoraModem(payLoad, payLength, tmst, sfTx, powe, fff, crc, iiq);
#else
    const uint8_t sfTx = atoi(datr+2);     // Convert "SF9BW125" to 9
    // convert double frequency (MHz) into uint32_t frequency in Hz.
    const uint32_t fff = (uint32_t) ((uint32_t)((ff+0.000035)*1000)) * 1000;
    _state = S_TX;          // _state set to transmit
    txLoraModem(payLoad, payLength, tmst, sfTx, powe, fff, crc, iiq);
#endif

    if (debug>=2) {
        Serial.print(F("Request:: "));
        Serial.print(F(" tmst=")); Serial.print(tmst); Serial.print(F(" wait=")); Serial.println(w);

        Serial.print(F(" strict=")); Serial.print(_STRICT_1CH);
        Serial.print(F(" datr=")); Serial.println(datr);
        Serial.print(F(" freq=")); Serial.print(freq); Serial.print(F(" ->")); Serial.println(fff);
        Serial.print(F(" sf  =")); Serial.print(sf); Serial.print(F(" ->")); Serial.print(sfTx);

        Serial.print(F(" modu=")); Serial.print(modu);
        Serial.print(F(" powe=")); Serial.print(powe);
        Serial.print(F(" codr=")); Serial.println(codr);

        Serial.print(F(" ipol=")); Serial.println(ipol);
        Serial.println();        // empty line between messages
    }

    if (payLength != psize) {
        Serial.print(F("sendPacket:: WARNING payLength: "));
        Serial.print(payLength);
        Serial.print(F(", psize="));
        Serial.println(psize);
    }
    else if (debug >= 2) {
        for (i=0; i<payLength; i++) {Serial.print(payLoad[i],HEX); Serial.print(':'); }
        Serial.println();
    }

    cp_up_pkt_fwd++;
    aJson.deleteItem(root);
#if 0
    StaticJsonBuffer<256> jsonBuffer;
    char * bufPtr = (char *) (buff_down);
    buff_down[length] = 0;

    if (debug >= 2) Serial.println((char *)buff_down);
    // Use JSON to decode the string after the first 4 bytes.
    // The data for the node is in the "data" field. This function destroys original buffer
    JsonObject& root = jsonBuffer.parseObject(bufPtr);

    if (!root.success()) {
        Serial.print (F("sendPacket:: ERROR Json Decode "));
        if (debug>=2) {
            Serial.print(": ");
            Serial.println(bufPtr);
        }
        return(-1);
    }
    delay(1);
    // Meta Data sent by server (example)
    // {"txpk":{"codr":"4/5","data":"YCkEAgIABQABGmIwYX/kSn4Y","freq":868.1,"ipol":true,"modu":"LORA","powe":14,"rfch":0,"size":18,"tmst":1890991792,"datr":"SF7BW125"}}

    // Used in the protocol:
    const char * data = root["txpk"]["data"];
    uint8_t psize  = root["txpk"]["size"];
    bool ipol   = root["txpk"]["ipol"];
    uint8_t powe  = root["txpk"]["powe"];
    uint32_t tmst  = (uint32_t) root["txpk"]["tmst"].as<unsigned long>();

    // Not used in the protocol:
    const char * datr = root["txpk"]["datr"];   // eg "SF7BW125"
    const float ff  = root["txpk"]["freq"];   // eg 869.525
    const char * modu = root["txpk"]["modu"];   // =="LORA"
    const char * codr = root["txpk"]["codr"];
    //if (root["txpk"].containsKey("imme") ) {
    // const bool imme = root["txpk"]["imme"];   // Immediate Transmit (tmst don't care)
    //}

    if (data != NULL) {
        if (debug>=2) { Serial.print(F("data: ")); Serial.println((char *) data); }
    }
    else {
        Serial.println(F("sendPacket:: ERROR data is NULL"));
        return(-1);
    }

    uint8_t iiq = (ipol? 0x40: 0x27);     // if ipol==true 0x40 else 0x27
    uint8_t crc = 0x00;         // switch CRC off for TX
    uint8_t payLength = base64_dec_len((char *) data, strlen(data));
    uint8_t payLoad[payLength];       // Declare buffer
    base64_decode((char *) payLoad, (char *) data, strlen(data));

    // Compute wait time in microseconds
    uint32_t w = (uint32_t) (tmst - micros());

#if _STRICT_1CH == 1
    // Use RX1 timeslot as this is our frequency.
    // Do not use RX2 or JOIN2 as they contain other frequencies
    if ((w>1000000) && (w<3000000)) { tmst-=1000000; }
    else if ((w>6000000) && (w<7000000)) { tmst-=1000000; }

    const uint8_t sfTx = sfi;       // Take care, TX sf not to be mixed with SCAN
    const uint32_t fff = freq;
    _state = S_TX;          // _state set to transmit
    txLoraModem(payLoad, payLength, tmst, sfTx, powe, fff, crc, iiq);
#else
    const uint8_t sfTx = atoi(datr+2);     // Convert "SF9BW125" to 9
    // convert double frequency (MHz) into uint32_t frequency in Hz.
    const uint32_t fff = (uint32_t) ((uint32_t)((ff+0.000035)*1000)) * 1000;
    _state = S_TX;          // _state set to transmit
    txLoraModem(payLoad, payLength, tmst, sfTx, powe, fff, crc, iiq);
#endif

    if (debug>=2) {
        Serial.print(F("Request:: "));
        Serial.print(F(" tmst=")); Serial.print(tmst); Serial.print(F(" wait=")); Serial.println(w);

        Serial.print(F(" strict=")); Serial.print(_STRICT_1CH);
        Serial.print(F(" datr=")); Serial.println(datr);
        Serial.print(F(" freq=")); Serial.print(freq); Serial.print(F(" ->")); Serial.println(fff);
        Serial.print(F(" sf  =")); Serial.print(sf); Serial.print(F(" ->")); Serial.print(sfTx);

        Serial.print(F(" modu=")); Serial.print(modu);
        Serial.print(F(" powe=")); Serial.print(powe);
        Serial.print(F(" codr=")); Serial.println(codr);

        Serial.print(F(" ipol=")); Serial.println(ipol);
        Serial.println();        // empty line between messages
    }

    if (payLength != psize) {
        Serial.print(F("sendPacket:: WARNING payLength: "));
        Serial.print(payLength);
        Serial.print(F(", psize="));
        Serial.println(psize);
    }
    else if (debug >= 2) {
        for (i=0; i<payLength; i++) {Serial.print(payLoad[i],HEX); Serial.print(':'); }
        Serial.println();
    }

    cp_up_pkt_fwd++;
#endif
    return 1;
}

// ----------------------------------------------------------------------------
// Based on the information read from the LoRa transceiver (or fake message)
// build a gateway message to send upstream.
//
// parameters:
// tmst: Timestamp to include in the upstream message
// buff_up: The buffer that is generated for upstream
// message: The payload message toincludein the the buff_up
// messageLength: The number of bytes received by the LoRa transceiver
// internal: Boolean value to indicate whether the local sensor is processed
//
// ----------------------------------------------------------------------------
int buildPacket(uint32_t tmst, uint8_t *buff_up, uint8_t *message, char messageLength, bool internal)
{
    long SNR;
    int rssicorr;
    int prssi;           // packet rssi
    char cfreq[12] = {0};        // Character array to hold freq in MHz
    lastTmst = tmst;         // Following/according to spec
    int buff_index=0;

    // Read SNR and RSSI from the register. Note: Not for internal sensors!
    // For internal sensor we fake these values as we cannot read a register
    if (internal) {
        SNR = 12;
        rssicorr = 157;
        prssi = 50;
    }
    else {
        uint8_t value = readRegister(REG_PKT_SNR_VALUE); // 0x19;
        if( value & 0x80 ) {        // The SNR sign bit is 1
            // Invert and divide by 4
            value = ( ( ~value + 1 ) & 0xFF ) >> 2;
            SNR = -value;
        }
        else {
            // Divide by 4
            SNR = ( value & 0xFF ) >> 2;
        }

        prssi = readRegister(REG_PKT_RSSI);    // read register 0x1A

        // Correction of RSSI value based on chip used.
        if (sx1272) {          // Is it a sx1272 radio?
            rssicorr = 139;
        } else {           // Probably SX1276 or RFM95
            rssicorr = 157;
        }
    }
#if 0
#if STATISTICS >= 1
    // Receive statistics
    for (int m=( MAX_STAT -1); m>0; m--) statr[m]=statr[m-1];
    statr[0].tmst = millis();
    statr[0].ch= ifreq;
    statr[0].prssi = prssi-rssicorr;
    statr[0].rssi = readRegister(0x1B)-rssicorr;
    statr[0].sf = readRegister(REG_MODEM_CONFIG2) >> 4;\
                  statr[0].node = ( message[4]<<24 | message[3]<<16 | message[2]<<8 | message[1] );
    //sprintf(statr[0].node, "%02X %02X %02X %02X",
    // message[4], message[3], message[2], message[1]);
#if STATISTICS >= 2
    switch (statr[0].sf) {
        case SF7: statc.sf7++; break;
        case SF8: statc.sf8++; break;
        case SF9: statc.sf9++; break;
        case SF10: statc.sf10++; break;
        case SF11: statc.sf11++; break;
        case SF12: statc.sf12++; break;
    }
#endif
#endif
#endif
    if (debug>=1) {
        Serial.print(F("Packet RSSI: "));
        Serial.print(prssi-rssicorr);
        Serial.print(F(" RSSI: "));
        Serial.print(readRegister(0x1B)-rssicorr);
        Serial.print(F(" SNR: "));
        Serial.print(SNR);
        Serial.print(F(" Length: "));
        Serial.print((int)messageLength);
        Serial.print(F(" -> "));
        int i;
        for (i=0; i< messageLength; i++) {
            Serial.print(message[i],HEX);
            Serial.print(' ');
        }
        Serial.println();
    }

    int j;
    // XXX Base64 library is nopad. So we may have to add padding characters until
    //  length is multiple of 4!
    int encodedLen = base64_enc_len(messageLength);  // max 341
    base64_encode(b64, (char *) message, messageLength);// max 341

    // pre-fill the data buffer with fixed fields
    buff_up[0] = PROTOCOL_VERSION;      // 0x01 still
    buff_up[3] = PKT_PUSH_DATA;       // 0x00

    // READ MAC ADDRESS OF ESP8266, and insert 0xFF 0xFF in the middle
    buff_up[4]  = MAC_array[0];
    buff_up[5]  = MAC_array[1];
    buff_up[6]  = MAC_array[2];
    buff_up[7]  = 0xFF;
    buff_up[8]  = 0xFF;
    buff_up[9]  = MAC_array[3];
    buff_up[10] = MAC_array[4];
    buff_up[11] = MAC_array[5];

    // start composing datagram with the header
    uint8_t token_h = (uint8_t)rand();      // random token
    uint8_t token_l = (uint8_t)rand();      // random token
    buff_up[1] = token_h;
    buff_up[2] = token_l;
    buff_index = 12;          // 12-byte binary (!) header

    // start of JSON structure that will make payload
    memcpy((void *)(buff_up + buff_index), (void *)"{\"rxpk\":[", 9);
    buff_index += 9;
    buff_up[buff_index] = '{';
    ++buff_index;
    j = snprintf((char *)(buff_up + buff_index), TX_BUFF_SIZE-buff_index, "\"tmst\":%u", tmst);
    buff_index += j;
    ftoa((double)freq/1000000,cfreq,6);     // XXX This can be done better
    j = snprintf((char *)(buff_up + buff_index), TX_BUFF_SIZE-buff_index, ",\"chan\":%1u,\"rfch\":%1u,\"freq\":%s", 0, 0, cfreq);
    buff_index += j;
    memcpy((void *)(buff_up + buff_index), (void *)",\"stat\":1", 9);
    buff_index += 9;
    memcpy((void *)(buff_up + buff_index), (void *)",\"modu\":\"LORA\"", 14);
    buff_index += 14;
    /* Lora datarate & bandwidth, 16-19 useful chars */
    switch (sf) {
        case SF6:
            memcpy((void *)(buff_up + buff_index), (void *)",\"datr\":\"SF6", 12);
            buff_index += 12;
            break;
        case SF7:
            memcpy((void *)(buff_up + buff_index), (void *)",\"datr\":\"SF7", 12);
            buff_index += 12;
            break;
        case SF8:
            memcpy((void *)(buff_up + buff_index), (void *)",\"datr\":\"SF8", 12);
            buff_index += 12;
            break;
        case SF9:
            memcpy((void *)(buff_up + buff_index), (void *)",\"datr\":\"SF9", 12);
            buff_index += 12;
            break;
        case SF10:
            memcpy((void *)(buff_up + buff_index), (void *)",\"datr\":\"SF10", 13);
            buff_index += 13;
            break;
        case SF11:
            memcpy((void *)(buff_up + buff_index), (void *)",\"datr\":\"SF11", 13);
            buff_index += 13;
            break;
        case SF12:
            memcpy((void *)(buff_up + buff_index), (void *)",\"datr\":\"SF12", 13);
            buff_index += 13;
            break;
        default:
            memcpy((void *)(buff_up + buff_index), (void *)",\"datr\":\"SF?", 12);
            buff_index += 12;
    }
    memcpy((void *)(buff_up + buff_index), (void *)"BW125\"", 6);
    buff_index += 6;
    memcpy((void *)(buff_up + buff_index), (void *)",\"codr\":\"4/5\"", 13);
    buff_index += 13;
    j = snprintf((char *)(buff_up + buff_index), TX_BUFF_SIZE-buff_index, ",\"lsnr\":%li", SNR);
    buff_index += j;
    j = snprintf((char *)(buff_up + buff_index), TX_BUFF_SIZE-buff_index, ",\"rssi\":%d,\"size\":%u", prssi-rssicorr, messageLength);
    buff_index += j;
    memcpy((void *)(buff_up + buff_index), (void *)",\"data\":\"", 9);
    buff_index += 9;

    // Use gBase64 library to fill in the data string
    encodedLen = base64_enc_len(messageLength);   // max 341
    j = base64_encode((char *)(buff_up + buff_index), (char *) message, messageLength);

    buff_index += j;
    buff_up[buff_index] = '"';
    ++buff_index;

    // End of packet serialization
    buff_up[buff_index] = '}';
    ++buff_index;
    buff_up[buff_index] = ']';
    ++buff_index;

    // end of JSON datagram payload */
    buff_up[buff_index] = '}';
    ++buff_index;
    buff_up[buff_index] = 0;        // add string terminator, for safety

    if (debug>=1) {
        Serial.print(F("RXPK:: "));
        Serial.println((char *)(buff_up + 12));   // DEBUG: display JSON payload
    }
    return(buff_index);
}

// ----------------------------------------------------------------------------
// This LoRa function reads a message from the LoRa transceiver
// returns message length read when message correctly received or
// it returns a negative value on error (CRC error for example).
// UP function
// This is the "lowlevel" receive function called by receivePacket()
// dealing with the radio specific LoRa functions
// ----------------------------------------------------------------------------
uint8_t receivePkt(uint8_t *payload)
{
    writeRegister(REG_IRQ_FLAGS, 0x40);      // 0x12; Clear RxDone

    int irqflags = readRegister(REG_IRQ_FLAGS);    // 0x12

    cp_nb_rx_rcv++;           // Receive statistics counter

    //  payload crc=0x20 set
    if((irqflags & 0x20) == 0x20)
    {
        Serial.println(F("CRC error"));
        // Reset CRC flag 0x20
        writeRegister(REG_IRQ_FLAGS, 0x20);     // set 0x12 to 0x20
        return -1;
    } else {

        cp_nb_rx_ok++;          // Receive OK statistics counter

        uint8_t currentAddr = readRegister(REG_FIFO_RX_CURRENT_ADDR); // 0x10
        uint8_t receivedCount = readRegister(REG_RX_NB_BYTES); // 0x13; How many bytes were read

        //writeRegister(REG_FIFO_ADDR_PTR, currentAddr); // 0x0D XXX??? This sets the FiFo higher!!!

        for(int i = 0; i < receivedCount; i++)
        {
            payload[i] = readRegister(REG_FIFO);   // 0x00
        }
        return(receivedCount);
    }
    return 0;
}

// ----------------------------------------------------------------------------
// Receive a LoRa package over the air
//
// Receive a LoRa message and fill the buff_up char buffer.
// returns values:
// - returns the length of string returned in buff_up
// - returns -1 when no message arrived.
//
// This is the "highlevel" function called by loop()
// _state is S_RX when starting and
// _state is S_STANDBY when leaving function
// ----------------------------------------------------------------------------
int receivePacket(uint8_t * buff_up)
{
    long SNR;
    //int rssicorr;
    //char cfreq[12] = {0};         // Character array to hold freq in MHz
    uint8_t message[256];
    uint8_t messageLength = 0;

    // Regular message received, see SX1276 spec table 18
    // Next statement could also be a "while" to combine several messages received
    // in one UDP message as the Semtech Gateway spec does allow this.
    // XXX Not yet supported

    // Take the timestamp as soon as possible, to have accurate reception timestamp
    // TODO: tmst can jump if micros() overflow.
    uint32_t tmst = (uint32_t) micros();    // Only microseconds, rollover in
    lastTmst = tmst;         // Following/according to spec


    // Handle the physical data read from FiFo
    if((messageLength = receivePkt(message)) > 0){

        // external received packet, so last parameter is false
        int buff_index = buildPacket(tmst, buff_up, message, messageLength, false);
#if DEBUG >= 2
        // app behave with buffer correct content
        if (message[0] != 0x40) {
            Serial.print(F("receivedPkt:: <"));
            Serial.print(messageLength);
            Serial.print(F("> Warning "));
        }
#endif
        // debugging AFTER reading the message
        if (debug >= 2) {
            Serial.print(F("receivePacket:: "));
            printState(2);
        }
        return(buff_index);

    } // received a message
    return(-1);        // failure no message read
}

// ----------------------------------------------------------------------------
// cadScanner()
//
// CAD Scanner will scan on the given channel for a valid Symbol/Preamble signal.
// So instead of receiving continuous on a given channel/sf combination
// we will wait on the given channel and scan for a preamble. Once received
// we will set the radio to the SF with best rssi (indicating reception on that sf).
// cadScanner() sets the _state to S_SCAN
// NOTE: DO not set the frequency here but use the frequency hopper
// ----------------------------------------------------------------------------
void cadScanner()
{
    //Serial.print(F("cadScanner:: Started sf=")); Serial.println(sf);
    opmodeLora();

    // Put the radio in standby/sleep mode
    //opmode(OPMODE_STANDBY);
    opmode(OPMODE_SLEEP);          // set 0x01 to 0x00

    // For every time we stat the scanner, we set the SF to the begin value
    sf = SF7;             // XXX MMM So we make SF one lower!

    // As we can come back from S_TX with other frequencies and SF
    // reset both to good values for cadScanner
    setRate(sf, 0x04);

    // listen to LORA_MAC_PREAMBLE
    writeRegister(REG_SYNC_WORD, 0x34);       // set reg 0x39 to 0x34

    // Clear all relevant interrupts
    writeRegister(REG_IRQ_FLAGS, 0xFF );      // May work better, clear ALL interrupts

    // Set the interrupts we want top listen to
    writeRegister(REG_DIO_MAPPING_1,
            MAP_DIO0_LORA_CADDONE | MAP_DIO1_LORA_CADDETECT | MAP_DIO2_LORA_NOP | MAP_DIO3_LORA_NOP);

    // Set the mask for interrupts (we do not want to listen to) except for
    writeRegister(REG_IRQ_FLAGS_MASK, ~(IRQ_LORA_CDDONE_MASK | IRQ_LORA_CDDETD_MASK) );

    // Set the opMode to CAD
    opmode(OPMODE_CAD);

    // If we are here. we either might have set the SF or we have a timeout in which
    // case the receive is started just as normal.
    return;
}

// ----------------------------------------------------------------------------
// eventHandler is the loop() function that handles the incoming events in the
// main loop() such as packet reading etc.
// This function is there to make sure we do not spend too much time in
// interrupt mode but do reading etc in loop().
// So it is not activated by interrupts directly, but interrupts set _state so
// that the buffer can be read in the main loop.
// ----------------------------------------------------------------------------
void eventHandler()
{
    int buff_index=0;
    uint8_t rssi;

    // The type of interrupt is dependent on the state we are in
    // at the moment of receiving.
    switch (_state) {

        // state tells us that the system was receiving a LoRa message
        // and that all data is now in the FIFO
        case S_RXDONE:
            if (debug >=4) { Serial.print(F("eH:: receive on SF=")); Serial.println(sf); }

            if ((buff_index = receivePacket(buff_up)) >= 0) { // read is successful
                // rxpk PUSH_DATA received from node is rxpk (*2, par. 3.2)
                sendUdp(buff_up, buff_index);  // send to 1 or 2 sockets
            }
            else {
                Serial.print(F("eH:: WARNING S_RX empty message, sf="));
                Serial.println(sf);
            }
            _state = S_RX;
            rxLoraModem();       // Reset all RX lora stuff

            // If we now switch to S_SCAN, we have to hop too
            if (_hop) {
                hop();
            }

            if (_cad) {
                // Set the state to CAD scanning after receiving
                _state = S_SCAN;      // Inititialise scanner
                cadScanner();
            }
            break;

        default:
            if (debug >= 2) {
                Serial.print(F("eH:: Unrecognized state, "));
                printState(2);
            }
            break;
    }//switch
}

// ----------------------------------------------------------------------------
// Interrupt handler for DIO0 having High Value
// Handler for:
//  - RxDone
//  - TxDone
//  - CadDone
// This interrupt routine has been kept as simple and short as possible.
// Thsi means that the handler will ONLY update the _state variable for
// reception (and do nothing for RXDONE events).
// Assuming tha the next sensible event after RXDONE is receive characters
// anyway.
// NOTE: We may clear the interrupt but l;eave the flag for the moment.
// The eventHandler // should take care of repairing flags between interrupts.
// ----------------------------------------------------------------------------
void Interrupt_0()
{
    Serial.println(F("Interrupt_0"));
    // Determine what interrupt flags are set
    flags = readRegister(REG_IRQ_FLAGS);
    mask = readRegister(REG_IRQ_FLAGS_MASK);
    uint8_t intr = flags & ( ~ mask );   // Only react on non masked interrupts
    uint8_t rssi;

    if (intr == 0x00) {
        if (debug>=4) Serial.println(F("DIO0:: NO intr"));
        return;
    }

    // Small state machine inside the interrupt handler
    // as next actions are depending on the state we are in.
    switch (_state) {

        // If the state is init, we are starting up.
        // The initLoraModem() function is already called ini setup();
        case S_INIT:
            if (debug >= 1) { Serial.println(F("DIO:: S_INIT")); }
            // new state, needed to startup the radio (to S_SCAN)
            break;

            // In S_SCAN we measure a high RSSI this means that there (probably) is a message
            // coming in at that freq. So find the right SF with CDDECTD.
        case S_SCAN:
            // Clear the CADDONE flag
            writeRegister(REG_IRQ_FLAGS, IRQ_LORA_CDDONE_MASK);

            //delayMicroseconds(250);      //XXX MMM Wait some microseconds to settle
            rssi = readRegister(REG_RSSI);    // Read the RSSI
            opmode(OPMODE_CAD);

            if ( rssi > RSSI_LIMIT ) {
                if (debug >= 4) {
                    Serial.print(F("DIO0:: CADDONE: "));
                    printState(4);
                }
                _state = S_CAD;       // XXX invoke the interrupt handler again?
            }

            // If we do not switch to S_SCAN, we have to hop
            // Instead of waiting for an interrupt we do this on timer bais (more regular).
            else if (_hop) { hop(); }

            // else keep on scanning
            break;

            // S_CAD: In CAD mode we scan every SF for high RSSI until we have a DETECT
            // DIO0 interrupt IRQ_LORA_CDDONE_MASK in state S_CAD==2 means that we might have
            // a lock on the Freq but not the right SF. So we increase the SF
        case S_CAD:
            if ((uint8_t)sf < 12) {
                sf = (sf_t)((uint8_t)sf + 1);   // XXX This would mean SF7 never used
                setRate(sf, 0x04);      // Set SF with CRC==on

                // reset interrupt flags for CAD Done
                writeRegister(REG_IRQ_FLAGS, IRQ_LORA_CDDONE_MASK );
                opmode(OPMODE_CAD);

                delayMicroseconds( RSSI_WAIT );
                rssi = readRegister(REG_RSSI);   // Read the RSSI
            }
            // if we are at SF12 level Restart the scanning process
            else {
                if (debug>=4) { Serial.println(F("DIO0:: SF Reset, restart scanner")); }
                if (_hop) { hop(); }     // XXX
                _state = S_SCAN;
                cadScanner();
            }
            break;

            // If we receice an interrupt on dio0 state==S_RX
            // it should be a RxDone interrupt
            // So we should handle the received message
        case S_RX:
            if (intr & IRQ_LORA_RXDONE_MASK) {
                _state=S_RXDONE;      // Will be picked up by eventHandler() in loop()
                writeRegister(REG_IRQ_FLAGS, 0xFF);  // reset all interrupts (have to wait until received);
                if (debug>=3) {
                    Serial.println(F("DIO0:: RXDONE, "));
                    printState(3);
                }
            }
            else if (debug >=3) {
                Serial.print(F("DIO0:: S_RX but no RXDONE, "));
                printState(3);
            }
            break;

            // If we receive an interrupt on dio0 state==S_TX==5 it is a TxDone interrupt
            // Do nothing with the interrupt, it is just an indication
            // Switch back to scanner mode after transmission finished OK
        case S_TX:
            if (debug>=3) { Serial.print(F("DIO0:: S_TX, ")); printState(3); }
            _state=S_RX;
            rxLoraModem();

            if (_cad) {
                // Set the state to CAD scanning
                _state = S_SCAN;
                cadScanner();        // Start the scanner after TX cycle
            }
            break;

        default:
            if (debug >= 2) { Serial.print("DIO0:: Unexpected state="); Serial.println(_state); }
            break;
    }
}

// ----------------------------------------------------------------------------
// Interrupt handler for DIO1 having High Value
// As DIO0 and DIO1 may be multiplexed on one GPIO inteerupt handler
// (as we do) we have to be carefull only to call the right Interrupt_x
// handler and clear the corresponding interrupts for that dio.
// NOTE: Make sure all Serial communication is only for debug level 3 and up.
// ----------------------------------------------------------------------------
void Interrupt_1()
{
    Serial.println(F("Interrupt_1"));
    flags = readRegister(REG_IRQ_FLAGS);
    mask = readRegister(REG_IRQ_FLAGS_MASK);
    uint8_t intr = flags & ( ~ mask );   // Only react on non masked interrupts
    uint8_t rssi;

    if (intr == 0x00) {
        if (debug>=4) Serial.println(F("DIO1:: NO intr"));
        return;
    }

    switch(_state) {
        // If we receive an interrupt on dio1 and _state==S_CAD or S_SCAN
        case S_CAD:
        case S_SCAN:

            // We have to set the sf based on a strong RSSI for this channel
            // So we scan this SF and if not high enough ... next
            if (intr & IRQ_LORA_CDDETD_MASK) {

                if (debug >=3) {
                    Serial.print(F("DIO1:: CADDETD, "));
                    printState(3);
                }

                _state = S_RX;       // Set state to receiving
                opmode(OPMODE_RX_SINGLE);    // set reg 0x01 to 0x06

                // Set RXDONE interrupt to dio0, RXTOUT to dio1
                writeRegister(REG_DIO_MAPPING_1, MAP_DIO0_LORA_RXDONE | MAP_DIO1_LORA_RXTOUT);

                // Accept no interrupts except RXDONE or RXTOUT
                writeRegister(REG_IRQ_FLAGS_MASK, ~(IRQ_LORA_RXDONE_MASK | IRQ_LORA_RXTOUT_MASK));

                //writeRegister(REG_IRQ_FLAGS, IRQ_LORA_CDDETD_MASK );
                writeRegister(REG_IRQ_FLAGS, 0xFF ); // reset CAD Detect interrupt flags

                delayMicroseconds( RSSI_WAIT_DOWN ); // Wait some microseconds less
                rssi = readRegister(REG_RSSI);   // Read the RSSI
                //If we have low rssi value, go scan again
                if (rssi < ( RSSI_LIMIT_DOWN )) {  // XXX RSSI might drop a little

                    if (debug >= 3) {
                        Serial.print(F("DIO1:: reset S_SCAN, rssi=")); Serial.print(rssi); Serial.print(F(", "));
                        printState(3);
                    }
                    if (_hop) { hop(); }    // XXX
                    _state = S_SCAN;
                    cadScanner();
                }
                // else we are going to read and get RXDONE of RXTOUT
                else if (debug >= 3 ) {
                    Serial.print(F("DIO1:: Start S_RX, "));
                    printState(3);
                }
            }//if
            else {
                if (debug >=3) {
                    Serial.print(F("DIO1:: ERROR Wrong flag ,"));
                    printState(3);
                }
            }
            break;

            // If DIO1 is not a CADDETECT interrupt, it is an RXTIMEOUT interrupt
            // So we restart the scanner
        case S_RX:
            if (intr & IRQ_LORA_RXDONE_MASK) {
                if (debug >= 3) { Serial.print(F("DIO1:: Warning RXDONE received, ")); printState(3); }
            }
            if (intr & IRQ_LORA_RXTOUT_MASK) {
                if (debug >= 3) {
                    Serial.println(F("DIO1:: RXTOUT, "));
                    printState(3);
                }
                // reset RX Timeout interrupt flags
                writeRegister(REG_IRQ_FLAGS_MASK, ~(IRQ_LORA_CDDETD_MASK | IRQ_LORA_RXTOUT_MASK));

                // Set the modem for next receive action. This must be done before
                // scanning takes place as we cannot do it once RXDETTD is set.
                _state=S_RX;
                rxLoraModem();

                if (_hop) { hop(); }

                if (_cad) {
                    // Set the state to CAD scanning
                    _state = S_SCAN;
                    cadScanner();        // Start the scanner after RXTOUT
                }
            }
            else if (debug >=3) {
                Serial.println(F("DIO1:: WARNING: _state==S_RX but event not IRQ_LORA_RXTOUT_MASK"));
            }
            break;

        default:
            if (debug >= 2) {
                Serial.print(F("DIO1:: ERROR Unrecognized state, "));
                printState(2);
            }
            break;
    }
}

// ----------------------------------------------------------------------------
// Frequency Hopping Channel (FHSS) dio2
// ----------------------------------------------------------------------------
//void Interrupt_2()
//{
// uint8_t fhss = readRegister(REG_HOP_CHANNEL);
// Serial.print(F("DIO2:: Calling interrupt"));
//if (_hop) {       // XXX HIGHLY experimental
//  hop();
//}
//}


// ----------------------------------------------------------------------------
// Interrupt Handler.
// Both interrupts DIO0 and DIO1 are mapped on GPIO15. Se we have to look at
// the interrupt flags to see which inteerupt(s) are called.
//
// NOTE:: This method may work not as good as just using more GPIO pins on
//  the ESP8266 mcu. But in practice it works good enough
// ----------------------------------------------------------------------------
void Interrupt()
{
    Serial.println(F("Interrupt"));
    flags = readRegister(REG_IRQ_FLAGS);
    mask = readRegister(REG_IRQ_FLAGS_MASK);
    uint8_t intr = flags & ( ~ mask );   // Only react on non masked interrupts

    // Check for dio interrupts and invoke corresponding interrupt routine
    if (intr & (IRQ_LORA_RXTOUT_MASK | IRQ_LORA_CDDETD_MASK | IRQ_LORA_FHSSCH_MASK)) { Interrupt_1(); }

    // dio0
    if (intr & (IRQ_LORA_RXDONE_MASK | IRQ_LORA_TXDONE_MASK | IRQ_LORA_CDDONE_MASK)) { Interrupt_0(); }

    // Only for Frequency Hopping dio2, not used in this code
    //if (intr & ( IRQ_LORA_FHSSCH_MASK )) { Interrupt_2(); }
}