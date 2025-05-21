/* --COPYRIGHT--,BSD
 * Copyright (c) 2012, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --/COPYRIGHT--*/
//! \file   solutions/instaspin_foc/src/proj_lab01.c
//! \brief CPU and Inverter Set-up and introduction to interfacing to the ROM library
//!
//! (C) Copyright 2011, Texas Instruments, Inc.

//! \defgroup PROJ_LAB01 PROJ_LAB01
//@{

//! \defgroup PROJ_LAB01_OVERVIEW Project Overview
//!
//! CPU and Inverter Set-up and introduction to interfacing to the ROM library
//!

//
// **************************************************************************
// the includes

// system includes
#define CAN_ENABLE
#include <math.h>


// modules
#include "sw/modules/math/src/32b/math.h"
#include "sw/modules/memCopy/src/memCopy.h"


// drivers


// platforms
#include "main.h"
#include "sw/drivers/can/src/32b/f28x/f2806x/can.h"


// **************************************************************************
// the defines

#define LED_BLINK_FREQ_Hz   5
#define test_cmd
//#define ECAN_API

// **************************************************************************
// the globals

uint_least32_t gLEDcnt = 0;                   // Counter used to divide down the ISR rate for visually blinking an LED

// CAN

int CANIDS = 6;
int CANDLS = 8;
// **************************************************************************
// CAN


uint_least32_t  ErrorCount = 0;
uint_least32_t  PassCount = 0;
uint_least32_t  MessageReceivedCount = 0;

uint32_t MDL_t_test = 0;
uint32_t MDH_t_test = 0;

uint16_t gTXtemp = 0;
uint32_t looptemp = 0;
uint32_t looptemp2 = 0;
uint32_t code43_count = 0;
uint32_t code42_count = 0;

int ECANIDS = 6;
uint32_t ECAN_rxBuf[8] = {0,0,0,0,0,0,0,0};

ECAN_Mailbox gECAN_Mailbox;
FIFO_ID_Obj gECAN_rxFIFO_ID;
FIFO_ID_Obj gECAN_txFIFO_ID;
MSG_t msg_temp;


//

#ifdef CSM_ENABLE
#pragma DATA_SECTION(halHandle,"rom_accessed_data");
#endif
HAL_Handle halHandle;                         // Handle to the Inverter hardware abstraction layer

#ifdef CSM_ENABLE
#pragma DATA_SECTION(gUserParams,"rom_accessed_data");
#endif
USER_Params gUserParams;                      // Contains the user.h settings

HAL_PwmData_t gPwmData = {0,0,0};             // Contains PWM duty cycles in global Q format

HAL_AdcData_t gAdcData = {0,0,0,0,0,0,0};     // Contains Current and Voltage ADC readings in global Q format

volatile MOTOR_Vars_t gMotorVars = MOTOR_Vars_INIT;

#ifdef FLASH
// Used for running BackGround in flash, and ISR in RAM
extern uint16_t *RamfuncsLoadStart, *RamfuncsLoadEnd, *RamfuncsRunStart;

#ifdef CSM_ENABLE
extern uint16_t *econst_start, *econst_end, *econst_ram_load;
extern uint16_t *switch_start, *switch_end, *switch_ram_load;
#endif
#endif


#ifdef DRV8301_SPI
// Watch window interface to the 8301 SPI
DRV_SPI_8301_Vars_t gDrvSpi8301Vars;
#endif

#ifdef DRV8305_SPI
// Watch window interface to the 8305 SPI
DRV_SPI_8305_Vars_t gDrvSpi8305Vars;
#endif

// **************************************************************************
// the functions

