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

#ifndef LORA_SX1276_H_
#define LORA_SX1276_H_

#include "lora_gateway_config.h"

#define SPIFREQ  10000000
#define SPISPEED 10000000      // was 50000/50KHz < 10MHz

// Definition of the GPIO pins used by the Gateway
struct pins {
    uint8_t dio0=D0; // For the Hallard board shared betweenDio0/DIO1/DIO2
    uint8_t dio1=D1; // Used for CAD, may or not be shared with DIO0
    uint8_t dio2=D2; // Used for frequency hopping, don't care
    uint8_t ss  =A4; // Select pin connected to SPIO16
    uint8_t rst =D8; // Reset pin not used
    uint8_t rxtx=D6; // Reset pin not used
};

// Set the structure for spreading factor
enum sf_t { SF6=6, SF7, SF8, SF9, SF10, SF11, SF12 };

// The state of the receiver. See Semtech Datasheet (rev 4, March 2015) page 43
// The _state is of the enum type (and should be cast when used as a number)
enum state_t { S_INIT=0, S_SCAN, S_CAD, S_RX, S_RXDONE, S_TX };

// Given the correction factor of 157, we can get to -118dB with this rating
// XXX 40 works
#define RSSI_LIMIT                  39
#define RSSI_LIMIT_DOWN             34      // Was 34
// How long to wait in LoRa mode before using the RSSSI value.
// XXX 275 works well in old CAD mode
#define RSSI_WAIT                   250
#define RSSI_WAIT_DOWN              225

// ============================================================================
// Set all definitions for Gateway
// ============================================================================
// Register definitions. These are the addresses of the TFM95, SX1276 that we
// need to set in the program.

#define REG_FIFO                    0x00
#define REG_OPMODE                  0x01
// Register 2 to 5 are unused for LoRa
#define REG_FRF_MSB                 0x06
#define REG_FRF_MID                 0x07
#define REG_FRF_LSB                 0x08
#define REG_PAC                     0x09
#define REG_PARAMP                  0x0A
#define REG_LNA                     0x0C
#define REG_FIFO_ADDR_PTR           0x0D
#define REG_FIFO_TX_BASE_AD         0x0E
#define REG_FIFO_RX_BASE_AD         0x0F

#define REG_FIFO_RX_CURRENT_ADDR    0x10
#define REG_IRQ_FLAGS_MASK          0x11
#define REG_IRQ_FLAGS               0x12
#define REG_RX_NB_BYTES             0x13
#define REG_PKT_SNR_VALUE           0x19
#define REG_PKT_RSSI                0x1A
#define REG_RSSI                    0x1B    // See section 5.5.5
#define REG_HOP_CHANNEL             0x1C
#define REG_MODEM_CONFIG1           0x1D
#define REG_MODEM_CONFIG2           0x1E
#define REG_SYMB_TIMEOUT_LSB        0x1F

#define REG_PAYLOAD_LENGTH          0x22
#define REG_MAX_PAYLOAD_LENGTH      0x23
#define REG_HOP_PERIOD              0x24
#define REG_MODEM_CONFIG3           0x26

#define REG_INVERTIQ                0x33
#define REG_DET_TRESH               0x37    // SF6
#define REG_SYNC_WORD               0x39

#define REG_DIO_MAPPING_1           0x40
#define REG_DIO_MAPPING_2           0x41
#define REG_VERSION                 0x42

#define REG_PADAC                   0x5A
#define REG_PADAC_SX1272            0x5A
#define REG_PADAC_SX1276            0x4D

// ----------------------------------------
// Used by REG_PAYLOAD_LENGTH to set receive patyload length
#define PAYLOAD_LENGTH              0x40

// ----------------------------------------
// opModes
#define SX72_MODE_SLEEP             0x80
#define SX72_MODE_STANDBY           0x81
#define SX72_MODE_FSTX              0x82
#define SX72_MODE_TX                0x83 // 0x80 | 0x03
#define SX72_MODE_RX_CONTINUOS      0x85

// ----------------------------------------
// LMIC Constants for radio registers
#define OPMODE_LORA                 0x80
#define OPMODE_MASK                 0x07
#define OPMODE_SLEEP                0x00
#define OPMODE_STANDBY              0x01
#define OPMODE_FSTX                 0x02
#define OPMODE_TX                   0x03
#define OPMODE_FSRX                 0x04
#define OPMODE_RX                   0x05
#define OPMODE_RX_SINGLE            0x06
#define OPMODE_CAD                  0x07

// ----------------------------------------
// LOW NOISE AMPLIFIER
#define LNA_MAX_GAIN                0x23
#define LNA_OFF_GAIN                0x00
#define LNA_LOW_GAIN                0x20

// CONF REG
#define REG1                        0x0A
#define REG2                        0x84

