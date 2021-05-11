/**
  ******************************************************************************
  * @file    stm32l475e_iot01_proximity.c
  * @author  MCD Application Team
  * @brief   This file provides a set of functions needed to manage the temperature sensor
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
#include "stm32l4s5i_iot01a_proximity.h"
  
extern I2C_HandleTypeDef hI2cHandler;

VL53L0X_Dev_t VL53L0XDev;
VL53L0X_RangingMeasurementData_t RangingMeasurementData;
int LeakyFactorFix8 = (int)( 0.0 *256);

int BSP_Proximity_Init()
{
  RangingConfig_e RangingConfig = LONG_RANGE;
  uint16_t Id;
  int status;
    
  VL53L0XDev.I2cHandle = &hI2cHandler;
  
  VL53L0X_Dev_t *pDev;
  pDev = &VL53L0XDev;
  pDev->I2cDevAddr = 0x52;
  pDev->Present = 0;
  HAL_Delay(2);
  
  /* Set VL53L0X API trace level */
  VL53L0X_trace_config(NULL, TRACE_MODULE_NONE, TRACE_LEVEL_NONE, TRACE_FUNCTION_NONE); // No Trace
  
  status = VL53L0X_WrByte(pDev, 0x88, 0x00);
  
  /* Try to read one register using default 0x52 address */
  status = VL53L0X_RdWord(pDev, VL53L0X_REG_IDENTIFICATION_MODEL_ID, &Id);
  
  if (status) {
    printf("VL53L0X Read id fail\r\n");
  }
  
  if (Id == 0xEEAA) {
    status = VL53L0X_DataInit(pDev);
    if( status == 0 ){
      pDev->Present = 1;
      status = SetupSingleShot(RangingConfig);
    }
    else{
      printf("VL53L0X_DataInit fail\r\n");
    }
  }
  else {
    printf("unknown ID %x\r\n", Id);
    status = 1;
  }
  
  return status;
}

int SetupSingleShot(RangingConfig_e rangingConfig){

  int status;
  uint8_t VhvSettings;
  uint8_t PhaseCal;
  uint32_t refSpadCount;
  uint8_t isApertureSpads;
  FixPoint1616_t signalLimit = (FixPoint1616_t)(0.25*65536);
  FixPoint1616_t sigmaLimit = (FixPoint1616_t)(18*65536);
  uint32_t timingBudget = 33000;
  uint8_t preRangeVcselPeriod = 14;
  uint8_t finalRangeVcselPeriod = 10;
  
  status=VL53L0X_StaticInit(&VL53L0XDev);
  if( status ){
    printf("VL53L0X_StaticInit failed\r\n");
  }
  
  status = VL53L0X_PerformRefCalibration(&VL53L0XDev, &VhvSettings, &PhaseCal);
  if( status ){
    printf("VL53L0X_PerformRefCalibration failed\r\n");
  }
  
  status = VL53L0X_PerformRefSpadManagement(&VL53L0XDev, &refSpadCount, &isApertureSpads);
  if( status ){
    printf("VL53L0X_PerformRefSpadManagement failed\r\n");
  }
  
  status = VL53L0X_SetDeviceMode(&VL53L0XDev, VL53L0X_DEVICEMODE_SINGLE_RANGING); // Setup in single ranging mode
  if( status ){
    printf("VL53L0X_SetDeviceMode failed\r\n");
  }
  
  status = VL53L0X_SetLimitCheckEnable(&VL53L0XDev, VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE, 1); // Enable Sigma limit
  if( status ){
    printf("VL53L0X_SetLimitCheckEnable failed\r\n");
  }
  
  status = VL53L0X_SetLimitCheckEnable(&VL53L0XDev, VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE, 1); // Enable Signa limit
  if( status ){
    printf("VL53L0X_SetLimitCheckEnable failed\r\n");
  }
  /* Ranging configuration */
  switch(rangingConfig) {
  case LONG_RANGE:
    signalLimit = (FixPoint1616_t)(0.1*65536);
    sigmaLimit = (FixPoint1616_t)(60*65536);
    timingBudget = 33000;
    preRangeVcselPeriod = 18;
    finalRangeVcselPeriod = 14;
    break;
  case HIGH_ACCURACY:
    signalLimit = (FixPoint1616_t)(0.25*65536);
    sigmaLimit = (FixPoint1616_t)(18*65536);
    timingBudget = 200000;
    preRangeVcselPeriod = 14;
    finalRangeVcselPeriod = 10;
    break;
  case HIGH_SPEED:
    signalLimit = (FixPoint1616_t)(0.25*65536);
    sigmaLimit = (FixPoint1616_t)(32*65536);
    timingBudget = 20000;
    preRangeVcselPeriod = 14;
    finalRangeVcselPeriod = 10;
    break;
  default:
    printf("Not Supported\r\n");
  }
  
  status = VL53L0X_SetLimitCheckValue(&VL53L0XDev,  VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE, signalLimit);
  if( status ){
    printf("VL53L0X_SetLimitCheckValue failed\r\n");
  }
  
  status = VL53L0X_SetLimitCheckValue(&VL53L0XDev,  VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE, sigmaLimit);
  if( status ){
    printf("VL53L0X_SetLimitCheckValue failed\r\n");
  }
  
  status = VL53L0X_SetMeasurementTimingBudgetMicroSeconds(&VL53L0XDev,  timingBudget);
  if( status ){
    printf("VL53L0X_SetMeasurementTimingBudgetMicroSeconds failed\r\n");
  }
  
  status = VL53L0X_SetVcselPulsePeriod(&VL53L0XDev,  VL53L0X_VCSEL_PERIOD_PRE_RANGE, preRangeVcselPeriod);
  if( status ){
    printf("VL53L0X_SetVcselPulsePeriod failed\r\n");
  }
  
  status = VL53L0X_SetVcselPulsePeriod(&VL53L0XDev,  VL53L0X_VCSEL_PERIOD_FINAL_RANGE, finalRangeVcselPeriod);
  if( status ){
    printf("VL53L0X_SetVcselPulsePeriod failed\r\n");
  }
  
  status = VL53L0X_PerformRefCalibration(&VL53L0XDev, &VhvSettings, &PhaseCal);
  if( status ){
    printf("VL53L0X_PerformRefCalibration failed\r\n");
  }
  
  VL53L0XDev.LeakyFirst=1;
  
  return status;
}


float BSP_Proximity_Read()
{
  int status;
  float value;
  status = VL53L0X_PerformSingleRangingMeasurement(&VL53L0XDev,&RangingMeasurementData);
  if( status ==0 ){
    /* Push data logging to UART */
    Sensor_SetNewRange(&VL53L0XDev,&RangingMeasurementData);
  }
  else{
    //while(1);
  }
  
  if( RangingMeasurementData.RangeStatus == 0 ){
    value = (float)VL53L0XDev.LeakyRange/10;
  }
  else{
    value = 0;
  }
  
  return value;
}

/* Store new ranging data into the device structure, apply leaky integrator if needed */
void Sensor_SetNewRange(VL53L0X_Dev_t *pDev, VL53L0X_RangingMeasurementData_t *pRange){
  if( pRange->RangeStatus == 0 ){
    if( pDev->LeakyFirst ){
      pDev->LeakyFirst = 0;
      pDev->LeakyRange = pRange->RangeMilliMeter;
    }
    else{
      pDev->LeakyRange = (pDev->LeakyRange*LeakyFactorFix8 + (256-LeakyFactorFix8)*pRange->RangeMilliMeter)>>8;
    }
  }
  else{
    pDev->LeakyFirst = 1;
  }
}
