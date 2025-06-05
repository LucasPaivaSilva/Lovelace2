

//! Lovelace Main Firmware
//!
//! InstaSpin Feature Rich Example without Controller Module
//!

// **************************************************************************
// the includes

// system includes
#include <math.h>
#include "main.h"
#include "Include/can.h"

#ifdef FLASH
#pragma CODE_SECTION(mainISR,"ramfuncs");
#pragma CODE_SECTION(runSetTrigger,"ramfuncs");
#pragma CODE_SECTION(runFieldWeakening,"ramfuncs");
#pragma CODE_SECTION(runCurrentReconstruction,"ramfuncs");
#pragma CODE_SECTION(angleDelayComp,"ramfuncs");
#endif

// Include header files used in the main function

// **************************************************************************
// the defines

#define ENABLE_CAN_SEND

// **************************************************************************
// the globals

CLARKE_Handle   clarkeHandle_I;               //!< the handle for the current Clarke transform
CLARKE_Obj      clarke_I;                     //!< the current Clarke transform object

CLARKE_Handle   clarkeHandle_V;               //!< the handle for the voltage Clarke transform
CLARKE_Obj      clarke_V;                     //!< the voltage Clarke transform object

CPU_USAGE_Handle  cpu_usageHandle;
CPU_USAGE_Obj     cpu_usage;

EST_Handle      estHandle;                    //!< the handle for the estimator

FW_Handle       fwHandle;
FW_Obj          fw;

PID_Obj         pid[3];                       //!< three handles for PID controllers 0 - Speed, 1 - Id, 2 - Iq
PID_Handle      pidHandle[3];                 //!< three objects for PID controllers 0 - Speed, 1 - Id, 2 - Iq
uint16_t        pidCntSpeed;                  //!< count variable to decimate the execution of the speed PID controller

IPARK_Handle    iparkHandle;                  //!< the handle for the inverse Park transform
IPARK_Obj       ipark;                        //!< the inverse Park transform object

FILTER_FO_Handle  filterHandle[6];            //!< the handles for the 3-current and 3-voltage filters for offset calculation
FILTER_FO_Obj     filter[6];                  //!< the 3-current and 3-voltage filters for offset calculation

SVGENCURRENT_Obj     svgencurrent;
SVGENCURRENT_Handle  svgencurrentHandle;

SVGEN_Handle    svgenHandle;                  //!< the handle for the space vector generator
SVGEN_Obj       svgen;                        //!< the space vector generator object

TRAJ_Handle     trajHandle_Id;                //!< the handle for the id reference trajectory
TRAJ_Obj        traj_Id;                      //!< the id reference trajectory object

TRAJ_Handle     trajHandle_spd;               //!< the handle for the speed reference trajectory
TRAJ_Obj        traj_spd;                     //!< the speed reference trajectory object

#ifdef CSM_ENABLE
#pragma DATA_SECTION(halHandle,"rom_accessed_data");
#endif
HAL_Handle      halHandle;                    //!< the handle for the hardware abstraction layer (HAL)

HAL_PwmData_t   gPwmData = {_IQ(0.0), _IQ(0.0), _IQ(0.0)};       //!< contains the three pwm values -1.0 - 0%, 1.0 - 100%

HAL_AdcData_t   gAdcData;                     //!< contains three current values, three voltage values and one DC buss value

MATH_vec3       gOffsets_I_pu = {_IQ(0.0), _IQ(0.0), _IQ(0.0)};  //!< contains the offsets for the current feedback

MATH_vec3       gOffsets_V_pu = {_IQ(0.0), _IQ(0.0), _IQ(0.0)};  //!< contains the offsets for the voltage feedback

MATH_vec2       gIdq_ref_pu = {_IQ(0.0), _IQ(0.0)};              //!< contains the Id and Iq references

MATH_vec2       gVdq_out_pu = {_IQ(0.0), _IQ(0.0)};              //!< contains the output Vd and Vq from the current controllers

MATH_vec2       gIdq_pu = {_IQ(0.0), _IQ(0.0)};                  //!< contains the Id and Iq measured values

#ifdef CSM_ENABLE
#pragma DATA_SECTION(gUserParams,"rom_accessed_data");
#endif
USER_Params     gUserParams;

volatile MOTOR_Vars_t gMotorVars = MOTOR_Vars_INIT;   //!< the global motor variables that are defined in main.h and used for display in the debugger's watch window

#ifdef FLASH
// Used for running BackGround in flash, and ISR in RAM
extern uint16_t *RamfuncsLoadStart, *RamfuncsLoadEnd, *RamfuncsRunStart;

#ifdef CSM_ENABLE
extern uint16_t *econst_start, *econst_end, *econst_ram_load;
extern uint16_t *switch_start, *switch_end, *switch_ram_load;
#endif
#endif


MATH_vec3 gIavg = {_IQ(0.0), _IQ(0.0), _IQ(0.0)};
uint16_t gIavg_shift = 1;
MATH_vec3       gPwmData_prev = {_IQ(0.0), _IQ(0.0), _IQ(0.0)};

#ifdef DRV8301_SPI
// Watch window interface to the 8301 SPI
DRV_SPI_8301_Vars_t gDrvSpi8301Vars;
#endif

#ifdef DRV8305_SPI
// Watch window interface to the 8305 SPI
DRV_SPI_8305_Vars_t gDrvSpi8305Vars;
#endif

#define LED_BLINK_FREQ_Hz   5
uint16_t gLEDcnt = 0;

_iq gFlux_pu_to_Wb_sf;

_iq gFlux_pu_to_VpHz_sf;

_iq gTorque_Ls_Id_Iq_pu_to_Nm_sf;

_iq gTorque_Flux_Iq_pu_to_Nm_sf;

_iq gSpeed_krpm_to_pu_sf = _IQ((float_t)USER_MOTOR_NUM_POLE_PAIRS * 1000.0 / (USER_IQ_FULL_SCALE_FREQ_Hz * 60.0));

_iq gSpeed_hz_to_krpm_sf = _IQ(60.0 / (float_t)USER_MOTOR_NUM_POLE_PAIRS / 1000.0);

_iq gIs_Max_squared_pu = _IQ((USER_MOTOR_MAX_CURRENT * USER_MOTOR_MAX_CURRENT) / (USER_IQ_FULL_SCALE_CURRENT_A * USER_IQ_FULL_SCALE_CURRENT_A));

float_t gCpuUsagePercentageMin = 0.0;
float_t gCpuUsagePercentageAvg = 0.0;
float_t gCpuUsagePercentageMax = 0.0;

uint32_t gOffsetCalcCount = 0;

uint16_t gTrjCnt = 0;

volatile bool gFlag_enableRsOnLine = false;

volatile bool gFlag_updateRs = false;

volatile _iq gRsOnLineFreq_Hz = _IQ(0.2);

volatile _iq gRsOnLineId_mag_A = _IQ(0.5);

volatile _iq gRsOnLinePole_Hz = _IQ(0.2);

// Temperature Variables
#define NUM_TEMP_SAMPLES 15  // Define the number of samples for the moving average, this shit needs: (NUM_TEMP_SCALE * 4095 < 2^16 (sumMotorTempData)) (dont ask)
#define ADC_SCALE_FACTOR_TEMP (3.3 / 4095.0)

// For motor temp
#define MOTOR_TEMP_COEF_A (-87.20222215)
#define MOTOR_TEMP_COEF_B (396.456962)
#define MOTOR_TEMP_COEF_C (-729.2932071)
#define MOTOR_TEMP_COEF_D (503.3563239)
uint16_t motorTempDataSamples[NUM_TEMP_SAMPLES];
uint8_t motorTempSampleIndex = 0;
uint16_t sumMotorTempData = 0;
bool isMotorTempBufferFull = false;
float_t motorTemp = 0;
// For IGBT temp
#define IGBT_TEMP_COEF_A (78.2527304)
#define IGBT_TEMP_COEF_B (-208.8104935)
#define IGBT_TEMP_COEF_C (225.043112)
#define IGBT_TEMP_COEF_D (-8.21890275)
#define igbtTempOffset (4.0);
uint16_t igbtTempDataSamples[NUM_TEMP_SAMPLES];
uint8_t igbtTempSampleIndex = 0;
uint16_t sumIgbtTempData = 0;
bool isIGBTTempBufferFull = false;
float_t igbtTemp = 0;

// SpeedMode
volatile bool speedMode = false;


// Safety Systems
//float iqLimitTest = 1.0;
#define OVERCURRENT_THRESHOLD 320.0
#define HIGH_OVERCURRENT_THRESHOLD 450.0

#define MAX_OVERCURRENT_COUNT 300
#define IMMEDIATE_OVERCURRENT_COUNT 10


float_t CurrentPhaseA = 0;
float_t CurrentPhaseB = 0;
float_t CurrentPhaseC = 0;
uint16_t overCurrentCountA = 0;
uint16_t overCurrentCountB = 0;
uint16_t overCurrentCountC = 0;
uint16_t immediateOverCurrentCountA = 0;
uint16_t immediateOverCurrentCountB = 0;
uint16_t immediateOverCurrentCountC = 0;

bool errorFlag_GlobalError = 0;
bool errorFlag_MotorOverTemperature = 0;
bool errorFlag_IGBTOverTemperature = 0;
bool errorFlag_ReverseSpeed = 0;
bool errorFlag_OCD = 0;
bool errorFlag_HOCD = 0;
bool errorFlag_LowDCBus = 0;