void main(void)
{
  // Only used if running from FLASH
  // Note that the variable FLASH is defined by the project
  #ifdef FLASH
  // Copy time critical code and Flash setup code to RAM
  // The RamfuncsLoadStart, RamfuncsLoadEnd, and RamfuncsRunStart
  // symbols are created by the linker. Refer to the linker files.
  memCopy((uint16_t *)&RamfuncsLoadStart,(uint16_t *)&RamfuncsLoadEnd,(uint16_t *)&RamfuncsRunStart);

  #ifdef CSM_ENABLE
    //copy .econst to unsecure RAM
    if(*econst_end - *econst_start)
      {
        memCopy((uint16_t *)&econst_start,(uint16_t *)&econst_end,(uint16_t *)&econst_ram_load);
      }

    //copy .switch ot unsecure RAM
    if(*switch_end - *switch_start)
      {
        memCopy((uint16_t *)&switch_start,(uint16_t *)&switch_end,(uint16_t *)&switch_ram_load);
      }
  #endif
  #endif

  // initialize the hardware abstraction layer
  halHandle = HAL_init(&hal,sizeof(hal));

  // check for errors in user parameters
  USER_checkForErrors(&gUserParams);

  // store user parameter error in global variable
  gMotorVars.UserErrorCode = USER_getErrorCode(&gUserParams);

  // do not allow code execution if there is a user parameter error
  if(gMotorVars.UserErrorCode != USER_ErrorCode_NoError)
    {
      for(;;)
        {
          gMotorVars.Flag_enableSys = false;
        }
    }

  // initialize the user parameters
  USER_setParams(&gUserParams);

  // set the hardware abstraction layer parameters
  HAL_setParams(halHandle,&gUserParams);

#ifdef LAUNCHPAD
  // Setup GPIOs 0 and 1 as outputs for use in project lab1 only.
  // This is specific to the launchpad because its LEDs are also used by the PWM channels.
  HAL_setupLaunchPadGpio0and1(halHandle);
#endif

  // setup faults
  HAL_setupFaults(halHandle);


  // initialize the interrupt vector table
  HAL_initIntVectorTable(halHandle);


  // enable the ADC interrupts
  HAL_enableAdcInts(halHandle);


  // enable global interrupts
  HAL_enableGlobalInts(halHandle);


  // enable debug interrupts
  HAL_enableDebugInt(halHandle);


  // disable the PWM
  HAL_disablePwm(halHandle);


#ifdef DRV8301_SPI
  // turn on the DRV8301 if present
  HAL_enableDrv(halHandle);
  // initialize the DRV8301 interface
  HAL_setupDrvSpi(halHandle,&gDrvSpi8301Vars);
#endif

#ifdef DRV8305_SPI
  // turn on the DRV8305 if present
  HAL_enableDrv(halHandle);
  // initialize the DRV8305 interface
  HAL_setupDrvSpi(halHandle,&gDrvSpi8305Vars);
#endif

// CAN //
  //  ECAN_configAuto_Answer(halHandle->ecanaHandle, MailBox0, 8,Standard_ID, DLC_8, 0x5A5A5A5A, 0x5A5A5A5A);
  //                                                      MailBoxID                                                              Mask
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox0,  0x046, Enable_Mbox, Tx_Dir, Standard_ID, DLC_8, Overwrite_on, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox1,  0x047, Enable_Mbox, Tx_Dir, Standard_ID, DLC_8, Overwrite_on, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox2,  ECANIDS, Disable_Mbox, Tx_Dir, Standard_ID, DLC_8, Overwrite_on, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox3,  ECANIDS, Disable_Mbox, Tx_Dir, Standard_ID, DLC_8, Overwrite_on, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox4,  ECANIDS, Disable_Mbox, Tx_Dir, Standard_ID, DLC_8, Overwrite_on, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox5,  ECANIDS, Disable_Mbox, Tx_Dir, Standard_ID, DLC_8, Overwrite_on, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox6,  ECANIDS, Disable_Mbox, Tx_Dir, Standard_ID, DLC_8, Overwrite_on, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox7,  ECANIDS, Disable_Mbox, Tx_Dir, Standard_ID, DLC_8, Overwrite_on, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox8,  ECANIDS, Disable_Mbox, Tx_Dir, Standard_ID, DLC_8, Overwrite_on, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox9,  ECANIDS, Disable_Mbox, Tx_Dir, Standard_ID, DLC_8, Overwrite_on, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox10, ECANIDS, Disable_Mbox, Tx_Dir, Standard_ID, DLC_8, Overwrite_on, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox11, ECANIDS, Disable_Mbox, Tx_Dir, Standard_ID, DLC_8, Overwrite_on, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox12, ECANIDS, Disable_Mbox, Tx_Dir, Standard_ID, DLC_8, Overwrite_on, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox13, ECANIDS, Disable_Mbox, Tx_Dir, Standard_ID, DLC_8, Overwrite_on, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox14, ECANIDS, Disable_Mbox, Tx_Dir, Standard_ID, DLC_8, Overwrite_on, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox15, ECANIDS, Disable_Mbox, Tx_Dir, Standard_ID, DLC_8, Overwrite_on, LAMI_0, Mask_not_used, 0x00000000);

    ECAN_setTx_Priority(halHandle->ecanaHandle, MailBox15, Tx_leve31);

    ECAN_configMailbox(halHandle->ecanaHandle, MailBox16, 0x042, Enable_Mbox, Rx_Dir, Standard_ID, DLC_8, Overwrite_off, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox17, 0x043, Enable_Mbox, Rx_Dir, Standard_ID, DLC_8, Overwrite_off, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox18, 0x044, Enable_Mbox, Rx_Dir, Standard_ID, DLC_8, Overwrite_off, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox19, 0x045, Enable_Mbox, Rx_Dir, Standard_ID, DLC_8, Overwrite_off, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox20, ECANIDS, Disable_Mbox, Rx_Dir, Standard_ID, DLC_8, Overwrite_off, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox21, ECANIDS, Disable_Mbox, Rx_Dir, Standard_ID, DLC_8, Overwrite_off, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox22, ECANIDS, Disable_Mbox, Rx_Dir, Standard_ID, DLC_8, Overwrite_off, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox23, ECANIDS, Disable_Mbox, Rx_Dir, Standard_ID, DLC_8, Overwrite_off, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox24, ECANIDS, Disable_Mbox, Rx_Dir, Standard_ID, DLC_8, Overwrite_off, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox25, ECANIDS, Disable_Mbox, Rx_Dir, Standard_ID, DLC_8, Overwrite_off, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox26, ECANIDS, Disable_Mbox, Rx_Dir, Standard_ID, DLC_8, Overwrite_off, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox27, ECANIDS, Disable_Mbox, Rx_Dir, Standard_ID, DLC_8, Overwrite_off, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox28, ECANIDS, Disable_Mbox, Rx_Dir, Standard_ID, DLC_8, Overwrite_off, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox29, ECANIDS, Disable_Mbox, Rx_Dir, Standard_ID, DLC_8, Overwrite_off, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox30, ECANIDS, Disable_Mbox, Rx_Dir, Standard_ID, DLC_8, Overwrite_off, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox31, ECANIDS, Disable_Mbox, Rx_Dir, Standard_ID, DLC_8, Overwrite_off, LAMI_0, Mask_not_used, 0x00000000);  // 1F80
    //                                    TX_max    TX_min    RX_max     RX_min
    ECAN_initMailboxUse(&gECAN_Mailbox, MailBox15, MailBox0, MailBox31, MailBox16);

  //  ECAN_setTx_Priority(halHandle->ecanaHandle, MailBox0, Tx_leve31);
  //  ECAN_SelfTest(halHandle->ecanaHandle, Self_test_mode);
    ECAN_SelfTest(halHandle->ecanaHandle, Normal_mode);

      FIFO_FLUSH(gECAN_rxFIFO_ID);
      FIFO_FLUSH(gECAN_txFIFO_ID);

  //  ECAN_putDataMailbox_ID(halHandle->ecanaHandle, MailBox14, 0x606, DLC_2, 0xA5A5A5A5, 0xA5A5A5A5);
  //  ECAN_putDataMailbox_ID(halHandle->ecanaHandle, MailBox13, 0x706, DLC_3, 0xA5A5A5A5, 0xA5A5A5A5);
  //  ECAN_transmit_N(halHandle->ecanaHandle, (uint32_t)((1<<MailBox13) | (1<<MailBox14)));
   //ECAN_sendMsg_ID(halHandle->ecanaHandle, MailBox12, 0x206, DLC_4, 0xA5A5A5A5, 0xA5A5A5A5);
      looptemp = 1;

  #ifdef ECAN_API
     int i = 0;
     for(i = 0; i < 16; i++) {
         FIFO_PUSH_ID(&gECAN_txFIFO_ID, 0x006 + (0x100*i), DLC_0 + i, 0x01020304, 0x05060708);
         //MessageReceivedCount++;
     }

     while(!(ECAN_sendMsg_FIFO_ID_One(halHandle->ecanaHandle, &gECAN_Mailbox, &gECAN_txFIFO_ID))) MessageReceivedCount++;
     looptemp = 2;
  #endif

  // For ever loop
  while(1){
          ECAN_putDataMailbox_ID(halHandle->ecanaHandle, MailBox15, 0x046, DLC_8, 0x5A5A5A5A, 0x5B5B5B5B);
          ECAN_transmitMsg(halHandle->ecanaHandle, MailBox15);
          if(ECAN_checkMail(halHandle->ecanaHandle)) {
                  ECAN_getMsgFIFO_ID_N(halHandle->ecanaHandle, &gECAN_Mailbox, &gECAN_rxFIFO_ID);
          }


          while(!(FIFO_IS_EMPTY(gECAN_rxFIFO_ID))) {
              msg_temp = FIFO_FRONT(gECAN_rxFIFO_ID);
              FIFO_POP(gECAN_rxFIFO_ID);
              looptemp = 4;
              int i = 0;
              i = msg_temp.msgID;
              switch(i) {
                  case 0x043: gMotorVars.Flag_enableUserParams = false; code43_count++;  break;
                  case 0x042: gMotorVars.Flag_enableUserParams = true; MDH_t_test = msg_temp.dataH; MDL_t_test = msg_temp.dataL; code42_count++;    break;
  //              case (ECANIDS + (0x100*2)): gMotorVars.Flag_enableSys = true; break;
  //              case (ECANIDS + (0x100*3)): gMotorVars.Flag_enableSys = true; break;
  //              case (ECANIDS + (0x100*4)): gMotorVars.Flag_enableSys = true; break;
  //              case (ECANIDS + (0x100*5)): gMotorVars.Flag_enableSys = true; break;
  //              case (ECANIDS + (0x100*6)): gMotorVars.Flag_enableSys = true; break;
  //              case (ECANIDS + (0x100*7)): gMotorVars.Flag_enableSys = true; break;
                  default:                       gMotorVars.Flag_enableSys = true; break;
              }
          }
          looptemp++;
          FIFO_FLUSH(gECAN_rxFIFO_ID);



  }

} // end of main() function


