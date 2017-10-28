/*
This file is part of CanFestival, a library implementing CanOpen Stack.

Copyright (C): Edouard TISSERANT and Francis DUPIN

See COPYING file for copyrights details.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>		/* for NULL */
#include <errno.h>


#ifdef RTCAN_SOCKET
#include "rtdm/rtcan.h"
#define CAN_IFNAME     "rtcan%s"
#define CAN_SOCKET     rt_dev_socket
#define CAN_CLOSE      rt_dev_close
#define CAN_RECV       rt_dev_recv
#define CAN_SEND       rt_dev_send
#define CAN_BIND       rt_dev_bind
#define CAN_IOCTL      rt_dev_ioctl
#define CAN_SETSOCKOPT rt_dev_setsockopt
#define CAN_ERRNO(err) (-err)
#else
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include "linux/can.h"
#include "linux/can/raw.h"
#include "linux/can/error.h"
#include "net/if.h"
#ifndef PF_CAN
#define PF_CAN 29
#endif
#ifndef AF_CAN
#define AF_CAN PF_CAN
#endif
//#include "af_can.h"
#define CAN_IFNAME     "can%s"
#define CAN_SOCKET     socket
#define CAN_CLOSE      close
#define CAN_RECV       recv
#define CAN_SEND       send
#define CAN_BIND       bind
#define CAN_IOCTL      ioctl
#define CAN_ERRNO(err) errno
#define CAN_SETSOCKOPT setsockopt
#endif

#include "cmi/CAN_driver.h"

using namespace CanMoveIt;

bool disable_error = false;

// Define for rtr PCAN message
#define PCAN_INIT_TYPE_ST_RTR PCAN_MESSAGE_STANDARD | PCAN_MESSAGE_RTR

/***************************************************************************/

#ifdef RTCAN_SOCKET
int
TranslateBitRate (const char *optarg)
{
    int bitrate;
    int val, len;
    char *pos = NULL;

    len = strlen (optarg);
    if (!len)
        return 0;

    switch ((int) optarg[len - 1])
    {
    case 'M':
        bitrate = 1000000;
        break;
    case 'K':
        bitrate = 1000;
        break;
    default:
        bitrate = 1;
        break;
    }
    if ((sscanf (optarg, "%i", &val)) == 1)
        bitrate *= val;
    else
        bitrate = 0;;

    return bitrate;
}
#endif

void change_system_bitrate(const char *busname, const char *bitrate)
{
    char cmd[100];
    std::string act_bitrate;

    if(!strcmp( bitrate, "1M"))           act_bitrate = "1000000";
    else if(!strcmp( bitrate, "1000K"))   act_bitrate = "1000000";
    else if(!strcmp( bitrate, "500K"))    act_bitrate =  "500000";
    else if(!strcmp( bitrate, "250K"))    act_bitrate =  "250000";
    else if(!strcmp( bitrate, "125K"))    act_bitrate =  "125000";
    else if(!strcmp( bitrate, "100K"))    act_bitrate =  "100000";
    else if(!strcmp( bitrate,  "50K"))    act_bitrate =   "50000";
    else if(!strcmp( bitrate,  "20K"))    act_bitrate =   "20000";
    else if(!strcmp( bitrate,  "10K"))    act_bitrate =   "10000";
    else if(!strcmp( bitrate,   "5K"))    act_bitrate =    "5000";

    sprintf(cmd, "ifconfig %s down; "
                 "ip link set %s type can bitrate %s triple-sampling on; "
                 "ifconfig %s up", busname, busname, act_bitrate.c_str(), busname);
    printf("Change bitrate returned: %d\n", system(cmd) );
}