// System control
bool flagDCBusPowered = 0;
_iq iqTorqueRequest = _IQ(0.0);
uint8_t torqueMod = 4;

// Tasks Decimation
uint16_t gCANRXcnt = 0;
bool gCANRX_Flag = 0;

uint16_t gCANTX1cnt = 1;
bool gCANTX1_Flag = 0;

uint16_t gCANTX2cnt = 2;
bool gCANTX2_Flag = 0;

uint16_t gCANTX3cnt = 3;
bool gCANTX3_Flag = 0;

uint16_t gTEMPcnt = 4;
bool gTEMP_Flag = 0;

#define CAN_TASK_FREQ_Hz   20
#define TEMP_TASK_FREQ_Hz   10

// CAN variables and defines

uint32_t ECAN_rxBuf[8] = {0,0,0,0,0,0,0,0};

ECAN_Mailbox gECAN_Mailbox;
FIFO_ID_Obj gECAN_rxFIFO_ID;
FIFO_ID_Obj gECAN_txFIFO_ID;
MSG_t msg_temp;

// CAN Variables
uint16_t accel_value = 0;
uint16_t brake_value = 0;
uint8_t regen_brake_flag = 0;
uint8_t rtd_flag = 0;
uint8_t event_mode = 0;
uint8_t torqueRequestDisable = 0;

uint16_t DCL_value = 0;
uint16_t CCL_value = 0;
uint16_t battery_pack_value = 0;

float_t canIdcLink = 0;
float_t canVdcLink = 0;
float_t canInverterOutputPower = 0;
float_t canMotorOutputPower = 0;

float_t canMotorTorque = 0;
float_t canMotorSpeedRPM = 0;

uint8_t canError = 0;

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

  // initialize the Clarke modules
  clarkeHandle_I = CLARKE_init(&clarke_I,sizeof(clarke_I));
  clarkeHandle_V = CLARKE_init(&clarke_V,sizeof(clarke_V));

  // initialize the estimator
  estHandle = EST_init((void *)USER_EST_HANDLE_ADDRESS, 0x200);

  // initialize the user parameters
  USER_setParams(&gUserParams);

  // set the hardware abstraction layer parameters
  HAL_setParams(halHandle,&gUserParams);

#ifdef FAST_ROM_V1p6
  {
    CTRL_Handle ctrlHandle = CTRL_init((void *)USER_CTRL_HANDLE_ADDRESS, 0x200);
    CTRL_Obj *obj = (CTRL_Obj *)ctrlHandle;
    obj->estHandle = estHandle;

    // initialize the estimator through the controller
    CTRL_setParams(ctrlHandle,&gUserParams);
    CTRL_setUserMotorParams(ctrlHandle);
    CTRL_setupEstIdleState(ctrlHandle);
  }
#else
  {
    // initialize the estimator
    EST_setEstParams(estHandle,&gUserParams);
    EST_setupEstIdleState(estHandle);
  }
