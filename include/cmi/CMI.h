/*******************************************************
 * Copyright (C) 2013-2014 Davide Faconti, Icarus Technology SL Spain>
 * All Rights Reserved.
 *
 * This file is part of CAN/MoveIt Core library
 *
 * Dissemination of this information or reproduction of this material
 * is strictly forbidden unless prior written permission is obtained
 * from Icarus Technology SL Incorporated.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *******************************************************/

#ifndef MOTORHAL_H
#define MOTORHAL_H


#include <vector>
#include "cmi/MAL_types.h"
#include "cmi/MAL_Interface.h"
#include "cmi/MAL_CANOpen402.h"
#include "cmi/CO301_interface.h"

namespace CanMoveIt{


typedef std::map<uint16_t, MAL_InterfacePtr>   MotorList;


/** Get all the MAL_InterfacePtr available.*/
const MotorList&  cmi_getMotorList() ;

int cmi_loadFile(const char* filename);


/**  This function must be called once at the beginning.
  It will load an XML file where all the information related to one or
  more motors is stored.

    @param config_file Name of the XML file that will be opened.
    @return num of motors allocated according to the XML config file. -1 in case of failure.
*/
int cmi_init( char const* config_file);


/** @brief Any servo drive must be configured at least once every time that it is switched on.
    @param  motor_ptr    Pointer of the motor.
    @param  autostart    Will execute automatically cmi_startMotor.
    @param  filename     Configuration file. Not currently used, but included in the API for future upgrades.
    @return true if succesfull.
*/
bool cmi_configureMotor(MAL_InterfacePtr motor_ptr, bool autostart = true, const char *filename=NULL);


/** Wrapper to get_CO301_Interface. To be removed in future versions.
 */
inline CO301_InterfacePtr cmi_getCO301_Interface(uint16_t device_id) { return get_CO301_Interface(device_id); }


/** Get the pointer of a motor previously allocated by the function cmi_init.
 */
MAL_InterfacePtr cmi_getMAL_Interface(uint16_t motor_id);


/** Broadcast a SYNC message to all the opened CANPorts
 * */
void cmi_sendSync();



} // end namespace

#endif // MOTORHAL_H
