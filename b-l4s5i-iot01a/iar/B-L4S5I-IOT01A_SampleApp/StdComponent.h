/**************************************************************************/
/*                                                                        */
/*       Copyright (c) Microsoft Corporation. All rights reserved.        */
/*                                                                        */
/*       This software is licensed under the Microsoft Software License   */
/*       Terms for Microsoft Azure RTOS. Full text of the license can be  */
/*       found in the LICENSE file at https://aka.ms/AzureRTOS_EULA       */
/*       and in the root directory of this software.                      */
/*                                                                        */
/**************************************************************************/

#ifndef STD_COMPONENT_H
#define STD_COMPONENT_H

#ifdef __cplusplus
extern   "C" {
#endif

#include "nx_azure_iot_pnp_client.h"
#include "nx_azure_iot_json_reader.h"
#include "nx_azure_iot_json_writer.h"
#include "nx_api.h"

/**
  * @brief  Accelerometer Full Scale structure definition
  */
typedef enum
{
  STD_COMP_ACC_FS_2G      = 0x00,
  STD_COMP_ACC_FS_4G      = 0x01,
  STD_COMP_ACC_FS_8G      = 0x02,
  STD_COMP_ACC_FS_16G     = 0x03
} STD_COMP_AccFullScaleTypeDef;

/**
  * @brief  Magnetometer Full Scale structure definition
  */
typedef enum
{
  STD_COMP_MAG_FS_4GAUSS  = 0x00,
  STD_COMP_MAG_FS_8GAUSS  = 0x01,
  STD_COMP_MAG_FS_12GAUSS = 0x02,
  STD_COMP_MAG_FS_16GAUSS = 0x03
} STD_COMP_MagFullScaleTypeDef;


/**
  * @brief  Standard Component definition
  */
typedef enum
{
  STD_COMP_GYRO_FS_125_DPS      = 0x00,
  STD_COMP_GYRO_FS_250_DPS      = 0x01,
  STD_COMP_GYRO_FS_500_DPS      = 0x02,
  STD_COMP_GYRO_FS_1000_DPS     = 0x03,
  STD_COMP_GYRO_FS_2000_DPS     = 0x04
} STD_COMP_GyroFullScaleTypeDef;

typedef struct STD_COMPONENT_TAG
{
    /* Name of this component */
    UCHAR *component_name_ptr;

    /* Name length of this component */
    UINT component_name_length;

    /*************
     * Telemetry *
     *************/
    
    /* Current acceleration X/Y/Z */
    double Acc_X;
    double Acc_Y;
    double Acc_Z;
    
    /* Current gyroscope X/Y/Z  */
    double Gyro_X;
    double Gyro_Y;
    double Gyro_Z;
    
    /* Current magnetometer X/Y/Z  */
    double Mag_X;
    double Mag_Y;
    double Mag_Z;
    

    /* Current temperature */
    double Temperature;
    
    /* Current humidity  */
    double Humidity;
    
    /* Current Pressure  */
    double Pressure;
	
    /* Current Distance */
    double Distance;
    
    /************
     * Property *
     ************/
  
    /* Trasmission Interval */
    int SendIntervalSec;
    double CurrentTelemetryInterval;
    UCHAR ReceivedTelemetyInterval;
    
    /* Current accelerometer full scale */
    STD_COMP_AccFullScaleTypeDef currentAccFS;
    UCHAR ReceivedDesiredAccFS;
    
    /* Current magnetometer full scale */
    STD_COMP_MagFullScaleTypeDef currentMagFS;
    UCHAR ReceivedDesiredMagFS;
    
    /* Current gyroscope full scale */
    STD_COMP_GyroFullScaleTypeDef currentGyroFS;
    UCHAR ReceivedDesiredGyroFS;
} STD_COMPONENT;

extern UINT StdComponent_init(STD_COMPONENT *handle,
                              UCHAR *component_name_ptr,
                              UINT component_name_length);

extern UINT StdComponent_report_all_properties(STD_COMPONENT *handle,
                                               UCHAR *component_name_ptr,
                                               UINT component_name_len,
                                               NX_AZURE_IOT_PNP_CLIENT *iotpnp_client_ptr);

extern UINT StdComponent_telemetry_send(STD_COMPONENT *handle,
                                        NX_AZURE_IOT_PNP_CLIENT *iotpnp_client_ptr);

extern UINT StdComponent_process_property_update(STD_COMPONENT *handle,
                                                 NX_AZURE_IOT_PNP_CLIENT *iotpnp_client_ptr,
                                                 const UCHAR *component_name_ptr, UINT component_name_length,
                                                 NX_AZURE_IOT_JSON_READER *name_value_reader_ptr, UINT version);

extern VOID sample_send_target_StdComponent_acc_enable_report(STD_COMPONENT *handle,
                                                              NX_AZURE_IOT_PNP_CLIENT *iotpnp_client_ptr,
                                                              UINT Value,
                                                              INT status_code,
                                                              UINT version,
                                                              const CHAR *description);

extern VOID sample_send_target_StdComponent_acc_fullscale_report(STD_COMPONENT *handle,
                                                                 NX_AZURE_IOT_PNP_CLIENT *iotpnp_client_ptr,
                                                                 int32_t Value,
                                                                 INT status_code, UINT version,
                                                                 const CHAR *description);

extern VOID sample_send_target_StdComponent_mag_fullscale_report(STD_COMPONENT *handle,
                                                                 NX_AZURE_IOT_PNP_CLIENT *iotpnp_client_ptr,
                                                                 int32_t Value,
                                                                 INT status_code,
                                                                 UINT version,
                                                                 const CHAR *description);

extern VOID sample_send_target_StdComponent_gyro_fullscale_report(STD_COMPONENT *handle,
                                                                  NX_AZURE_IOT_PNP_CLIENT *iotpnp_client_ptr,
                                                                  int32_t Value,
                                                                  INT status_code,
                                                                  UINT version,
                                                                  const CHAR *description);

extern VOID sample_send_target_StdComponent_telemetry_interval_report(STD_COMPONENT *handle,
                                                                      NX_AZURE_IOT_PNP_CLIENT *iotpnp_client_ptr,
                                                                      double Value,
                                                                      UINT status_code,
                                                                      UINT version,
                                                                      const CHAR *description);

extern UINT StdComp_property_sent;

#ifdef __cplusplus
}
#endif
#endif /* STD_COMPONENT_H */
