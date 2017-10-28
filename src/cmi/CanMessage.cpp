/*******************************************************
 * Copyright (C) 2013-2014 Davide Faconti, Icarus Technology SL Spain>
 * All Rights Reserved.
 *
 * This file is part of CAN/MoveIt Core library
 *
 * Dissemination of this information or reproduction of this material
 * is strictly forbidden unless prior written permission is obtained
 * from Icarus Technology SL Incorporated.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *******************************************************/

#include <stdio.h>
#include "cmi/CanMessage.h"

namespace CanMoveIt
{

CanMessage::CanMessage():
    raw_storage(0),
    timestamp_usec(0)
{
    data[0] = data[1] = data[2] = data[3] = 0;
    data[4] = data[5] = data[6] = data[7] = 0;
}

void CanMessage::clear()
{
    raw_storage = 0;
    timestamp_usec = 0;
    data[0] = data[1] = data[2] = data[3] = 0;
    data[4] = data[5] = data[6] = data[7] = 0;
    wait_answer = NO_WAIT;
}


void CanMessage::sprint(char* str) const
{
    int off = sprintf(str, "COB: 0x%X  node: %d [ " ,  getCOB() , (int)getNode() );

    for( int i = 0; i < len; i++ )
    {
        if( i!= 3)   off += sprintf(&str[off], "0x%X ", data[i] );
        else         off += sprintf(&str[off], "0x%X\t", data[i] );
    }
    sprintf(&str[off],  " ]\n");
}

void CanMessage::print() const
{
    char temp[100];
    this->sprint(temp);
    printf("%s", temp );
}

} //end namespace