#endif

  // disable Rs recalculation by default
  gMotorVars.Flag_enableRsRecalc = false;
  EST_setFlag_enableRsRecalc(estHandle,false);

  // configure RsOnLine
  EST_setFlag_enableRsOnLine(estHandle,gFlag_enableRsOnLine);
  EST_setFlag_updateRs(estHandle,gFlag_updateRs);
  EST_setRsOnLineAngleDelta_pu(estHandle,_IQmpy(gRsOnLineFreq_Hz, _IQ(1.0/USER_ISR_FREQ_Hz)));
  EST_setRsOnLineId_mag_pu(estHandle,_IQmpy(gRsOnLineId_mag_A, _IQ(1.0/USER_IQ_FULL_SCALE_CURRENT_A)));

  // Calculate coefficients for all filters
  {
    _iq b0 = _IQmpy(gRsOnLinePole_Hz, _IQ(1.0/USER_ISR_FREQ_Hz));
    _iq a1 = b0 - _IQ(1.0);
    EST_setRsOnLineFilterParams(estHandle,EST_RsOnLineFilterType_Current,b0,a1,_IQ(0.0),b0,a1,_IQ(0.0));
    EST_setRsOnLineFilterParams(estHandle,EST_RsOnLineFilterType_Voltage,b0,a1,_IQ(0.0),b0,a1,_IQ(0.0));
  }

  // set the number of current sensors
  setupClarke_I(clarkeHandle_I,USER_NUM_CURRENT_SENSORS);

  // set the number of voltage sensors
  setupClarke_V(clarkeHandle_V,USER_NUM_VOLTAGE_SENSORS);

  // set the pre-determined current and voltage feeback offset values
  gOffsets_I_pu.value[0] = _IQ(I_A_offset);
  gOffsets_I_pu.value[1] = _IQ(I_B_offset);
  gOffsets_I_pu.value[2] = _IQ(I_C_offset);
  gOffsets_V_pu.value[0] = _IQ(V_A_offset);
  gOffsets_V_pu.value[1] = _IQ(V_B_offset);
  gOffsets_V_pu.value[2] = _IQ(V_C_offset);

  // initialize the PID controllers
  {
    _iq maxCurrent_pu = _IQ(USER_MOTOR_MAX_CURRENT / USER_IQ_FULL_SCALE_CURRENT_A);
    _iq maxVoltage_pu = _IQ(USER_MAX_VS_MAG_PU * USER_VD_SF);
    float_t fullScaleCurrent = USER_IQ_FULL_SCALE_CURRENT_A;
    float_t fullScaleVoltage = USER_IQ_FULL_SCALE_VOLTAGE_V;
    float_t IsrPeriod_sec = 1.0 / USER_ISR_FREQ_Hz;
    float_t Ls_d = USER_MOTOR_Ls_d;
    float_t Ls_q = USER_MOTOR_Ls_q;
    float_t Rs = USER_MOTOR_Rs;
    float_t RoverLs_d = Rs/Ls_d;
    float_t RoverLs_q = Rs/Ls_q;
    _iq Kp_Id = _IQ((0.25*Ls_d*fullScaleCurrent)/(IsrPeriod_sec*fullScaleVoltage));
    _iq Ki_Id = _IQ(RoverLs_d*IsrPeriod_sec);
    _iq Kp_Iq = _IQ((0.25*Ls_q*fullScaleCurrent)/(IsrPeriod_sec*fullScaleVoltage));
    _iq Ki_Iq = _IQ(RoverLs_q*IsrPeriod_sec);

    pidHandle[0] = PID_init(&pid[0],sizeof(pid[0]));
    pidHandle[1] = PID_init(&pid[1],sizeof(pid[1]));
    pidHandle[2] = PID_init(&pid[2],sizeof(pid[2]));

    PID_setGains(pidHandle[0],_IQ(1.0),_IQ(0.01),_IQ(0.0));
    PID_setMinMax(pidHandle[0],-maxCurrent_pu,maxCurrent_pu);
    PID_setUi(pidHandle[0],_IQ(0.0));
    pidCntSpeed = 0;

    PID_setGains(pidHandle[1],Kp_Id,Ki_Id,_IQ(0.0));
    PID_setMinMax(pidHandle[1],-maxVoltage_pu,maxVoltage_pu);
    PID_setUi(pidHandle[1],_IQ(0.0));

    PID_setGains(pidHandle[2],Kp_Iq,Ki_Iq,_IQ(0.0));
    PID_setMinMax(pidHandle[2],_IQ(0.0),_IQ(0.0));
    PID_setUi(pidHandle[2],_IQ(0.0));
  }

  // initialize the speed reference in kilo RPM where base speed is USER_IQ_FULL_SCALE_FREQ_Hz
  gMotorVars.SpeedRef_krpm = _IQmpy(_IQ(10.0), gSpeed_hz_to_krpm_sf);

  // initialize the inverse Park module
  iparkHandle = IPARK_init(&ipark,sizeof(ipark));

  // initialize and configure offsets using filters
  {
    uint16_t cnt = 0;
    _iq b0 = _IQ(gUserParams.offsetPole_rps/(float_t)gUserParams.ctrlFreq_Hz);
    _iq a1 = (b0 - _IQ(1.0));
    _iq b1 = _IQ(0.0);

    for(cnt=0;cnt<6;cnt++)
      {
        filterHandle[cnt] = FILTER_FO_init(&filter[cnt],sizeof(filter[0]));
        FILTER_FO_setDenCoeffs(filterHandle[cnt],a1);
        FILTER_FO_setNumCoeffs(filterHandle[cnt],b0,b1);
        FILTER_FO_setInitialConditions(filterHandle[cnt],_IQ(0.0),_IQ(0.0));
      }

    gMotorVars.Flag_enableOffsetcalc = false;
  }

  // initialize the space vector generator module
  svgenHandle = SVGEN_init(&svgen,sizeof(svgen));

  // Initialize and setup the 100% SVM generator
  svgencurrentHandle = SVGENCURRENT_init(&svgencurrent,sizeof(svgencurrent));

  // setup svgen current
  {
    float_t minWidth_microseconds = 2.0;
    uint16_t minWidth_counts = (uint16_t)(minWidth_microseconds * USER_SYSTEM_FREQ_MHz);
    float_t fdutyLimit = 0.5-(2.0*minWidth_microseconds*USER_PWM_FREQ_kHz*0.001);
    _iq dutyLimit = _IQ(fdutyLimit);

    SVGENCURRENT_setMinWidth(svgencurrentHandle, minWidth_counts);
    SVGENCURRENT_setIgnoreShunt(svgencurrentHandle, use_all);
    SVGENCURRENT_setMode(svgencurrentHandle,all_phase_measurable);
    SVGENCURRENT_setVlimit(svgencurrentHandle,dutyLimit);
  }

  // set overmodulation to maximum value
  gMotorVars.OverModulation = _IQ(USER_MAX_VS_MAG_PU);
  // initialize the speed reference trajectory
  trajHandle_spd = TRAJ_init(&traj_spd,sizeof(traj_spd));

  // configure the speed reference trajectory
  TRAJ_setTargetValue(trajHandle_spd,_IQ(0.0));
  TRAJ_setIntValue(trajHandle_spd,_IQ(0.0));
  TRAJ_setMinValue(trajHandle_spd,_IQ(-1.0));
  TRAJ_setMaxValue(trajHandle_spd,_IQ(1.0));
  TRAJ_setMaxDelta(trajHandle_spd,_IQ(USER_MAX_ACCEL_Hzps / USER_IQ_FULL_SCALE_FREQ_Hz / USER_ISR_FREQ_Hz));

  // initialize the Id reference trajectory
  trajHandle_Id = TRAJ_init(&traj_Id,sizeof(traj_Id));

  if(USER_MOTOR_TYPE == MOTOR_Type_Pm)
    {
      // configure the Id reference trajectory
      TRAJ_setTargetValue(trajHandle_Id,_IQ(0.0));
      TRAJ_setIntValue(trajHandle_Id,_IQ(0.0));
      TRAJ_setMinValue(trajHandle_Id,_IQ(-USER_MOTOR_MAX_CURRENT / USER_IQ_FULL_SCALE_CURRENT_A));
      TRAJ_setMaxValue(trajHandle_Id,_IQ(USER_MOTOR_MAX_CURRENT / USER_IQ_FULL_SCALE_CURRENT_A));
      TRAJ_setMaxDelta(trajHandle_Id,_IQ(USER_MOTOR_RES_EST_CURRENT / USER_IQ_FULL_SCALE_CURRENT_A / USER_ISR_FREQ_Hz));

      // Initialize field weakening
      fwHandle = FW_init(&fw,sizeof(fw));
      FW_setFlag_enableFw(fwHandle, false); // Disable field weakening
      FW_clearCounter(fwHandle); // Clear field weakening counter
      FW_setNumIsrTicksPerFwTick(fwHandle, FW_NUM_ISR_TICKS_PER_CTRL_TICK); // Set the number of ISR per field weakening ticks
      FW_setDeltas(fwHandle, FW_INC_DELTA, FW_DEC_DELTA); // Set the deltas of field weakening
      FW_setOutput(fwHandle, _IQ(0.0)); // Set initial output of field weakening to zero
      FW_setMinMax(fwHandle,_IQ(USER_MAX_NEGATIVE_ID_REF_CURRENT_A/USER_IQ_FULL_SCALE_CURRENT_A),_IQ(0.0)); // Set the field weakening controller limits
    }
  else
    {
      // configure the Id reference trajectory
      TRAJ_setTargetValue(trajHandle_Id,_IQ(0.0));
      TRAJ_setIntValue(trajHandle_Id,_IQ(0.0));
      TRAJ_setMinValue(trajHandle_Id,_IQ(0.0));
      TRAJ_setMaxValue(trajHandle_Id,_IQ(USER_MOTOR_MAGNETIZING_CURRENT / USER_IQ_FULL_SCALE_CURRENT_A));
      TRAJ_setMaxDelta(trajHandle_Id,_IQ(USER_MOTOR_MAGNETIZING_CURRENT / USER_IQ_FULL_SCALE_CURRENT_A / USER_ISR_FREQ_Hz));
    }

  // initialize the CPU usage module
  cpu_usageHandle = CPU_USAGE_init(&cpu_usage,sizeof(cpu_usage));
  CPU_USAGE_setParams(cpu_usageHandle,
                      HAL_getTimerPeriod(halHandle,1),     // timer period, cnts
                     (uint32_t)USER_ISR_FREQ_Hz);                  // average over 1 second of ISRs

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

  // compute scaling factors for flux and torque calculations
  gFlux_pu_to_Wb_sf = USER_computeFlux_pu_to_Wb_sf();
  gFlux_pu_to_VpHz_sf = USER_computeFlux_pu_to_VpHz_sf();
  gTorque_Ls_Id_Iq_pu_to_Nm_sf = USER_computeTorque_Ls_Id_Iq_pu_to_Nm_sf();
  gTorque_Flux_Iq_pu_to_Nm_sf = USER_computeTorque_Flux_Iq_pu_to_Nm_sf();

  HAL_enableTimer0Int(halHandle);

  // enable the system by default
  gMotorVars.Flag_enableSys = true;

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

  setupCANMailBoxes();

  // Begin the background loop
  for(;;)
  {
    // Waiting for enable system flag to be set
    while(!(gMotorVars.Flag_enableSys)){
        updateTasks();
    }
    gMotorVars.MaxAccel_krpmps = _IQ(0.2);

    // loop while the enable system flag is true
    while(gMotorVars.Flag_enableSys)
      {

        if(gMotorVars.Flag_Run_Identify)
          {
            // disable Rs recalculation
            EST_setFlag_enableRsRecalc(estHandle,false);

            // update estimator state
            EST_updateState(estHandle,0);

            #ifdef FAST_ROM_V1p6
              // call this function to fix 1p6
              softwareUpdate1p6(estHandle);
            #endif

            // enable the PWM
            HAL_enablePwm(halHandle);

            // set trajectory target for speed reference
            TRAJ_setTargetValue(trajHandle_spd,_IQmpy(gMotorVars.SpeedRef_krpm, gSpeed_krpm_to_pu_sf));

            if(USER_MOTOR_TYPE == MOTOR_Type_Pm)
              {
                // set trajectory target for Id reference
                TRAJ_setTargetValue(trajHandle_Id,gIdq_ref_pu.value[0]);
              }
            else
              {
                if(gMotorVars.Flag_enablePowerWarp)
                  {
                    _iq Id_target_pw_pu = EST_runPowerWarp(estHandle,TRAJ_getIntValue(trajHandle_Id),gIdq_pu.value[1]);
                    TRAJ_setTargetValue(trajHandle_Id,Id_target_pw_pu);
                    TRAJ_setMinValue(trajHandle_Id,_IQ(USER_MOTOR_MAGNETIZING_CURRENT * 0.3 / USER_IQ_FULL_SCALE_CURRENT_A));
                    TRAJ_setMaxDelta(trajHandle_Id,_IQ(USER_MOTOR_MAGNETIZING_CURRENT * 0.3 / USER_IQ_FULL_SCALE_CURRENT_A / USER_ISR_FREQ_Hz));
                  }
                else
                  {
                    // set trajectory target for Id reference
                    TRAJ_setTargetValue(trajHandle_Id,_IQ(USER_MOTOR_MAGNETIZING_CURRENT / USER_IQ_FULL_SCALE_CURRENT_A));
                    TRAJ_setMinValue(trajHandle_Id,_IQ(0.0));
                    TRAJ_setMaxDelta(trajHandle_Id,_IQ(USER_MOTOR_MAGNETIZING_CURRENT / USER_IQ_FULL_SCALE_CURRENT_A / USER_ISR_FREQ_Hz));
                  }
              }
          }
        else if(gMotorVars.Flag_enableRsRecalc)
          {
            // set angle to zero
            EST_setAngle_pu(estHandle,_IQ(0.0));

            // enable or disable Rs recalculation
            EST_setFlag_enableRsRecalc(estHandle,true);

            // update estimator state
            EST_updateState(estHandle,0);

            #ifdef FAST_ROM_V1p6
              // call this function to fix 1p6
              softwareUpdate1p6(estHandle);
            #endif

            // enable the PWM
            HAL_enablePwm(halHandle);

            // set trajectory target for speed reference
            TRAJ_setTargetValue(trajHandle_spd,_IQ(0.0));

            // set trajectory target for Id reference
            TRAJ_setTargetValue(trajHandle_Id,_IQ(USER_MOTOR_RES_EST_CURRENT / USER_IQ_FULL_SCALE_CURRENT_A));

            // if done with Rs recalculation, disable flag
            if(EST_getState(estHandle) == EST_State_OnLine) gMotorVars.Flag_enableRsRecalc = false;
          }
        else
          {
            // set estimator to Idle
            EST_setIdle(estHandle);

            // disable the PWM
            if(!gMotorVars.Flag_enableOffsetcalc) HAL_disablePwm(halHandle);

            // clear the speed reference trajectory
            TRAJ_setTargetValue(trajHandle_spd,_IQ(0.0));
            TRAJ_setIntValue(trajHandle_spd,_IQ(0.0));

            // clear the Id reference trajectory
            TRAJ_setTargetValue(trajHandle_Id,_IQ(0.0));
            TRAJ_setIntValue(trajHandle_Id,_IQ(0.0));

            // configure trajectory Id defaults depending on motor type
            if(USER_MOTOR_TYPE == MOTOR_Type_Pm)
              {
                TRAJ_setMinValue(trajHandle_Id,_IQ(-USER_MOTOR_MAX_CURRENT / USER_IQ_FULL_SCALE_CURRENT_A));
                TRAJ_setMaxDelta(trajHandle_Id,_IQ(USER_MOTOR_RES_EST_CURRENT / USER_IQ_FULL_SCALE_CURRENT_A / USER_ISR_FREQ_Hz));
              }
            else
              {
                TRAJ_setMinValue(trajHandle_Id,_IQ(0.0));
                TRAJ_setMaxDelta(trajHandle_Id,_IQ(USER_MOTOR_MAGNETIZING_CURRENT / USER_IQ_FULL_SCALE_CURRENT_A / USER_ISR_FREQ_Hz));
              }

            // clear integral outputs
            PID_setUi(pidHandle[0],_IQ(0.0));
            PID_setUi(pidHandle[1],_IQ(0.0));
            PID_setUi(pidHandle[2],_IQ(0.0));

            // clear Id and Iq references
            gIdq_ref_pu.value[0] = _IQ(0.0);
            gIdq_ref_pu.value[1] = _IQ(0.0);

            // disable RsOnLine flags
            gFlag_enableRsOnLine = false;
            gFlag_updateRs = false;

            // disable PowerWarp flag
            gMotorVars.Flag_enablePowerWarp = true;
          }

        // update the global variables
        updateGlobalVariables(estHandle);

        // set field weakening enable flag depending on user's input
        FW_setFlag_enableFw(fwHandle,gMotorVars.Flag_enableFieldWeakening);

        // set the speed acceleration
        TRAJ_setMaxDelta(trajHandle_spd,_IQmpy(MAX_ACCEL_KRPMPS_SF,gMotorVars.MaxAccel_krpmps));

        // update CPU usage
        updateCPUusage();

        updateRsOnLine(estHandle);

        // enable/disable the forced angle
        EST_setFlag_enableForceAngle(estHandle,gMotorVars.Flag_enableForceAngle);


#ifdef DRV8301_SPI
        HAL_writeDrvData(halHandle,&gDrvSpi8301Vars);

        HAL_readDrvData(halHandle,&gDrvSpi8301Vars);
#endif
#ifdef DRV8305_SPI
        HAL_writeDrvData(halHandle,&gDrvSpi8305Vars);

        HAL_readDrvData(halHandle,&gDrvSpi8305Vars);
#endif

        updateTasks();
      } // end of while(gFlag_enableSys) loop

    // disable the PWM
    HAL_disablePwm(halHandle);

    gMotorVars.Flag_Run_Identify = false;
  } // end of for(;;) loop
} // end of main() function