CAN_Handle_t LIBAPI canOpen_driver (const char *busname, const char* baud_rate)
{
    struct ifreq ifr;
    struct sockaddr_can addr;
    int err;
    int fd;

#ifdef RTCAN_SOCKET
    can_bitrate_t  *bitrate;
    can_mode_t      *mode;
#endif

    fd = CAN_SOCKET (PF_CAN, SOCK_RAW, CAN_RAW);
    if (fd < 0)
    {
        fprintf (stderr, "Socket creation failed: %s\n",
                 strerror (CAN_ERRNO (fd)));
        goto error_ret;
    }

    if (busname[0] >= '0' && busname[0] <= '9')
        snprintf (ifr.ifr_name, IFNAMSIZ, CAN_IFNAME, busname);
    else
        strncpy (ifr.ifr_name, busname, IFNAMSIZ);
    err = CAN_IOCTL (fd, SIOCGIFINDEX, &ifr);
    if (err)
    {
        fprintf (stderr, "Getting IF index for %s failed: %d / %s\n",
                 ifr.ifr_name, err, strerror (CAN_ERRNO (err)));
        goto error_close;
    }

    printf("Trying to open driver %s\n",ifr.ifr_name);

    {
        int loopback = 0;
        err = CAN_SETSOCKOPT(fd, SOL_CAN_RAW, CAN_RAW_LOOPBACK,
                             &loopback, sizeof(loopback));
        if (err) {
            fprintf(stderr, "rt_dev_setsockopt: %d / %s\n", err, strerror (CAN_ERRNO (err)));
            goto error_close;
        }
    }

#ifndef RTCAN_SOCKET /*CAN_RAW_RECV_OWN_MSGS not supported in rtsocketcan*/
    {
        change_system_bitrate(busname, baud_rate );

        int recv_own_msgs = 0; /* 0 = disabled (default), 1 = enabled */
        err = CAN_SETSOCKOPT(fd, SOL_CAN_RAW, CAN_RAW_RECV_OWN_MSGS,
                             &recv_own_msgs, sizeof(recv_own_msgs));
        if (err) {
            fprintf(stderr, "rt_dev_setsockopt: %d / %s\n", err, strerror (CAN_ERRNO (err)));
            goto error_close;
        }
    }
#endif

    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    err = CAN_BIND (fd, (struct sockaddr *) &addr, sizeof (addr));
    if (err)
    {
        fprintf (stderr, "Binding failed: %d / %s\n", err, strerror (CAN_ERRNO (err)));
        goto error_close;
    }

#ifdef RTCAN_SOCKET
    bitrate = (can_bitrate_t *) & ifr.ifr_ifru;
    *bitrate = TranslateBitRate (board->bitrate);
    if (!*bitrate)
        goto error_close;

    err = CAN_IOCTL (fd, SIOCSCANBITRATE, &ifr);
    if (err)
    {
        fprintf (stderr,
                 "Setting bitrate %d failed: %d / %s\n",
                 *bitrate, err, strerror (CAN_ERRNO (err)));
        goto error_close;
    }

    mode = (can_mode_t *) & ifr.ifr_ifru;
    *mode = CAN_MODE_START;
    err = CAN_IOCTL (fd, SIOCSCANMODE, &ifr);
    if (err)
    {
        fprintf (stderr, "Starting CAN device failed: %d / %s\n",
                 err, strerror (CAN_ERRNO (err)));
        goto error_close;
    }
#endif

    {
        //can_err_mask_t err_mask = 0x1FF;
        //  CAN_SETSOCKOPT(fd, SOL_CAN_RAW, CAN_RAW_ERR_FILTER,
        //                  &err_mask, sizeof(err_mask));

    }
    CAN_Handle_t handle;
    handle.fd = fd;
    return handle;

error_close:
    CAN_CLOSE(fd);

error_ret:
    handle.fd = -1;
    handle.vp = NULL;
    return handle;
}

static volatile bool _threadGoAway = false;


/********* functions which permit to communicate with the board ****************/
int LIBAPI canReceive_driver( CAN_Handle_t handle, CanMessage * m )
{
    int fd = handle.fd;
    int err = 0;
    struct can_frame frame;
    //----------------
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;

    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(fd, &readSet);

    int rc = select(fd+1, &readSet, NULL, NULL, &timeout);
    if (rc > 0)
    {
        if (FD_ISSET(fd, &readSet))
        {
            err = CAN_RECV(fd, &frame, sizeof (frame), 0);
        }
    }
    else{
        if (rc == 0) return 1;
        else         return -1;
    }
    //----------
    //   err = CAN_RECV (fd, &frame, sizeof (frame), 0);
    if (err < 0)
    {
        fprintf (stderr, "Recv failed: %d / %s\n", err, strerror (CAN_ERRNO (err)));
        return err;
    }

    m->cob_id = frame.can_id & CAN_EFF_MASK;
    m->len    = frame.can_dlc;
    if (frame.can_id & CAN_RTR_FLAG)
        m->rtr  = 1;
    else
        m->rtr  = 0;
    memcpy (m->data, frame.data, 8);

    return 0;
}

/***************************************************************************/
/*static*/ int LIBAPI canSend_driver( CAN_Handle_t  handle, CanMessage const * m)
{
    int fd = handle.fd;
    int err;
    struct can_frame frame;

    frame.can_id = m->cob_id;

    if (frame.can_id >= 0x800)
        frame.can_id |= CAN_EFF_FLAG;

    frame.can_dlc = m->len;

    if (m->rtr)
        frame.can_id |= CAN_RTR_FLAG;
    else
        memcpy (frame.data, m->data, 8);

    err = CAN_SEND (fd, &frame, sizeof (frame), 0);
    if (err < 0)
    {
        fprintf (stderr, "Send failed: %d / %s\n", err , strerror (CAN_ERRNO (err)));
        return err;
    }
    return 0;
}


/***************************************************************************/
int LIBAPI canClose_driver(CAN_Handle_t handle)
{
    int fd = handle.fd;
    if (fd >= 0)
    {
        _threadGoAway = true;
        shutdown(fd, SHUT_RDWR);
        CAN_CLOSE (fd);
    }
    return 0;
}

/***************************************************************************/
int LIBAPI canStatus_driver(CAN_Handle_t handle)
{
    int fd = handle.fd;
    if (fd >= 0)
    {
        int error = 0;
        socklen_t len = sizeof (error);
        int retval = getsockopt (fd, SOL_SOCKET, SO_ERROR, &error, &len );

        return retval;
    }
    return 0;
}
