#include "cmi/CAN_driver.h"
#include <fcntl.h>
#include <unistd.h>
#include <libpcan.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

using namespace CanMoveIt;

CAN_Handle_t LIBAPI canOpen_driver (const char *busname, const char* bitrate)
{
    HANDLE h = LINUX_CAN_Open(busname, O_RDWR);

    CAN_Handle_t handle;
    handle.fd = -1;
    handle.vp = NULL;

    if( h == NULL)
    {
        printf("Can't open the busname %s\n",busname);
        return handle;
    }
    handle.vp = h;

    /* request the associated file descriptor */

    handle.fd = LINUX_CAN_FileHandle(h);
    unsigned br;

    if(!strcmp( bitrate, "1M"))           br = 0x0014;
    else if(!strcmp( bitrate, "1000K"))   br = 0x0014;
    else if(!strcmp( bitrate, "500K"))    br = 0x001C;
    else if(!strcmp( bitrate, "250K"))    br = 0x011C;
    else if(!strcmp( bitrate, "125K"))    br = 0x031C;
    else if(!strcmp( bitrate, "100K"))    br = 0x432F;
    else if(!strcmp( bitrate,  "50K"))    br = 0x472F;
    else if(!strcmp( bitrate,  "20K"))    br = 0x532F;
    else if(!strcmp( bitrate,  "10K"))    br = 0x672F;
    else if(!strcmp( bitrate,   "5K"))    br = 0x7F7F;
    else{
        printf("Bitrate not recognized -%s-\n",bitrate);
        handle.fd = -1;
        handle.vp = NULL;
        return handle;
    }

    DWORD err = CAN_Init(h, br, CAN_INIT_TYPE_ST);
    if( err )
    {
        printf("Bitrate init failed. Error %d\n",err);
        handle.fd = -1;
        handle.vp = NULL;
        return handle;
    }

    printf("Can succesfully opened %s\n",busname);
    return handle;
}


int LIBAPI canReceive_driver( CAN_Handle_t handle, CanMessage * m )
{
    TPCANRdMsg frame;
    frame.Msg.ID = 0xFFFF;

  //  int ret =  LINUX_CAN_Read_Timeout(handle.vp, &frame, 100*1000);

    CAN_ResetFilter(handle.vp);
     printf("READ\n");
    int ret = CAN_Read(handle.vp,  &frame.Msg);
    printf("Ret %d/0x%X    0x%X\n",ret,ret, frame.Msg.ID);

    if( ret == CAN_ERR_QRCVEMPTY)  return 1;
    if( ret != CAN_ERR_OK)         return -ret;

    m->cob_id = frame.Msg.ID & 0x7FF;
    m->len    = frame.Msg.LEN;
    if (frame.Msg.MSGTYPE & MSGTYPE_RTR)
        m->rtr  = 1;
    else
        m->rtr  = 0;
    memcpy (m->data, frame.Msg.DATA, 8);

    return CAN_ERR_OK;
}

/***************************************************************************/
/*static*/ int LIBAPI canSend_driver( CAN_Handle_t  handle, CanMessage const * m)
{
    TPCANMsg msg;

    msg.MSGTYPE = MSGTYPE_STANDARD;
    if (m->rtr)   msg.MSGTYPE |= MSGTYPE_RTR;

    msg.LEN = m->len;
    memcpy ( msg.DATA, m->data, 8);

    msg.ID = m->cob_id;

    int err = CAN_Write(handle.vp, &msg);

    if (err != CAN_ERR_OK)
    {
        fprintf (stderr, "CAN Send failed: %d\n", err );
        return err;
    }
    return 0;
}


/***************************************************************************/
int LIBAPI canClose_driver(CAN_Handle_t handle)
{
    if( handle.fd == -1 || handle.vp == NULL) return 0;
   return CAN_Close (handle.vp);
}

/***************************************************************************/
int LIBAPI canStatus_driver(CAN_Handle_t handle)
{
    if( handle.fd == -1 || handle.vp == NULL) return 0;
    return CAN_Status(handle.vp);
}