//! \brief     The main ISR that implements the motor control.
interrupt void mainISR(void)
{
  if(++gLEDcnt >= (uint_least32_t)(USER_ISR_FREQ_Hz / LED_BLINK_FREQ_Hz))
   {
      if (errorFlag_GlobalError == 0){
          HAL_toggleLed(halHandle,(GPIO_Number_e)HAL_Gpio_LED2);
      }
      gLEDcnt = 0;
  }
  if(gMotorVars.Flag_Run_Identify){
      if (flagDCBusPowered == 0){
          gMotorVars.Flag_Run_Identify = false;
      }
  }

  if (_IQmpy(gAdcData.dcBus,_IQ(USER_IQ_FULL_SCALE_VOLTAGE_V/1000.0)) > _IQ(0.080)){
      flagDCBusPowered = 1;
      errorFlag_LowDCBus = false;
  }
  else{
      flagDCBusPowered = 0;
      gMotorVars.Flag_Run_Identify = false;
      errorFlag_LowDCBus = true;
  }

  _iq angle_pu = _IQ(0.0);
  _iq speed_pu = _IQ(0.0);
  _iq oneOverDcBus;
  MATH_vec2 Iab_pu;
  MATH_vec2 Vab_pu;
  MATH_vec2 phasor;
  uint32_t timer1Cnt;

  // read the timer 1 value and update the CPU usage module
  timer1Cnt = HAL_readTimerCnt(halHandle,1);
  CPU_USAGE_updateCnts(cpu_usageHandle,timer1Cnt);

  // acknowledge the ADC interrupt
  HAL_acqAdcInt(halHandle,ADC_IntNumber_1);

  // convert the ADC data
  HAL_readAdcDataWithOffsets(halHandle,&gAdcData);

  // remove offsets
  gAdcData.I.value[0] = gAdcData.I.value[0] - gOffsets_I_pu.value[0];
  gAdcData.I.value[1] = gAdcData.I.value[1] - gOffsets_I_pu.value[1];
  gAdcData.I.value[2] = gAdcData.I.value[2] - gOffsets_I_pu.value[2];
  gAdcData.V.value[0] = gAdcData.V.value[0] - gOffsets_V_pu.value[0];
  gAdcData.V.value[1] = gAdcData.V.value[1] - gOffsets_V_pu.value[1];
  gAdcData.V.value[2] = gAdcData.V.value[2] - gOffsets_V_pu.value[2];

  checkOverCurrent();

  /////////////////////////////////////////////////////////////////
  if (flagDCBusPowered == 1){
      // run the current reconstruction algorithm
      runCurrentReconstruction();

      // run Clarke transform on current
      CLARKE_run(clarkeHandle_I,&gAdcData.I,&Iab_pu);

      // run Clarke transform on voltage
      CLARKE_run(clarkeHandle_V,&gAdcData.V,&Vab_pu);

      // run a trajectory for Id reference, so the reference changes with a ramp instead of a step
      TRAJ_run(trajHandle_Id);

      // run the estimator
      EST_run(estHandle,
              &Iab_pu,
              &Vab_pu,
              gAdcData.dcBus,
              TRAJ_getIntValue(trajHandle_spd));

      // generate the motor electrical angle
      angle_pu = EST_getAngle_pu(estHandle);
      speed_pu = EST_getFm_pu(estHandle);

      // get Idq from estimator to avoid sin and cos
      EST_getIdq_pu(estHandle,&gIdq_pu);
  }
  ///////////////////////////////////////////////////////////////

  // run the appropriate controller
  if((gMotorVars.Flag_Run_Identify) || (gMotorVars.Flag_enableRsRecalc))
    {
      _iq refValue;
      _iq fbackValue;
      _iq outMax_pu;

      // calculate Id reference squared
       _iq Id_ref_squared_pu = _IQmpy(PID_getRefValue(pidHandle[1]),PID_getRefValue(pidHandle[1]));

       // Take into consideration that Iq^2+Id^2 = Is^2
       _iq Iq_Max_pu = _IQsqrt(gIs_Max_squared_pu - Id_ref_squared_pu);

      // when appropriate, run the PID speed controller
      if((pidCntSpeed++ >= USER_NUM_CTRL_TICKS_PER_SPEED_TICK) && (!gMotorVars.Flag_enableRsRecalc) && speedMode)
        {
          // clear counter
          pidCntSpeed = 0;
          Iq_Max_pu = _IQmpy(Iq_Max_pu, _IQ(((float)(0.1f * torqueMod))));

          // Set new min and max for the speed controller output
          PID_setMinMax(pidHandle[0], -Iq_Max_pu, Iq_Max_pu);

          //run speed controller
          PID_run_spd(pidHandle[0],TRAJ_getIntValue(trajHandle_spd),speed_pu,&(gIdq_ref_pu.value[1]));
        }

      // get the reference value from the trajectory module
      refValue = TRAJ_getIntValue(trajHandle_Id) + EST_getRsOnLineId_pu(estHandle);

      // get the feedback value
      fbackValue = gIdq_pu.value[0];

      // run the Id PID controller
      PID_run(pidHandle[1],refValue,fbackValue,&(gVdq_out_pu.value[0]));

      // set Iq reference to zero when doing Rs recalculation
      if(gMotorVars.Flag_enableRsRecalc) gIdq_ref_pu.value[1] = _IQ(0.0);

      // get the Iq reference value
      if (speedMode){
          refValue = gIdq_ref_pu.value[1];
      }
      else{
          if ((regen_brake_flag == 1) && (iqTorqueRequest == _IQ(0.0)) && (gMotorVars.Speed_krpm > _IQ(0.5)) && (torqueRequestDisable==0)){
              refValue = _IQmpy(Iq_Max_pu, _IQ(-0.4));
          }
          else{
              refValue = _IQmpy(Iq_Max_pu, iqTorqueRequest);
          }
      }



      // get the feedback value
      fbackValue = gIdq_pu.value[1];

      // calculate Iq controller limits, and run Iq controller
      _iq max_vs = _IQmpy(gMotorVars.OverModulation,EST_getDcBus_pu(estHandle));
      outMax_pu = _IQsqrt(_IQmpy(max_vs,max_vs) - _IQmpy(gVdq_out_pu.value[0],gVdq_out_pu.value[0]));
      PID_setMinMax(pidHandle[2],-outMax_pu,outMax_pu);
      PID_run(pidHandle[2],refValue,fbackValue,&(gVdq_out_pu.value[1]));

      // compensate angle for PWM delay
      angle_pu = angleDelayComp(speed_pu, angle_pu);

      // compute the sin/cos phasor
      phasor.value[0] = _IQcosPU(angle_pu);
      phasor.value[1] = _IQsinPU(angle_pu);

      // set the phasor in the inverse Park transform
      IPARK_setPhasor(iparkHandle,&phasor);

      // run the inverse Park module
      IPARK_run(iparkHandle,&gVdq_out_pu,&Vab_pu);

      // run the space Vector Generator (SVGEN) module
      oneOverDcBus = EST_getOneOverDcBus_pu(estHandle);
      Vab_pu.value[0] = _IQmpy(Vab_pu.value[0],oneOverDcBus);
      Vab_pu.value[1] = _IQmpy(Vab_pu.value[1],oneOverDcBus);
      SVGEN_run(svgenHandle,&Vab_pu,&(gPwmData.Tabc));

      // run the PWM compensation and current ignore algorithm
      SVGENCURRENT_compPwmData(svgencurrentHandle,&(gPwmData.Tabc),&gPwmData_prev);

      gTrjCnt++;
    }
  else if(gMotorVars.Flag_enableOffsetcalc == true)
    {
      runOffsetsCalculation();
    }
  else
    {
      // disable the PWM
      HAL_disablePwm(halHandle);

      // Set the PWMs to 50% duty cycle
      gPwmData.Tabc.value[0] = _IQ(0.0);
      gPwmData.Tabc.value[1] = _IQ(0.0);
      gPwmData.Tabc.value[2] = _IQ(0.0);
    }

  // write the PWM compare values
  HAL_writePwmData(halHandle,&gPwmData);

  if (gTrjCnt >= gUserParams.numCtrlTicksPerTrajTick)
  {
      if (flagDCBusPowered == 1){
          // clear counter
          gTrjCnt = 0;

          // run a trajectory for speed reference, so the reference changes with a ramp instead of a step
          TRAJ_run(trajHandle_spd);

      }
  }

  // run function to set next trigger
  if(!gMotorVars.Flag_enableRsRecalc) runSetTrigger();

  // run field weakening
  if(USER_MOTOR_TYPE == MOTOR_Type_Pm) runFieldWeakening();

  // read the timer 1 value and update the CPU usage module
  timer1Cnt = HAL_readTimerCnt(halHandle,1);
  CPU_USAGE_updateCnts(cpu_usageHandle,timer1Cnt);

  // run the CPU usage module
  CPU_USAGE_run(cpu_usageHandle);

  return;
} // end of mainISR() function


