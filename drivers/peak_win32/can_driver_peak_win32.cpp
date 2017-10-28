#if defined(WIN32) && !defined(__CYGWIN__)
#define usleep(micro) Sleep(micro%1000 ? (micro/1000) + 1 : (micro/1000))
#else
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#endif

#include <Windows.h>

#include "PCANBasic.h"
#include "cmi/CAN_driver.h"

CRITICAL_SECTION lock; 

bool disable_error = false;

// Define for rtr PCAN message
#define PCAN_INIT_TYPE_ST_RTR PCAN_MESSAGE_STANDARD | PCAN_MESSAGE_RTR

/***************************************************************************/
static int TranslateBaudeRate(const char* optarg)
{
    if(!strcmp( optarg, "1M")) return PCAN_BAUD_1M;
    if(!strcmp( optarg, "500K")) return PCAN_BAUD_500K;
    if(!strcmp( optarg, "250K")) return PCAN_BAUD_250K;
    if(!strcmp( optarg, "125K")) return PCAN_BAUD_125K;
    if(!strcmp( optarg, "100K")) return PCAN_BAUD_100K;
    if(!strcmp( optarg, "50K")) return PCAN_BAUD_50K;
    if(!strcmp( optarg, "20K")) return PCAN_BAUD_20K;
    if(!strcmp( optarg, "10K")) return PCAN_BAUD_10K;
    if(!strcmp( optarg, "5K")) return PCAN_BAUD_5K;
    if(!strcmp( optarg, "none")) return 0;
    return 0x0000;
}

static int TranslateChannel(const char* optarg)
{
    if(!strcmp( optarg, "PCAN_ISABUS1")) return PCAN_ISABUS1;
    if(!strcmp( optarg, "PCAN_ISABUS2")) return PCAN_ISABUS2;
    if(!strcmp( optarg, "PCAN_ISABUS3")) return PCAN_ISABUS3;
    if(!strcmp( optarg, "PCAN_ISABUS4")) return PCAN_ISABUS4;

    if(!strcmp( optarg, "PCAN_DNGBUS1")) return PCAN_DNGBUS1;

    if(!strcmp( optarg, "PCAN_PCIBUS1")) return PCAN_PCIBUS1;
    if(!strcmp( optarg, "PCAN_PCIBUS2")) return PCAN_PCIBUS2;
    if(!strcmp( optarg, "PCAN_PCIBUS3")) return PCAN_PCIBUS3;
    if(!strcmp( optarg, "PCAN_PCIBUS4")) return PCAN_PCIBUS4;

    if(!strcmp( optarg, "PCAN_USBBUS1")) return PCAN_USBBUS1;
    if(!strcmp( optarg, "PCAN_USBBUS2")) return PCAN_USBBUS2;
    if(!strcmp( optarg, "PCAN_USBBUS3")) return PCAN_USBBUS3;
    if(!strcmp( optarg, "PCAN_USBBUS4")) return PCAN_USBBUS4;

    if(!strcmp( optarg, "PCAN_PCCBUS1")) return PCAN_PCCBUS1;
    if(!strcmp( optarg, "PCAN_PCCBUS2")) return PCAN_PCCBUS2;

    return PCAN_NONEBUS;
}


/*static*/ CAN_Handle_t LIBAPI canOpen_driver (const char *busname, const char* baud_rate)
{
    int baudrate;
    int channel;
    TPCANStatus ret = -1;

    CAN_Handle_t handle;
    handle.fd = -1;
    handle.vp = NULL;

    CAN_Handle_t no_handle = handle;

    //Create the Event for the first board
    InitializeCriticalSectionAndSpinCount(&lock, 4000);


    EnterCriticalSection(&lock);
    baudrate = TranslateBaudeRate(baud_rate);
    channel  = TranslateChannel(busname);

    if(baudrate == 0){
        printf ("Baudrate not recognized \n");
        return no_handle;
    }
    if(channel == 0){
        printf ("Channel not recognized \n");
        return no_handle;
    }

   ret = CAN_Initialize( handle.fd, baudrate);

    if(ret == PCAN_ERROR_OK)
    {
        LeaveCriticalSection(&lock);
        return handle;
    }

    printf (">> Cannot open the CAN with these parameters. Error 0x%X \n" , ret);

    LeaveCriticalSection(&lock);
    return no_handle;
}

