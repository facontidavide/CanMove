#include "cmi/CAN.h"
#include "cmi/CAN_Interface.h"
#include "cmi/CAN_driver.h"
#include "cmi/ObjectDictionary.h"
//#include "../build/Ingenia_Venus.h"
#include <OS/TimeChrono.h>

using namespace CanMoveIt;

int main(int argc, char** argv)
{
    ObjectsDictionary dictionary;

    try{
        dictionary.parseEDS("../etc/ingenia_venus.eds");

        printf("------ done ------------\n");

        TimePoint t1 = GetTimeNow();

      //  for (int a=0; a<1000; a++ )
        for (int i=0; i<dictionary.size(); i++ )
        {
          //  printf(">>> %d\n",i);
            ObjectKey k(i);
            uint16_t index    = dictionary.getEntry(k).index();
            uint8_t  subindex = dictionary.getEntry(k).subindex();

            ObjectKey fk = dictionary.find (index, subindex);

           /* if( fk == k)   {
               // printf("%d: OK\n",i);
            }
            else  {
                printf("%d: Oh MY 0x%X 0x%X!!!\n",i,index, subindex);
                exit(1);
            }*/

       //     printf("[0x%4X / 0x%4X]: \n", dictionary.getEntry(k)->index(), dictionary.getEntry(k)->subindex());
        }
        TimePoint t2 = GetTimeNow();
        printf("time: %lld\n", ElapsedTime<Microseconds>(t1, t2).count() );


        std::map<uint32_t, ObjectEntry> my_map;
        for (int i=0; i<dictionary.size(); i++ )
        {
            ObjectKey k(i);
            my_map.insert( std::make_pair( dictionary.getEntry(k).id().get()  , dictionary.getEntry(k) ) ) ;
        }

        t1 = GetTimeNow();

        //  for (int a=0; a<1000; a++ )
        for (int i=0; i<dictionary.size(); i++ )
        {
            ObjectKey k(i);
            uint16_t index    = dictionary.getEntry(k).index();
            uint8_t  subindex = dictionary.getEntry(k).subindex();

            my_map.find( ObjectID(index,subindex).get() ) ;
        }
        t2 = GetTimeNow();
        printf("time: %lld\n", ElapsedTime<Microseconds>(t1, t2).count() );
      //  dictionary.generateCode("Ingenia_Venus");
    }
    catch( std::exception e)
    {
        printf("%s\n", e.what());
    }

    return 0;
}