interrupt void timer0ISR(void)
{
    // Runs 100 times/second
    // acknowledge the Timer 0 interrupt
    HAL_acqTimer0Int(halHandle);

    if(++gCANRXcnt >= (uint_least32_t)(100 / CAN_TASK_FREQ_Hz))
    {
        gCANRX_Flag = 1;
        gCANRXcnt = 0;
    }

    if(++gCANTX1cnt >= (uint_least32_t)(100 / CAN_TASK_FREQ_Hz))
    {
        gCANTX1_Flag = 1;
        gCANTX1cnt = 0;
    }

    if(++gCANTX2cnt >= (uint_least32_t)(100 / CAN_TASK_FREQ_Hz))
    {
        gCANTX2_Flag = 1;
        gCANTX2cnt = 0;
    }

    if(++gCANTX3cnt >= (uint_least32_t)(100 / CAN_TASK_FREQ_Hz))
    {
        gCANTX3_Flag = 1;
        gCANTX3cnt = 0;
    }

    if(++gTEMPcnt >= (uint_least32_t)(100 / TEMP_TASK_FREQ_Hz))
    {
        gTEMP_Flag = 1;
        gTEMPcnt = 0;
    }
    return;
} // end of timer0ISR() function


_iq angleDelayComp(const _iq fm_pu, const _iq angleUncomp_pu)
{
  _iq angleDelta_pu = _IQmpy(fm_pu,_IQ(USER_IQ_FULL_SCALE_FREQ_Hz/(USER_PWM_FREQ_kHz*1000.0)));
  _iq angleCompFactor = _IQ(1.0 + (float_t)USER_NUM_PWM_TICKS_PER_ISR_TICK * 0.5);
  _iq angleDeltaComp_pu = _IQmpy(angleDelta_pu, angleCompFactor);
  uint32_t angleMask = ((uint32_t)0xFFFFFFFF >> (32 - GLOBAL_Q));
  _iq angleComp_pu;
  _iq angleTmp_pu;

  // increment the angle
  angleTmp_pu = angleUncomp_pu + angleDeltaComp_pu;

  // mask the angle for wrap around
  // note: must account for the sign of the angle
  angleComp_pu = _IQabs(angleTmp_pu) & angleMask;

  // account for sign
  if(angleTmp_pu < _IQ(0.0))
    {
      angleComp_pu = -angleComp_pu;
    }

  return(angleComp_pu);
} // end of angleDelayComp() function


void runCurrentReconstruction(void)
{
  SVGENCURRENT_MeasureShunt_e measurableShuntThisCycle = SVGENCURRENT_getMode(svgencurrentHandle);

  // run the current reconstruction algorithm
  SVGENCURRENT_RunRegenCurrent(svgencurrentHandle, (MATH_vec3 *)(gAdcData.I.value));

  gIavg.value[0] += (gAdcData.I.value[0] - gIavg.value[0])>>gIavg_shift;
  gIavg.value[1] += (gAdcData.I.value[1] - gIavg.value[1])>>gIavg_shift;
  gIavg.value[2] += (gAdcData.I.value[2] - gIavg.value[2])>>gIavg_shift;

  if(measurableShuntThisCycle > two_phase_measurable)
  {
      gAdcData.I.value[0] = gIavg.value[0];
      gAdcData.I.value[1] = gIavg.value[1];
      gAdcData.I.value[2] = gIavg.value[2];
  }

  return;
} // end of runCurrentReconstruction() function

void runSetTrigger(void)
{
  SVGENCURRENT_IgnoreShunt_e ignoreShuntNextCycle = SVGENCURRENT_getIgnoreShunt(svgencurrentHandle);
  SVGENCURRENT_VmidShunt_e midVolShunt = SVGENCURRENT_getVmid(svgencurrentHandle);

  // Set trigger point in the middle of the low side pulse
  HAL_setTrigger(halHandle,ignoreShuntNextCycle,midVolShunt);

  return;
} // end of runSetTrigger() function

void runFieldWeakening(void)
{
  if(FW_getFlag_enableFw(fwHandle) == true)
    {
      FW_incCounter(fwHandle);

      if(FW_getCounter(fwHandle) > FW_getNumIsrTicksPerFwTick(fwHandle))
        {
          _iq refValue;
          _iq fbackValue;

          FW_clearCounter(fwHandle);

          refValue = gMotorVars.VsRef;

          fbackValue =_IQmpy(gMotorVars.Vs,EST_getOneOverDcBus_pu(estHandle));

          FW_run(fwHandle, refValue, fbackValue, &(gIdq_ref_pu.value[0]));

          gMotorVars.IdRef_A = _IQmpy(gIdq_ref_pu.value[0], _IQ(USER_IQ_FULL_SCALE_CURRENT_A));
        }
    }
  else
    {
      gIdq_ref_pu.value[0] = _IQmpy(gMotorVars.IdRef_A, _IQ(1.0/USER_IQ_FULL_SCALE_CURRENT_A));
    }

  return;
} // end of runFieldWeakening() function


void runOffsetsCalculation(void)
{
  uint16_t cnt;

  // enable the PWM
  HAL_enablePwm(halHandle);

  for(cnt=0;cnt<3;cnt++)
    {
      // Set the PWMs to 50% duty cycle
      gPwmData.Tabc.value[cnt] = _IQ(0.0);

      // reset offsets used
      gOffsets_I_pu.value[cnt] = _IQ(0.0);
      gOffsets_V_pu.value[cnt] = _IQ(0.0);

      // run offset estimation
      FILTER_FO_run(filterHandle[cnt],gAdcData.I.value[cnt]);
      FILTER_FO_run(filterHandle[cnt+3],gAdcData.V.value[cnt]);
    }

  if(gOffsetCalcCount++ >= gUserParams.ctrlWaitTime[CTRL_State_OffLine])
    {
      gMotorVars.Flag_enableOffsetcalc = false;
      gOffsetCalcCount = 0;

      for(cnt=0;cnt<3;cnt++)
        {
          // get calculated offsets from filter
          gOffsets_I_pu.value[cnt] = FILTER_FO_get_y1(filterHandle[cnt]);
          gOffsets_V_pu.value[cnt] = FILTER_FO_get_y1(filterHandle[cnt+3]);

          // clear filters
          FILTER_FO_setInitialConditions(filterHandle[cnt],_IQ(0.0),_IQ(0.0));
          FILTER_FO_setInitialConditions(filterHandle[cnt+3],_IQ(0.0),_IQ(0.0));
        }
    }

  return;
} // end of runOffsetsCalculation() function


void softwareUpdate1p6(EST_Handle handle)
{
  float_t fullScaleInductance = USER_IQ_FULL_SCALE_VOLTAGE_V/(USER_IQ_FULL_SCALE_CURRENT_A*USER_VOLTAGE_FILTER_POLE_rps);
  float_t Ls_coarse_max = _IQ30toF(EST_getLs_coarse_max_pu(handle));
  int_least8_t lShift = ceil(log(USER_MOTOR_Ls_d/(Ls_coarse_max*fullScaleInductance))/log(2.0));
  uint_least8_t Ls_qFmt = 30 - lShift;
  float_t L_max = fullScaleInductance * pow(2.0,lShift);
  _iq Ls_d_pu = _IQ30(USER_MOTOR_Ls_d / L_max);
  _iq Ls_q_pu = _IQ30(USER_MOTOR_Ls_q / L_max);


  // store the results
  EST_setLs_d_pu(handle,Ls_d_pu);
  EST_setLs_q_pu(handle,Ls_q_pu);
  EST_setLs_qFmt(handle,Ls_qFmt);

  return;
} // end of softwareUpdate1p6() function

