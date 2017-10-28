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

#ifndef CAN_DRIVERL_H_
#define CAN_DRIVERL_H_

#include "builtin_types.hpp"
#include "cmi/CanMessage.h"


using CanMoveIt::CAN_Handle_t;
using CanMoveIt::CanMessage;


#if !defined(WIN32) || defined(__CYGWIN__) || defined(__MINGW32__)

#define LIBAPI
#define DLL_CALL(funcname) funcname

#else

#ifdef DLL_EXPORTS
#define LIBAPI __declspec(dllexport) 
#else
#define LIBAPI __declspec(dllimport) 
#endif
#define DLL_CALL(funcname) LIBAPI funcname

#endif

#ifdef __cplusplus
extern "C" {
#endif


int             DLL_CALL( canReceive_driver) (CAN_Handle_t handle, CanMessage * m)			;
int             DLL_CALL( canSend_driver)    (CAN_Handle_t handle, CanMessage const * m)	;
CAN_Handle_t    DLL_CALL( canOpen_driver)    (const char *busname, const char* baud_rate)	;
CAN_Handle_t    DLL_CALL( canOpen_driver)    (const char *busname, const char* baud_rate)	;
int				DLL_CALL(canClose_driver)   (CAN_Handle_t handle);
int             DLL_CALL( canStatus_driver ) (CAN_Handle_t handle );

#ifdef __cplusplus
}
#endif

#endif