// ----------------------------------------
// MC1 sx1276 RegModemConfig1
#define SX1276_MC1_BW_125           0x70
#define SX1276_MC1_BW_250           0x80
#define SX1276_MC1_BW_500           0x90
#define SX1276_MC1_CR_4_5           0x02
#define SX1276_MC1_CR_4_6           0x04
#define SX1276_MC1_CR_4_7           0x06
#define SX1276_MC1_CR_4_8           0x08
#define SX1276_MC1_IMPLICIT_HEADER_MODE_ON  0x01

#define SX72_MC1_LOW_DATA_RATE_OPTIMIZE     0x01  // mandated for SF11 and SF12

// ----------------------------------------
// MC2 definitions
#define SX72_MC2_FSK                0x00
#define SX72_MC2_SF7                0x70  // SF7 == 0x07, so (SF7<<4) == SX7_MC2_SF7
#define SX72_MC2_SF8                0x80
#define SX72_MC2_SF9                0x90
#define SX72_MC2_SF10               0xA0
#define SX72_MC2_SF11               0xB0
#define SX72_MC2_SF12               0xC0

// ----------------------------------------
// MC3
#define SX1276_MC3_LOW_DATA_RATE_OPTIMIZE  0x08
#define SX1276_MC3_AGCAUTO                 0x04

// ----------------------------------------
// FRF
#define FRF_MSB      0xD9  // 868.1 Mhz
#define FRF_MID      0x06
#define FRF_LSB      0x66

// ----------------------------------------
// DIO function mappings                  D0D1D2D3
#define MAP_DIO0_LORA_RXDONE     0x00  // 00------ bit 7 and 6
#define MAP_DIO0_LORA_TXDONE     0x40  // 01------
#define MAP_DIO0_LORA_CADDONE    0x80  // 10------
#define MAP_DIO0_LORA_NOP        0xC0  // 11------

#define MAP_DIO1_LORA_RXTOUT     0x00  // --00---- bit 5 and 4
#define MAP_DIO1_LORA_FCC        0x10  // --01----
#define MAP_DIO1_LORA_CADDETECT  0x20  // --10----
#define MAP_DIO1_LORA_NOP        0x30  // --11----

#define MAP_DIO2_LORA_FCC0       0x00  // ----00-- bit 3 and 2
#define MAP_DIO2_LORA_FCC1       0x04  // ----01-- bit 3 and 2
#define MAP_DIO2_LORA_FCC2       0x08  // ----10-- bit 3 and 2
#define MAP_DIO2_LORA_NOP        0x0C  // ----11-- bit 3 and 2

#define MAP_DIO3_LORA_CADDONE    0x00  // ------00 bit 1 and 0
#define MAP_DIO3_LORA_NOP        0x03  // ------11

#define MAP_DIO0_FSK_READY       0x00  // 00------ (packet sent / payload ready)
#define MAP_DIO1_FSK_NOP         0x30  // --11----
#define MAP_DIO2_FSK_TXNOP       0x04  // ----01--
#define MAP_DIO2_FSK_TIMEOUT     0x08  // ----10--

// ----------------------------------------
// Bits masking the corresponding IRQs from the radio
#define IRQ_LORA_RXTOUT_MASK     0x80
#define IRQ_LORA_RXDONE_MASK     0x40
#define IRQ_LORA_CRCERR_MASK     0x20
#define IRQ_LORA_HEADER_MASK     0x10
#define IRQ_LORA_TXDONE_MASK     0x08
#define IRQ_LORA_CDDONE_MASK     0x04
#define IRQ_LORA_FHSSCH_MASK     0x02
#define IRQ_LORA_CDDETD_MASK     0x01

// ----------------------------------------
// Definitions for UDP message arriving from server
#define PROTOCOL_VERSION         0x01
#define PKT_PUSH_DATA            0x00
#define PKT_PUSH_ACK             0x01
#define PKT_PULL_DATA            0x02
#define PKT_PULL_RESP            0x03
#define PKT_PULL_ACK             0x04


extern int debug;                // Debug level! 0 is no msgs, 1 normal, 2 extensive
extern struct pins pins;
extern sf_t sf;
extern sf_t sfi;    // Initial value of SF
extern uint32_t  freq;
extern uint8_t  ifreq;        // Channel Index
extern state_t _state;

extern bool _cad;     // Set to true for Channel Activity Detection, only when dio1 connected
extern bool _hop;           // experimental; frequency hopping. Only use when dio2 connected
extern bool inHop;
extern unsigned long hopTime;


int sendPacket(uint8_t *buff_down, uint8_t length);
void initLoraModem();
void rxLoraModem();
void cadScanner();
void Interrupt_0();
void Interrupt_1();
void Interrupt();
void eventHandler();
void hop();

#endif