//! \brief     Setup the Clarke transform for either 2 or 3 sensors.
//! \param[in] handle             The clarke (CLARKE) handle
//! \param[in] numCurrentSensors  The number of current sensors
void setupClarke_I(CLARKE_Handle handle,const uint_least8_t numCurrentSensors)
{
  _iq alpha_sf,beta_sf;

  // initialize the Clarke transform module for current
  if(numCurrentSensors == 3)
    {
      alpha_sf = _IQ(MATH_ONE_OVER_THREE);
      beta_sf = _IQ(MATH_ONE_OVER_SQRT_THREE);
    }
  else if(numCurrentSensors == 2)
    {
      alpha_sf = _IQ(1.0);
      beta_sf = _IQ(MATH_ONE_OVER_SQRT_THREE);
    }
  else
    {
      alpha_sf = _IQ(0.0);
      beta_sf = _IQ(0.0);
    }

  // set the parameters
  CLARKE_setScaleFactors(handle,alpha_sf,beta_sf);
  CLARKE_setNumSensors(handle,numCurrentSensors);

  return;
} // end of setupClarke_I() function


//! \brief     Setup the Clarke transform for either 2 or 3 sensors.
//! \param[in] handle             The clarke (CLARKE) handle
//! \param[in] numVoltageSensors  The number of voltage sensors
void setupClarke_V(CLARKE_Handle handle,const uint_least8_t numVoltageSensors)
{
  _iq alpha_sf,beta_sf;

  // initialize the Clarke transform module for voltage
  if(numVoltageSensors == 3)
    {
      alpha_sf = _IQ(MATH_ONE_OVER_THREE);
      beta_sf = _IQ(MATH_ONE_OVER_SQRT_THREE);
    }
 else
    {
      alpha_sf = _IQ(0.0);
      beta_sf = _IQ(0.0);
    }

  // set the parameters
  CLARKE_setScaleFactors(handle,alpha_sf,beta_sf);
  CLARKE_setNumSensors(handle,numVoltageSensors);

  return;
} // end of setupClarke_V() function


//! \brief     Update the global variables (gMotorVars).
//! \param[in] handle  The estimator (EST) handle
void updateGlobalVariables(EST_Handle handle)
{
  // get the speed estimate
  gMotorVars.Speed_krpm = EST_getSpeed_krpm(handle);

  // get the torque estimate
  {
    _iq Flux_pu = EST_getFlux_pu(handle);
    _iq Id_pu = PID_getFbackValue(pidHandle[1]);
    _iq Iq_pu = PID_getFbackValue(pidHandle[2]);
    _iq Ld_minus_Lq_pu = _IQ30toIQ(EST_getLs_d_pu(handle)-EST_getLs_q_pu(handle));
    _iq Torque_Flux_Iq_Nm = _IQmpy(_IQmpy(Flux_pu,Iq_pu),gTorque_Flux_Iq_pu_to_Nm_sf);
    _iq Torque_Ls_Id_Iq_Nm = _IQmpy(_IQmpy(_IQmpy(Ld_minus_Lq_pu,Id_pu),Iq_pu),gTorque_Ls_Id_Iq_pu_to_Nm_sf);
    _iq Torque_Nm = Torque_Flux_Iq_Nm + Torque_Ls_Id_Iq_Nm;

    gMotorVars.Torque_Nm = Torque_Nm;
  }

  canMotorTorque = _IQtoF(gMotorVars.Torque_Nm);
  canMotorSpeedRPM = _IQtoF(gMotorVars.Speed_krpm) * 1000;

  if (canMotorSpeedRPM < -200.0){
      //errorFlag_ReverseSpeed = 1;
      //errorFlag_GlobalError = 1;
      //gMotorVars.Flag_Run_Identify = false;
  }

  // get the magnetizing current
  gMotorVars.MagnCurr_A = EST_getIdRated(handle);

  // get the rotor resistance
  gMotorVars.Rr_Ohm = EST_getRr_Ohm(handle);

  // get the stator resistance
  gMotorVars.Rs_Ohm = EST_getRs_Ohm(handle);

  // get the online stator resistance
  gMotorVars.RsOnLine_Ohm = EST_getRsOnLine_Ohm(handle);

  // get the stator inductance in the direct coordinate direction
  gMotorVars.Lsd_H = EST_getLs_d_H(handle);

  // get the stator inductance in the quadrature coordinate direction
  gMotorVars.Lsq_H = EST_getLs_q_H(handle);

  // get the flux in V/Hz in floating point
  gMotorVars.Flux_VpHz = EST_getFlux_VpHz(handle);

  // get the flux in Wb in fixed point
  gMotorVars.Flux_Wb = _IQmpy(EST_getFlux_pu(handle),gFlux_pu_to_Wb_sf);

  // get the estimator state
  gMotorVars.EstState = EST_getState(handle);

  // Get the DC buss voltage
  gMotorVars.VdcBus_kV = _IQmpy(gAdcData.dcBus,_IQ(USER_IQ_FULL_SCALE_VOLTAGE_V/1000.0));

  // read Vd and Vq vectors per units
  gMotorVars.Vd = gVdq_out_pu.value[0];
  gMotorVars.Vq = gVdq_out_pu.value[1];

  // calculate vector Vs in per units
  gMotorVars.Vs = _IQsqrt(_IQmpy(gMotorVars.Vd, gMotorVars.Vd) + _IQmpy(gMotorVars.Vq, gMotorVars.Vq));

  canVdcLink = _IQ24toF(gMotorVars.VdcBus_kV) * 1000;

  float_t input_power_sf = _IQtoF(gAdcData.dcBus) * (float_t)(1 * USER_IQ_FULL_SCALE_VOLTAGE_V * USER_IQ_FULL_SCALE_CURRENT_A);

  canInverterOutputPower = input_power_sf * _IQtoF(_IQmpy(gPwmData.Tabc.value[0], gAdcData.I.value[0]) + _IQmpy(gPwmData.Tabc.value[1], gAdcData.I.value[1]) + _IQmpy(gPwmData.Tabc.value[2], gAdcData.I.value[2]));

  // Equations for motor power

  canMotorOutputPower = _IQtoF(gMotorVars.Torque_Nm) * _IQtoF(gMotorVars.Speed_krpm) * (float_t)((MATH_TWO_PI * 1000.0) / 60.0);

  // Equation for DClink input current (estimative)

  canIdcLink = (canInverterOutputPower)/canVdcLink;


  return;
} // end of updateGlobalVariables() function


void updateRsOnLine(EST_Handle handle)
{
  // execute Rs OnLine code
  if(gMotorVars.Flag_Run_Identify == true)
    {
      if((EST_getState(handle) == EST_State_OnLine) && (gFlag_enableRsOnLine))
        {
          float_t RsError_Ohm = gMotorVars.RsOnLine_Ohm - gMotorVars.Rs_Ohm;

          EST_setFlag_enableRsOnLine(handle,true);
          EST_setRsOnLineId_mag_pu(handle,_IQmpy(gRsOnLineId_mag_A,_IQ(1.0/USER_IQ_FULL_SCALE_CURRENT_A)));
          EST_setRsOnLineAngleDelta_pu(handle,_IQmpy(gRsOnLineFreq_Hz, _IQ(1.0/USER_ISR_FREQ_Hz)));

          if(abs(RsError_Ohm) < (gMotorVars.Rs_Ohm * 0.05))
            {
              EST_setFlag_updateRs(handle,true);
            }
        }
      else
        {
          EST_setRsOnLineId_mag_pu(handle,_IQ(0.0));
          EST_setRsOnLineId_pu(handle,_IQ(0.0));
          EST_setRsOnLine_pu(handle, EST_getRs_pu(handle));
          EST_setFlag_enableRsOnLine(handle,false);
          EST_setFlag_updateRs(handle,false);
          EST_setRsOnLine_qFmt(handle,EST_getRs_qFmt(handle));
        }
    }

  return;
} // end of updateRsOnLine() function
void updateCPUusage(void)
{
  uint32_t minDeltaCntObserved = CPU_USAGE_getMinDeltaCntObserved(cpu_usageHandle);
  uint32_t avgDeltaCntObserved = CPU_USAGE_getAvgDeltaCntObserved(cpu_usageHandle);
  uint32_t maxDeltaCntObserved = CPU_USAGE_getMaxDeltaCntObserved(cpu_usageHandle);
  uint16_t pwmPeriod = HAL_readPwmPeriod(halHandle,PWM_Number_1);
  float_t  cpu_usage_den = (float_t)pwmPeriod * (float_t)USER_NUM_PWM_TICKS_PER_ISR_TICK * 2.0;

  // calculate the minimum cpu usage percentage
  gCpuUsagePercentageMin = (float_t)minDeltaCntObserved / cpu_usage_den * 100.0;

  // calculate the average cpu usage percentage
  gCpuUsagePercentageAvg = (float_t)avgDeltaCntObserved / cpu_usage_den * 100.0;

  // calculate the maximum cpu usage percentage
  gCpuUsagePercentageMax = (float_t)maxDeltaCntObserved / cpu_usage_den * 100.0;

  return;
} // end of updateCPUusage() function