/********* functions which permit to communicate with the board ****************/
/*static*/ int LIBAPI canReceive_driver( CAN_Handle_t handle, CanMessage * m )
{
    static int HeavyCounter = 0;

    TPCANStatus ret = PCAN_ERROR_OK;
    TPCANMsg peak_msg;
    TPCANTimestamp peakRcvTime;

    DWORD Res;

    // loop until valid message or fatal error
    do{
        // We read the queue looking for messages.

        Res = CAN_Read( handle.fd, &peak_msg, &peakRcvTime );

        // Exit receive thread when handle is no more valid
        if(Res & PCAN_ERROR_ILLHANDLE) return -1;


        if( Res==PCAN_ERROR_INITIALIZE && disable_error)
        {
            return -1;
        }

        // A message was received : we process the message(s)
        if(Res == PCAN_ERROR_OK)
        {
            switch(peak_msg.MSGTYPE)
            {
                case PCAN_MESSAGE_STATUS:
                    switch(peak_msg.DATA[3])
                    {
                        case PCAN_ERROR_BUSHEAVY:
                            printf ("Peak board read BUSHEAVY\n");
                            break;
                        case PCAN_ERROR_BUSOFF:
                            printf ("Peak board read BUSOFF: re-init!!!\n");
                            // TODO canInit((s_BOARD*)fd0);
                            usleep(33);
                            break;
                    }
                    return -peak_msg.DATA[3];	/* if something different that 11bit or rtr... problem */

                case PCAN_MESSAGE_STANDARD:		/* bits of PCAN_MESSAGE_ */
                case PCAN_MESSAGE_EXTENDED:
                    m->rtr = 0;
                    break;

                case PCAN_MESSAGE_RTR:			/* bits of PCAN_MESSAGE_ */
                    m->rtr = 1;
                    break;


                default: return -PCAN_ERROR_OVERRUN;	/* If status, return status if 29bit, return overrun. */

            }

            m->cob_id = peak_msg.ID;
            if (peak_msg.MSGTYPE == PCAN_MODE_STANDARD)  /* bits of PCAN_MESSAGE_ */
                m->rtr = 0;
            else
                m->rtr = 1;
            m->len = peak_msg.LEN;		/* count of data bytes (0..8) */
            for (int d = 0; d < peak_msg.LEN; d++)
            {
                m->data[d] = peak_msg.DATA[d];	/* data bytes, up to 8 */
            }

        }
        else
        {
            // not benign error => fatal error
            if (!(Res & PCAN_ERROR_QRCVEMPTY
                  || Res & PCAN_ERROR_BUSLIGHT
                  || Res & PCAN_ERROR_BUSHEAVY  ) )
            {
                printf ("canReceive returned error (0x%X)\n", Res);
                return -1;
            }
        }
    } while(Res != PCAN_ERROR_OK);
    return 0;
}

/***************************************************************************/
/*static*/ int LIBAPI canSend_driver( CAN_Handle_t  handle, CanMessage const * m)
{
    TPCANMsg peak_msg;
    peak_msg.ID = m->cob_id;	/* 11/29 bit code */
    if (m->rtr == 0)
    {
        if(peak_msg.ID > 0x7FF)
            peak_msg.MSGTYPE = PCAN_MESSAGE_EXTENDED;	/* bits of PCAN_MESSAGE_ */
        else
            peak_msg.MSGTYPE = PCAN_MESSAGE_STANDARD;	/* bits of PCAN_MESSAGE_ */
    }
    else
        peak_msg.MSGTYPE = PCAN_MESSAGE_RTR;		/* bits of PCAN_MESSAGE_ */

    peak_msg.LEN = m->len;

    for (int d = 0; d < m->len; d++)
    {
        peak_msg.DATA[d] = m->data[d];	/* data bytes, up to 8 */
    }

    do{

        errno = CAN_Write (handle.fd, &peak_msg);

        if (errno)
        {
            if (errno == PCAN_ERROR_BUSOFF)
            {
                printf ("!!! Peak board write : re-init\n");
                // TODO canInit((s_BOARD*)fd0);
                usleep (10000);
            }
            usleep (1000);
        }
    }
    while (errno != PCAN_ERROR_OK);

    return 0;

}


/***************************************************************************/
/*static*/ int LIBAPI canClose_driver(CAN_Handle_t handle)
{
    disable_error = true;
    CAN_Uninitialize (handle.fd);

    return 0;
}
