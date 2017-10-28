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
#include <zmq.h>
#include "cmi/CAN_driver.h"
#include "zmq2can.h"

using namespace CanMoveIt;

typedef struct {
    void *context;
    void *receiver;
    void *sender;
}zmq_handle;




CAN_Handle_t LIBAPI canOpen_driver (const char *busname, const char* )
{
    zmq_handle* zmq = new zmq_handle;
    zmq->context  = 0;
    zmq->receiver = 0;
    zmq->sender   = 0;

    CAN_Handle_t handle;
    handle.fd = -1;
    handle.vp = 0;

    zmq->context = zmq_ctx_new ();
    zmq->receiver = zmq_socket (zmq->context, ZMQ_SUB);

    char connection_name[100];
    //----------------------
    sprintf( connection_name, "tcp://%s:%s", busname, CANREAD_PORT);

    if( zmq_connect (zmq->receiver, connection_name) == -1)
    {
        printf("canOpen_driver: error connecting to %s\n", connection_name);
        return handle;
    }

    zmq_setsockopt(zmq->receiver,ZMQ_SUBSCRIBE, NULL ,0);

    int timeout = 1000;
    zmq_setsockopt(zmq->receiver,ZMQ_RCVTIMEO, &timeout , sizeof( timeout) );
    //----------------------

    sprintf( connection_name, "tcp://%s:%s", busname, CANWRITE_PORT);
    zmq->sender= zmq_socket (zmq->context, ZMQ_PUSH);

    if( zmq_connect (zmq->sender, connection_name) == -1)
    {
        printf("canOpen_driver: error connecting to %s\n", connection_name);
        return handle;
    }
    //----------------------

    unsigned char buffer[CAN_MSG_LENGTH];
   // Can2Buff( *m, buffer );
    printf(" will send empty buffer to %s\n", connection_name);
    zmq_send( zmq->sender, buffer, CAN_MSG_LENGTH, 0);

    handle.fd = 0;
    handle.vp = (void*)zmq;
    return handle;
}


int LIBAPI canReceive_driver( CAN_Handle_t handle, CanMessage * m )
{
    zmq_handle* zmq = static_cast<zmq_handle*>( handle.vp );

    unsigned char buffer[CAN_MSG_LENGTH+1];
    int ret = zmq_recv( zmq->receiver, buffer,CAN_MSG_LENGTH, 0 );

    if( ret == -1)
    {
        if( zmq_errno() == EAGAIN)
        {
            //timeout
            return 1;
        }
        else{
            return -1;
        }
    }

    Buff2Can( buffer, m);
    return 0; // ok
}

/***************************************************************************/
int LIBAPI canSend_driver( CAN_Handle_t  handle, CanMessage const * m)
{
    zmq_handle* zmq = static_cast<zmq_handle*>( handle.vp );

    unsigned char buffer[CAN_MSG_LENGTH];
    Can2Buff( *m, buffer );

    int ret = zmq_send( zmq->sender, buffer, CAN_MSG_LENGTH, 0);
    if (ret == -1)
    {
        return zmq_errno();
    }
    else { return 0; }
}


/***************************************************************************/
int LIBAPI canClose_driver(CAN_Handle_t handle)
{

    return 0;
}

/***************************************************************************/
int LIBAPI canStatus_driver(CAN_Handle_t handle)
{

    return 0;
}
