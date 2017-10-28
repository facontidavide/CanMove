#ifndef INGENIA_HH_
#define INGENIA_HH_

const char* EmergencyCodeToString( long unsigned error_code)
{
    error_code &= 0xFFFF;

    switch(error_code)
    {
        case 0x3210:
        case 0x3211: return("over-voltage-b") ;
        case 0x3220:
        case 0x3221: return("under-voltage!!!!") ;
        case 0x4300:
        case 0x4310: return("over temperature") ;

        case 0x7306: return("differential encoder broken") ;

        case 0x8110: return("CAN bus over-run") ;
        case 0x8120: return("CAN in error passive mode");

        case 0x8130: return("Lifeguard error") ;
        case 0x8140: return("Recovered from Bus off") ;
        case 0x8141: return("Bus off occurred") ;

        case 0xFF02:
        case 0xFF03: return("Not allowed digital hall combination") ;

        case 0xFF05: return("Interpolated position buffer full") ;
        case 0xFF06: return("Error in analog hall signals") ;

        default: "not recognized";
    }
}

#endif