void updateMotorTemperature(void)
{
    // Read the current raw temperature data
    uint16_t currentMotorTempData = HAL_readMotorTemperatureData(halHandle);

    // Update the moving average buffer
    if (isMotorTempBufferFull) {
        // Subtract the oldest sample from the sum
        sumMotorTempData -= motorTempDataSamples[motorTempSampleIndex];
    }

    // Add the new sample to the sum and the buffer
    sumMotorTempData += currentMotorTempData;
    motorTempDataSamples[motorTempSampleIndex] = currentMotorTempData;

    // Increment the sample index and wrap it around if necessary
    motorTempSampleIndex = (motorTempSampleIndex + 1) % NUM_TEMP_SAMPLES;

    // Check if the buffer is now full
    if (motorTempSampleIndex == 0 && !isMotorTempBufferFull) {
        isMotorTempBufferFull = true;
    }

    // Calculate the average of the samples in the buffer
    float_t averagedTempData = (float_t)(isMotorTempBufferFull ? (sumMotorTempData / NUM_TEMP_SAMPLES) : (sumMotorTempData / motorTempSampleIndex));
    averagedTempData = ADC_SCALE_FACTOR_TEMP * averagedTempData;

    // Convert the averaged raw data to temperature
    motorTemp = (MOTOR_TEMP_COEF_A * (averagedTempData) * (averagedTempData) * (averagedTempData)) +
                (MOTOR_TEMP_COEF_B * (averagedTempData) * (averagedTempData)) +
                (MOTOR_TEMP_COEF_C * (averagedTempData)) +
                MOTOR_TEMP_COEF_D;

    // Check if the temperature exceeds the threshold
    if (motorTemp > 110.0) {
        errorFlag_MotorOverTemperature = 1;
        errorFlag_GlobalError = 1;
        gMotorVars.Flag_Run_Identify = false;
    }
    else{
        //errorFlag_MotorOverTemperature = 0;
    }
}

void updateIGBTTemperature(void)
{
    // Read the current raw temperature data
    uint16_t currentTempData = HAL_readIGBTTemperatureData(halHandle);

    // Update the moving average buffer
    if (isIGBTTempBufferFull) {
        // Subtract the oldest sample from the sum
        sumIgbtTempData -= igbtTempDataSamples[igbtTempSampleIndex];
    }

    // Add the new sample to the sum and the buffer
    sumIgbtTempData += currentTempData;
    igbtTempDataSamples[igbtTempSampleIndex] = currentTempData;

    // Increment the sample index and wrap it around if necessary
    igbtTempSampleIndex = (igbtTempSampleIndex + 1) % NUM_TEMP_SAMPLES;

    // Check if the buffer is now full
    if (igbtTempSampleIndex == 0 && !isIGBTTempBufferFull) {
        isIGBTTempBufferFull = true;
    }

    // Calculate the average of the samples in the buffer
    float_t averagedTempData = (float_t)(isIGBTTempBufferFull ? (sumIgbtTempData / NUM_TEMP_SAMPLES) : (sumIgbtTempData / (igbtTempSampleIndex)));
    averagedTempData = ADC_SCALE_FACTOR_TEMP * averagedTempData;

    // Convert the averaged raw data to temperature
    igbtTemp = (IGBT_TEMP_COEF_A * (averagedTempData) * (averagedTempData) * (averagedTempData)) +
               (IGBT_TEMP_COEF_B * (averagedTempData) * (averagedTempData)) +
               (IGBT_TEMP_COEF_C * (averagedTempData)) +
               IGBT_TEMP_COEF_D + igbtTempOffset;

    // Check if the temperature exceeds the threshold
    if (igbtTemp > 45.0) {
        errorFlag_IGBTOverTemperature = 1;
        errorFlag_GlobalError = 1;
        gMotorVars.Flag_Run_Identify = false;
    }
    else{
        //errorFlag_IGBTOverTemperature = 0;
    }
}

void checkOverCurrent() {
    CurrentPhaseA = (float_t)(USER_IQ_FULL_SCALE_CURRENT_A) * _IQtoF(gAdcData.I.value[0]);
    CurrentPhaseB = (float_t)(USER_IQ_FULL_SCALE_CURRENT_A) * _IQtoF(gAdcData.I.value[1]);
    CurrentPhaseC = (float_t)(USER_IQ_FULL_SCALE_CURRENT_A) * _IQtoF(gAdcData.I.value[2]);

    // Phase A - HIGH_OVERCURRENT_THRESHOLD
    if (CurrentPhaseA > HIGH_OVERCURRENT_THRESHOLD || CurrentPhaseA < -HIGH_OVERCURRENT_THRESHOLD) {
        immediateOverCurrentCountA++;
        if (immediateOverCurrentCountA >= IMMEDIATE_OVERCURRENT_COUNT) {
            errorFlag_HOCD = 1;
            errorFlag_GlobalError = 1;
            gMotorVars.Flag_Run_Identify = false;
            return;
        }
    } else {
        immediateOverCurrentCountA = 0;
    }
    // Phase A - OVERCURRENT_THRESHOLD
    if ((CurrentPhaseA > OVERCURRENT_THRESHOLD) || (CurrentPhaseA < -OVERCURRENT_THRESHOLD)) {
        overCurrentCountA++;
        if (overCurrentCountA >= MAX_OVERCURRENT_COUNT) {
            errorFlag_OCD = 1;
            errorFlag_GlobalError = 1;
            gMotorVars.Flag_Run_Identify = false;
        }
    } else {
        overCurrentCountA = 0;
    }

    // Phase B - HIGH_OVERCURRENT_THRESHOLD
    if (CurrentPhaseB > HIGH_OVERCURRENT_THRESHOLD || CurrentPhaseB < -HIGH_OVERCURRENT_THRESHOLD) {
        immediateOverCurrentCountB++;
        if (immediateOverCurrentCountB >= IMMEDIATE_OVERCURRENT_COUNT) {
            errorFlag_HOCD = 1;
            errorFlag_GlobalError = 1;
            gMotorVars.Flag_Run_Identify = false;
            return;
        }
    } else {
        immediateOverCurrentCountB = 0;
    }
    // Phase B - OVERCURRENT_THRESHOLD
    if ((CurrentPhaseB > OVERCURRENT_THRESHOLD) || (CurrentPhaseB < -OVERCURRENT_THRESHOLD)) {
        overCurrentCountB++;
        if (overCurrentCountB >= MAX_OVERCURRENT_COUNT) {
            errorFlag_OCD = 1;
            errorFlag_GlobalError = 1;
            gMotorVars.Flag_Run_Identify = false;
        }
    } else {
        overCurrentCountB = 0;
    }

    // Phase C - HIGH_OVERCURRENT_THRESHOLD
    if (CurrentPhaseC > HIGH_OVERCURRENT_THRESHOLD || CurrentPhaseC < -HIGH_OVERCURRENT_THRESHOLD) {
        immediateOverCurrentCountC++;
        if (immediateOverCurrentCountC >= IMMEDIATE_OVERCURRENT_COUNT) {
            errorFlag_HOCD = 1;
            errorFlag_GlobalError = 1;
            gMotorVars.Flag_Run_Identify = false;
            return;
        }
    } else {
        immediateOverCurrentCountC = 0;
    }
    // Phase C - OVERCURRENT_THRESHOLD
    if ((CurrentPhaseC > OVERCURRENT_THRESHOLD) || (CurrentPhaseC < -OVERCURRENT_THRESHOLD)) {
        overCurrentCountC++;
        if (overCurrentCountC >= MAX_OVERCURRENT_COUNT) {
            errorFlag_OCD = 1;
            errorFlag_GlobalError = 1;
            gMotorVars.Flag_Run_Identify = false;
        }
    } else {
        overCurrentCountC = 0;
    }
}

// Função para mapear accel_value para Iq usando IQmath
float map_accel_to_float(uint16_t accel_value) {
    // Converte accel_value para float e mapeia para o intervalo de 0 a IQ_MAX
    float float_value = ((float)accel_value / 65535) * ((float)(0.1f * torqueMod));
    return float_value;
}

float map_accel_to_float_driverless(uint16_t accel_value) {
    // Converte accel_value para float e mapeia para o intervalo de 0 a IQ_MAX
    float float_value = ((float)accel_value / 65535) * 3.0f;
    return float_value;
}

