/*******************************************************
 * Copyright (C) 2013-2014 Davide Faconti, Icarus Technology SL Spain>
 * All Rights Reserved.
 *
 * This file is part of CAN/MoveIt Core library
 *
 * Dissemination of this information or reproduction of this material
 * is strictly forbidden unless prior written permission is obtained
 * from Icarus Technology SL Incorporated.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *******************************************************/

#ifndef CO301_DEF_H_
#define CO301_DEF_H_

#include "cmi/CAN.h"
#include <cmi/ObjectDictionary.h>

namespace CanMoveIt{


const uint16_t  NMT = 0x0;
const uint16_t  SYNC = 0x80;
const uint16_t  EMERGENCY = 0x80;
const uint16_t  TIME_STAMP = 0x100;

const uint16_t  SDO_TX  = 0x580;
const uint16_t  SDO_RX  = 0x600;
const uint16_t  NODE_GUARD = 0x700;

enum{
    /// Response to an event standardized by the CiA Profile.
    ASYNCH_PROFILE = 255,
    /// Response to a manufacturer specific event.
    ASYNCH_MANUFACTURER = 254
};

typedef enum{
	NMT_STOP = 0x02,
	NMT_PRE_OPERATIONAL = 0x80,
	NMT_OPERATIONAL = 0x01,
	NMT_RESET_APP =0x81,
	NMT_RESET_COMM = 0x82,
	NMT_ALL_NODES = 0
}NMT_StatesCmd;

typedef enum{
    NMT_STATE_STOPPED = 0x04,
    NMT_STATE_PRE_OPERATIONAL = 0x7F,
    NMT_STATE_OPERATIONAL = 0x05,
    NMT_STATE_NOT_DEFINED = 0
}NMT_OperationalState;

typedef enum{
    PDO1_RX ,
    PDO2_RX ,
    PDO3_RX ,
    PDO4_RX ,
    PDO5_RX ,
    PDO6_RX ,
    PDO7_RX ,
    PDO8_RX ,

    PDO1_TX ,
    PDO2_TX ,
    PDO3_TX ,
    PDO4_TX,
    PDO5_TX ,
    PDO6_TX ,
    PDO7_TX ,
    PDO8_TX
}PDO_Id;


const uint16_t PDO1_RX_Comm = 0x1400;
/*const uint16_t PDO2_RX_Comm = 0x1401;
const uint16_t PDO3_RX_Comm = 0x1402;
const uint16_t PDO4_RX_Comm = 0x1403;*/

const uint16_t PDO1_RX_Map = 0x1600;
/*const uint16_t PDO2_RX_Map = 0x1601;
const uint16_t PDO3_RX_Map = 0x1602;
const uint16_t PDO4_RX_Map = 0x1603;*/

const uint16_t PDO1_TX_Comm = 0x1800;
/*const uint16_t PDO2_TX_Comm = 0x1801;
const uint16_t PDO3_TX_Comm = 0x1802;
const uint16_t PDO4_TX_Comm = 0x1803;*/

const uint16_t PDO1_TX_Map = 0x1A00;
/*const uint16_t PDO2_TX_Map = 0x1A01;
const uint16_t PDO3_TX_Map = 0x1A02;
const uint16_t PDO4_TX_Map = 0x1A03;*/

/*const uint32_t PDO_COBID_DEFAULT[16]=
{
    0x200, 0x300, 0x400, 0x500, 0x600,
    0x180, 0x280, 0x380, 0x480
};*/

enum {
	OD_SUCCESSFUL 	            =  0x00000000,
	OD_READ_NOT_ALLOWED        =  0x06010001,
	OD_WRITE_NOT_ALLOWED       =  0x06010002,
	OD_NO_SUCH_OBJECT          =  0x06020000,
	OD_NOT_MAPPABLE            =  0x06040041,
	OD_LENGTH_DATA_INVALID     =  0x06070010,
	OD_NO_SUCH_SUBINDEX 	    =  0x06090011,
	OD_VALUE_RANGE_EXCEEDED    =  0x06090030, /* Value range test result */
	OD_VALUE_TOO_LOW           =  0x06090031, /* Value range test result */
	OD_VALUE_TOO_HIGH          =  0x06090032, /* Value range test result */
	SDO_ABT_TOGGLE_NOT_ALTERNED =  0x05030000,
	SDO_ABT_TIMED_OUT           =  0x05040000,
	SDO_ABT_OUT_OF_MEMORY       =  0x05040005, /* Size data exceed SDO_MAX_LENGTH_TRANSFER */
	SDO_ABT_GENERAL_ERROR       =  0x08000000, /* Error size of SDO message */
	SDO_ABT_LOCAL_CTRL_ERROR    =  0x08000021
};


}//end namespace

#endif /* CO301_DEFAULT_H_ */