interrupt void mainISR(void)
{

  // toggle status LED
  if(++gLEDcnt >= (uint_least32_t)(USER_ISR_FREQ_Hz / LED_BLINK_FREQ_Hz))
  {
    HAL_toggleLed(halHandle,(GPIO_Number_e)HAL_Gpio_LED2);
    gLEDcnt = 0;
  }


  // acknowledge the ADC interrupt
  HAL_acqAdcInt(halHandle,ADC_IntNumber_1);


  // convert the ADC data
  HAL_readAdcData(halHandle,&gAdcData);


  // ADC processing and pwm result code goes here


  // write the PWM compare values
  HAL_writePwmData(halHandle,&gPwmData);


  return;
} // end of mainISR() function


#ifdef ecan_test

void mailbox_read(ECAN_Handle handle, int_least16_t MBXnbr){
   volatile struct MBOX *Mailbox = (&(handle->ECanaMboxes->MBOX0)) + MBXnbr;
   TestMbox1 = Mailbox->MDL.all; // = 0x9555AAAn (n is the MBX number)
   TestMbox2 = Mailbox->MDH.all; // = 0x89ABCDEF (a constant)
   TestMbox3 = Mailbox->MSGID.all;// = 0x9555AAAn (n is the MBX number)

} // MSGID of a rcv MBX is transmitted as the MDL data.

void mailbox_check(int_least32_t T1, int_least32_t T2, int_least32_t T3){
    if((T1 != T3) || ( T2 != 0x89ABCDEF))
    {
        ErrorCount++;
    }
    else
    {
        PassCount++;
    }
}
#endif

//@} //defgroup
// end of file