void setupCANMailBoxes(void)
{
    // 0x046 -> 0x300
    // 0x047 -> 0x301
    // 0x048 -> 0x302
    //                                                   MailBoxID                                                              Mask
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox0,  0x300, Enable_Mbox, Tx_Dir, Standard_ID, DLC_8, Overwrite_on, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox1,  0x301, Enable_Mbox, Tx_Dir, Standard_ID, DLC_8, Overwrite_on, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox2,  0x302, Enable_Mbox, Tx_Dir, Standard_ID, DLC_8, Overwrite_on, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox3,  0x303, Enable_Mbox, Tx_Dir, Standard_ID, DLC_8, Overwrite_on, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox4,  0x304, Enable_Mbox, Tx_Dir, Standard_ID, DLC_8, Overwrite_on, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox5,  0x305, Enable_Mbox, Tx_Dir, Standard_ID, DLC_8, Overwrite_on, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox6,  0x306, Enable_Mbox, Tx_Dir, Standard_ID, DLC_8, Overwrite_on, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox7,  6, Disable_Mbox, Tx_Dir, Standard_ID, DLC_8, Overwrite_on, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox8,  6, Disable_Mbox, Tx_Dir, Standard_ID, DLC_8, Overwrite_on, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox9,  6, Disable_Mbox, Tx_Dir, Standard_ID, DLC_8, Overwrite_on, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox10, 6, Disable_Mbox, Tx_Dir, Standard_ID, DLC_8, Overwrite_on, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox11, 6, Disable_Mbox, Tx_Dir, Standard_ID, DLC_8, Overwrite_on, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox12, 6, Disable_Mbox, Tx_Dir, Standard_ID, DLC_8, Overwrite_on, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox13, 6, Disable_Mbox, Tx_Dir, Standard_ID, DLC_8, Overwrite_on, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox14, 6, Disable_Mbox, Tx_Dir, Standard_ID, DLC_8, Overwrite_on, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox15, 0x106, Enable_Mbox, Tx_Dir, Standard_ID, DLC_8, Overwrite_on, LAMI_0, Mask_not_used, 0x00000000);

    ECAN_setTx_Priority(halHandle->ecanaHandle, MailBox15, Tx_leve31);

    // 0x042 -> 0x171

    ECAN_configMailbox(halHandle->ecanaHandle, MailBox16, 0x041, Enable_Mbox, Rx_Dir, Standard_ID, DLC_8, Overwrite_off, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox17, 0x171, Enable_Mbox, Rx_Dir, Standard_ID, DLC_8, Overwrite_off, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox18, 0x043, Enable_Mbox, Rx_Dir, Standard_ID, DLC_8, Overwrite_off, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox19, 0x044, Enable_Mbox, Rx_Dir, Standard_ID, DLC_8, Overwrite_off, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox20, 0x045, Enable_Mbox, Rx_Dir, Standard_ID, DLC_8, Overwrite_off, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox21, 6, Disable_Mbox, Rx_Dir, Standard_ID, DLC_8, Overwrite_off, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox22, 6, Disable_Mbox, Rx_Dir, Standard_ID, DLC_8, Overwrite_off, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox23, 6, Disable_Mbox, Rx_Dir, Standard_ID, DLC_8, Overwrite_off, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox24, 6, Disable_Mbox, Rx_Dir, Standard_ID, DLC_8, Overwrite_off, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox25, 6, Disable_Mbox, Rx_Dir, Standard_ID, DLC_8, Overwrite_off, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox26, 6, Disable_Mbox, Rx_Dir, Standard_ID, DLC_8, Overwrite_off, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox27, 6, Disable_Mbox, Rx_Dir, Standard_ID, DLC_8, Overwrite_off, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox28, 6, Disable_Mbox, Rx_Dir, Standard_ID, DLC_8, Overwrite_off, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox29, 6, Disable_Mbox, Rx_Dir, Standard_ID, DLC_8, Overwrite_off, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox30, 6, Disable_Mbox, Rx_Dir, Standard_ID, DLC_8, Overwrite_off, LAMI_0, Mask_not_used, 0x00000000);
    ECAN_configMailbox(halHandle->ecanaHandle, MailBox31, 6, Disable_Mbox, Rx_Dir, Standard_ID, DLC_8, Overwrite_off, LAMI_0, Mask_not_used, 0x00000000);  // 1F80
    //                                    TX_max    TX_min    RX_max     RX_min
    ECAN_initMailboxUse(&gECAN_Mailbox, MailBox15, MailBox0, MailBox31, MailBox16);

    //  ECAN_setTx_Priority(halHandle->ecanaHandle, MailBox0, Tx_leve31);
    //  ECAN_SelfTest(halHandle->ecanaHandle, Self_test_mode);
    ECAN_SelfTest(halHandle->ecanaHandle, Normal_mode);

    FIFO_FLUSH(gECAN_rxFIFO_ID);
    FIFO_FLUSH(gECAN_txFIFO_ID);
}

void readCAN(void){
    if(ECAN_checkMail(halHandle->ecanaHandle)) {
        ECAN_getMsgFIFO_ID_N(halHandle->ecanaHandle, &gECAN_Mailbox, &gECAN_rxFIFO_ID);
    }
    while(!(FIFO_IS_EMPTY(gECAN_rxFIFO_ID))) {
        msg_temp = FIFO_FRONT(gECAN_rxFIFO_ID);
        FIFO_POP(gECAN_rxFIFO_ID);
        int i = 0;
        i = msg_temp.msgID;
        msg_temp.dataL = reverse_bytes(msg_temp.dataL);
        msg_temp.dataH = reverse_bytes(msg_temp.dataH);
        switch(i) {
                case 0x041:
                    torqueMod = (uint8_t)(msg_temp.dataL & 0xFF);
                    if (torqueMod > 10){
                        torqueMod = 10;
                    }
                    break;
                case 0x043:
                    DCL_value = (msg_temp.dataL & 0xFFFF); // Bytes 0-1
                    CCL_value = (msg_temp.dataL >> 16); // Bytes 2-3
                    battery_pack_value = (msg_temp.dataH & 0xFFFF); // Bytes 4-5
                    break;
                case 0x171:
                    accel_value = (msg_temp.dataL & 0xFFFF);       // Bytes 0-1
                    brake_value = (msg_temp.dataL >> 16);          // Bytes 2-3
                    regen_brake_flag = (msg_temp.dataH & 0xFF);     // Byte 4
                    rtd_flag = ((msg_temp.dataH >> 8) & 0xFF);      // Byte 5
                    event_mode = ((msg_temp.dataH >> 16) & 0xFF);         // Byte 6
                    torqueRequestDisable = ((msg_temp.dataH >> 24) & 0xFF); // Byte 7

                    //iqTorqueRequest = (long) ((map_accel_to_float(accel_value)) * 16777216.0L);

                    if (torqueRequestDisable == 1){
                        iqTorqueRequest = _IQ(0.0);
                    }
                    else{
                        iqTorqueRequest = (long) ((map_accel_to_float(accel_value)) * 16777216.0L);
                    }

                    if (event_mode == 4){
                        speedMode = true;
                        gMotorVars.SpeedRef_krpm = (long) ((map_accel_to_float_driverless(accel_value)) * 16777216.0L);
                    }
                    else{
                        speedMode = false;
                    }

                    if ((rtd_flag == 1) && (errorFlag_OCD == 0) && (errorFlag_HOCD == 0) && (errorFlag_IGBTOverTemperature == 0) && (errorFlag_ReverseSpeed == 0)){

                        if ((accel_value > 0)){
                            gMotorVars.Flag_Run_Identify = true;
                        }
                        else{
                            if (gMotorVars.Speed_krpm < _IQ(0.2)){
                                gMotorVars.Flag_Run_Identify = false;
                            }
                        }
                    }
                    else{
                        gMotorVars.Flag_Run_Identify = false;
                    }

                    break;
                default:
                    break;
            }
      }
      FIFO_FLUSH(gECAN_rxFIFO_ID);

}

uint8_t createErrorByte(bool error1, bool error2, bool error3, bool error4, bool error5, bool error6, bool error7, bool error8) {
    uint8_t errorFlags = 0;

    if (error1) errorFlags |= (1 << 0);  // Set bit 0 if error1 is HOCD
    if (error2) errorFlags |= (1 << 1);  // Set bit 1 if error2 is OCD
    if (error3) errorFlags |= (1 << 2);  // Set bit 2 if error3 is ReverseSpeed
    if (error4) errorFlags |= (1 << 3);  // Set bit 3 if error4 is MotorOverTemperature
    if (error5) errorFlags |= (1 << 4);  // Set bit 4 if error5 is IGBTOverTemperature
    if (error6) errorFlags |= (1 << 5);  // Set bit 5 if error6 is tbd
    if (error7) errorFlags |= (1 << 6);  // Set bit 6 if error7 is tbd
    if (error8) errorFlags |= (1 << 7);  // Set bit 7 if error8 is tbd

    return errorFlags;
}

void updateTasks(void)
{
    if (gCANRX_Flag == 1){
        gCANRX_Flag = 0;
        readCAN();
    }
    #ifdef ENABLE_CAN_SEND
    if (gCANTX1_Flag == 1){
        gCANTX1_Flag = 0;
        send_CAN_floats(halHandle->ecanaHandle, MailBox3, 0x303, canIdcLink, canVdcLink);
        send_CAN_floats(halHandle->ecanaHandle, MailBox2, 0x302, igbtTemp, motorTemp);
    }
    if (gCANTX2_Flag == 1){
        gCANTX2_Flag = 0;
        send_CAN_floats(halHandle->ecanaHandle, MailBox4, 0x304, canMotorOutputPower, canInverterOutputPower);
        //send_CAN_floats(halHandle->ecanaHandle, MailBox1, 0x301, 42.42, 12.0);
    }
    if (gCANTX3_Flag == 1){
        gCANTX3_Flag = 0;
        canError = createErrorByte(errorFlag_HOCD, errorFlag_OCD, errorFlag_ReverseSpeed, errorFlag_MotorOverTemperature, errorFlag_IGBTOverTemperature, 0, 0, 0);
        //send_CAN_message_048(halHandle->ecanaHandle, (gMotorVars.Flag_Run_Identify), ((uint8_t)(gCpuUsagePercentageAvg)), torqueMod, canError);
        send_CAN_floats(halHandle->ecanaHandle, MailBox5, 0x305, canMotorTorque, canMotorSpeedRPM);
        send_CAN_byte(halHandle->ecanaHandle, MailBox15, 0x106, canError);
    }
    #endif
    if (gTEMP_Flag == 1){
        gTEMP_Flag = 0;
        updateMotorTemperature();
        updateIGBTTemperature();
    }
}

//@} //defgroup
// end of file



