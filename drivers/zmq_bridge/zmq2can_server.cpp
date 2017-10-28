#include "cmi/CMI.h"
#include "zmq.h"
#include "zmq2can.h"

using namespace CanMoveIt;


static CANPortPtr can_port;
static void *context  = 0;
static void *receiver = 0;
static void *sender   = 0;



void read_callback(const CanMessage & msg)
{
    unsigned char buffer[CAN_MSG_LENGTH];

    Can2Buff(msg, buffer);
    zmq_send(sender,buffer,CAN_MSG_LENGTH, 0);
}


int main (int argc, char** argv)
{
    //  Prepare our context and socket
    context = zmq_ctx_new ();
    receiver = zmq_socket (context, ZMQ_PULL);

    char connection_name[100];
    sprintf( connection_name, "tcp://*:%s", CANWRITE_PORT);

    zmq_bind (receiver, connection_name);

    sender= zmq_socket (context, ZMQ_PUB);

    sprintf( connection_name, "tcp://*:%s", CANREAD_PORT);
    zmq_bind (sender, connection_name);

    //-------------------------------------------------------------
    try{

        // Load the driver stored as a dynamic library.
        LoadCanDriver("/usr/lib/libdriver_socket.so" );

        // Open the CAN port

        if(argc==1)   { can_port = openCanPort("can0", "1M");    }
        else
            if(argc>=3)   { can_port = openCanPort( argv[1], argv[2]); }
            else{
                printf("usage: %s can0 1M\n", argv[0]);
            }

        // read_callcabck will be executed when a received CanMessage "msg" passes
        // the following test:
        //
        //  if( 0x700 == (msg.cob_id & 0xFF80 ) )
        //
        can_port->subscribeCallback(read_callback, 0, 0 );


        while(1)
        {
            unsigned char buffer[CAN_MSG_LENGTH+1];
            int S = zmq_recv( receiver, buffer, CAN_MSG_LENGTH, 0);

            CanMessage msg;
            Buff2Can( buffer, &msg);
            can_port->send( &msg );
        }
        std::cout << "End";
    }
    catch (const std::exception &e) {
        std::cerr <<  e.what() << std::endl;
        return 1;
    }
    //-------------------------------------------------------------


    zmq_close (receiver);
    zmq_ctx_destroy (context);
    return 0;
}
