case 0xFF00 :
                {
                    if (m.data[2]== 0x81) // see elmo manual (301) section 6
                    {
                        switch (m.data[3])
                        {
                            case 0x7:  printf(" Elmo:  BAD_MODE_INIT_DATA\n"); break;
                            case 0x34: printf(" Elmo:  PVT_QUEUE_FULL\n"); break;
                            case 0x5B: printf(" Elmo:  BAD_HEAD_POINTER"); break;
                            case 0x56: printf(" Elmo:  PVT_QUEUE_LOW"); break;
                            case 0x8 : printf(" Elmo:  MOTION_TERMINATED"); break;
                            default:  error_recognized = false;
                        }
                    }
                } break;
