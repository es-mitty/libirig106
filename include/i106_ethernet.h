/****************************************************************************

 i106_ethernet.h

 ****************************************************************************/

#ifndef _I106_ETHERNET_H
#define _I106_ETHERNET_H

#include "libirig106.h"

/* Data structures */

#pragma pack(push,1)

// Channel specific data word
typedef struct EthernetF0_CSDW EthernetF0_CSDW;
struct EthernetF0_CSDW {
    uint32_t    Frames    : 16;      // Number of frames
    uint32_t    Reserved  : 16;
};

// Intra-packet header
typedef struct EthernetF0_IPH EthernetF0_IPH;
struct EthernetF0_IPH {
    uint32_t    Length     : 16;     // Frame length
    uint32_t    Reserved   : 16;
};

// Current Ethernet message
typedef struct {
    uint32_t             FrameNumber;
    uint32_t             Length;
    EthernetF0_CSDW    * CSDW;
    EthernetF0_IPH     * IPH;
    uint8_t            * Data;
} EthernetF0_Message;

#pragma pack(pop)

/* Function Declaration */
I106Status I106_Decode_FirstEthernetF0(I106C10Header *header, void *buffer, EthernetF0_Message *msg);
I106Status I106_Decode_NextEthernetF0(EthernetF0_Message *msg);

#endif