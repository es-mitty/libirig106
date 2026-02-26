/****************************************************************************

 i106_can.h
 Created by: Tommaso Falchi Delitala <volalto86@gmail.com>

 ****************************************************************************/

#ifndef _I106_CAN_H
#define _I106_CAN_H

#include "libirig106.h"
#include "i106_util.h"

/* Data structures */

#pragma pack(push,1)

// Channel specific data word
typedef struct CAN_CSDW CAN_CSDW;
struct CAN_CSDW {
    uint32_t    Count     : 16;      // Message count
    uint32_t    Reserved  : 16;
};

// Intra-packet header
typedef struct CAN_IPH CAN_IPH;
struct CAN_IPH {
    uint32_t    Length     : 16;     // Message length
    uint32_t    Reserved   : 16;
};

// CAN ID structure
typedef struct CAN_ID CAN_ID;
struct CAN_ID {
    uint32_t    ID         : 29;     // CAN ID
    uint32_t    RTR        :  1;     // Remote Transmission Request
    uint32_t    IDE        :  1;     // Identifier Extension
    uint32_t    Reserved   :  1;
};

// Current CAN message
typedef struct {
    uint32_t           MessageNumber;
    uint32_t           BytesRead;
    I106C10Header    * Header;
    CAN_CSDW         * CSDW;
    IntraPacketTS    * IPTS;
    CAN_IPH          * IPH;
    CAN_ID           * ID;
    uint8_t          * Data;
    TimeRef            Time;
} CAN_Message;

#pragma pack(pop)

/* Function Declaration */
I106Status I106_Decode_FirstCAN(I106C10Header *header, void *buffer, CAN_Message *msg);
I106Status I106_Decode_NextCAN(CAN_Message *msg);

#endif