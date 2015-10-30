/* Copyright 2015, Pablo Ridolfi
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*==================[inclusions]=============================================*/
#include "monitor.h"            /* <= own header */
#include "os.h"                 /* <= operating system header */
#include "ciaaPOSIX_stdio.h"    /* <= device handler header */
#include "ciaaPOSIX_string.h"   /* <= string header */
#include "ciaak.h"              /* <= ciaa kernel header */
#include "ciaaMulticore.h"      /* <= multicore lib header */

/*==================[macros and definitions]=================================*/

/*==================[internal data declaration]==============================*/

/*==================[internal functions declaration]=========================*/

/*==================[internal data definition]===============================*/

static int32_t fd_in;
static int32_t fd_out;
static int32_t fd_uart;

/*==================[external data definition]===============================*/

/*==================[internal functions definition]==========================*/

/*==================[external functions definition]==========================*/

int main(void)
{
   StartOS(AppMode1);
   return 0;
}

void ErrorHook(void)
{
   ciaaPOSIX_printf("ErrorHook was called\n");
   ciaaPOSIX_printf("Service: %d, P1: %d, P2: %d, P3: %d, RET: %d\n", OSErrorGetServiceId(), OSErrorGetParam1(), OSErrorGetParam2(), OSErrorGetParam3(), OSErrorGetRet());
   ShutdownOS(0);
}

TASK(InitTaskMaster)
{
   ciaak_start();

   fd_in = ciaaPOSIX_open("/dev/dio/in/0", ciaaPOSIX_O_RDWR);
   fd_out = ciaaPOSIX_open("/dev/dio/out/0", ciaaPOSIX_O_RDWR);

   fd_uart = ciaaPOSIX_open("/dev/serial/uart/1", ciaaPOSIX_O_RDWR);

   TerminateTask();
}

TASK(CommTaskMaster)
{
   uint8_t inputs;
   char str[] = "[0000]\r\n";
   int i;

   ciaaPOSIX_read(fd_in, &inputs, 1);

   for (i=0; i<4; i++) {
      str[i+1] = (inputs & (1 << i)) ? '1' : '0';
   }

   ciaaPOSIX_write(fd_uart, str, ciaaPOSIX_strlen(str));

   if ( (inputs & 0x08) == 0) {
      uint32_t * p = 0;
      *p = 0; /* hard fault test! */
   }

   TerminateTask();
}

TASK(TaskCheckSlave)
{
   EventMaskType ev;
   uint8_t outputs;

   SetRelAlarm(TimeoutSlave, 5000, 0);
   WaitEvent(evSlaveAlive | evSlaveTimeout);
   GetEvent(TaskCheckSlave, &ev);

   if (ev & evSlaveAlive) {

      CancelAlarm(TimeoutSlave);

      ciaaPOSIX_read(fd_out, &outputs, 1);
      outputs ^= 0x08;
      ciaaPOSIX_write(fd_out, &outputs, 1);

      ClearEvent(evSlaveAlive);
   }

   if (ev & evSlaveTimeout) {
      outputs = 0x15;
      ciaaPOSIX_write(fd_out, &outputs, 1);
      ShutdownOS(0);
   }

   ChainTask(TaskCheckSlave);
}

/** @} doxygen end group definition */
/** @} doxygen end group definition */
/** @} doxygen end group definition */
/*==================[end of file]============================================*/
