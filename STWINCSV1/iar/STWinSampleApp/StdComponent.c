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

/* Includes ------------------------------------------------------------------*/
#include "StdComponent.h"
#include "STWIN_env_sensors.h"
#include "STWIN_motion_sensors.h"
#include "SampleCommonDefine.h"

/* exported variables  */
UINT StdComp_property_sent;

static UCHAR scratch_buffer[512];

/* Telemetry key */
static const CHAR StdComp_acc_value_telemetry_name[] = "acceleration";
static const CHAR StdComp_temp_value_telemetry_name[] = "temperature";
static const CHAR StdComp_hum_value_telemetry_name[]  = "humidity";
static const CHAR StdComp_press_value_telemetry_name[] = "pressure";
static const CHAR StdComp_gryo_value_telemetry_name[]  = "gyroscope";
static const CHAR StdComp_mag_value_telemetry_name[] = "magnetometer";

/* Property Names  */
static const CHAR target_StdComponent_acc_fullscale_property_name[] = "acc_fullscale";
static const CHAR target_StdComponent_gryo_fullscale_property_name[] = "gyro_fullscale";
static const CHAR target_StdComponent_mag_fullscale_property_name[] = "magneto_fullscale";
static const CHAR target_StdComponent_telemetry_interval_property_name[] = "telemetry_interval";

static UINT append_properties(STD_COMPONENT *handle,NX_AZURE_IOT_JSON_WRITER *json_writer)
{
  UINT status;
  
  if (nx_azure_iot_json_writer_append_property_with_int32_value(json_writer,
                                                                 (UCHAR *)target_StdComponent_gryo_fullscale_property_name,
                                                                 sizeof(target_StdComponent_gryo_fullscale_property_name) - 1,
                                                                 handle->currentGyroFS) ||
        nx_azure_iot_json_writer_append_property_with_int32_value(json_writer,
                                                                 (UCHAR *)target_StdComponent_acc_fullscale_property_name,
                                                                 sizeof(target_StdComponent_acc_fullscale_property_name) - 1,
                                                                 handle->currentAccFS) ||
          nx_azure_iot_json_writer_append_property_with_int32_value(json_writer,
                                                                 (UCHAR *)target_StdComponent_mag_fullscale_property_name,
                                                                 sizeof(target_StdComponent_mag_fullscale_property_name) - 1,
                                                                 handle->currentMagFS) ||
            nx_azure_iot_json_writer_append_property_with_double_value(json_writer,
                                                                 (UCHAR *)target_StdComponent_telemetry_interval_property_name,
                                                                 sizeof(target_StdComponent_telemetry_interval_property_name) - 1,
                                                                 handle->CurrentTelemetryInterval/NX_IP_PERIODIC_RATE,DOUBLE_DECIMAL_PLACE_DIGITS))
                                                                 
  {
    status = NX_NOT_SUCCESSFUL;
  }
  else
  {
    status = NX_AZURE_IOT_SUCCESS;
  }
  
  return(status);
}


UINT StdComponent_report_all_properties(STD_COMPONENT *handle,UCHAR *component_name_ptr, UINT component_name_len,
                                                 NX_AZURE_IOT_PNP_CLIENT *iotpnp_client_ptr)
{
  UINT status;
  UINT response_status = 0;
  NX_AZURE_IOT_JSON_WRITER json_writer;
  
  if ((status = nx_azure_iot_pnp_client_reported_properties_create(iotpnp_client_ptr,
                                                                   &json_writer, NX_WAIT_FOREVER)))
  {
    AZURE_PRINTF("Failed create reported properties: error code = 0x%08x\r\n", status);
    return(status);
  }
  
  if ((status = nx_azure_iot_pnp_client_reported_property_component_begin(iotpnp_client_ptr,
                                                                          &json_writer,
                                                                          component_name_ptr,
                                                                          component_name_len)) ||
      (status = append_properties(handle,&json_writer)) ||
        (status = nx_azure_iot_pnp_client_reported_property_component_end(iotpnp_client_ptr,
                                                                          &json_writer)))
  {
    AZURE_PRINTF("Failed to build reported property!: error code = 0x%08x\r\n", status);
    nx_azure_iot_json_writer_deinit(&json_writer);
    return(status);
  }
  
  if ((status = nx_azure_iot_pnp_client_reported_properties_send(iotpnp_client_ptr,
                                                                 &json_writer,
                                                                 NX_NULL, &response_status,
                                                                 NX_NULL,
                                                                 handle->SendIntervalSec)))
  {
    AZURE_PRINTF("Reported properties send failed!: error code = 0x%08x\r\n", status);
    nx_azure_iot_json_writer_deinit(&json_writer);
    return(status);
  }
  
  nx_azure_iot_json_writer_deinit(&json_writer);
  
  if ((response_status < 200) || (response_status >= 300))
  {
    AZURE_PRINTF("Reported properties send failed with code : %d\r\n", response_status);
    return(NX_NOT_SUCCESSFUL);
  }
  
  return(status);
}

VOID sample_send_target_StdComponent_acc_fullscale_report(STD_COMPONENT *handle,
                                                    NX_AZURE_IOT_PNP_CLIENT *iotpnp_client_ptr, int32_t Value,
                                                    INT status_code, UINT version, const CHAR *description)
{
  UINT response_status;
  NX_AZURE_IOT_JSON_WRITER json_writer;
  
  if (nx_azure_iot_pnp_client_reported_properties_create(iotpnp_client_ptr,
                                                         &json_writer, NX_WAIT_FOREVER)) {
                                                           AZURE_PRINTF("Failed to build reported response\r\n");
                                                           return;
                                                         }
  
  if (nx_azure_iot_pnp_client_reported_property_component_begin(iotpnp_client_ptr, &json_writer,
                                                                handle ->component_name_ptr,
                                                                handle -> component_name_length) ||
      nx_azure_iot_pnp_client_reported_property_status_begin(iotpnp_client_ptr,
                                                             &json_writer, (UCHAR *)target_StdComponent_acc_fullscale_property_name,
                                                             sizeof(target_StdComponent_acc_fullscale_property_name) - 1,
                                                             status_code, version,
                                                             (const UCHAR *)description, strlen(description)) ||
        nx_azure_iot_json_writer_append_int32(&json_writer,Value) ||
          //nx_azure_iot_json_writer_append_bool(&json_writer,Value) ||
          //nx_azure_iot_json_writer_append_double(&json_writer,
          //                                       Value, DOUBLE_DECIMAL_PLACE_DIGITS) ||
          nx_azure_iot_pnp_client_reported_property_status_end(iotpnp_client_ptr, &json_writer) ||
            nx_azure_iot_pnp_client_reported_property_component_end(iotpnp_client_ptr, &json_writer)) {
              nx_azure_iot_json_writer_deinit(&json_writer);
              AZURE_PRINTF("Failed to build reported response\r\n");
            } else {
              if (nx_azure_iot_pnp_client_reported_properties_send(iotpnp_client_ptr,
                                                                   &json_writer, NX_NULL,
                                                                   &response_status, NX_NULL,
                                                                   handle->SendIntervalSec)) {
                                                                     AZURE_PRINTF("Failed to send reported response\r\n");
                                                                   }
              
              nx_azure_iot_json_writer_deinit(&json_writer);
            }
}


VOID sample_send_target_StdComponent_mag_fullscale_report(STD_COMPONENT *handle,
                                                    NX_AZURE_IOT_PNP_CLIENT *iotpnp_client_ptr, int32_t Value,
                                                    INT status_code, UINT version, const CHAR *description)
{
  UINT response_status;
  NX_AZURE_IOT_JSON_WRITER json_writer;
  
  if (nx_azure_iot_pnp_client_reported_properties_create(iotpnp_client_ptr,
                                                         &json_writer, NX_WAIT_FOREVER)) {
                                                           AZURE_PRINTF("Failed to build reported response\r\n");
                                                           return;
                                                         }
  
  if (nx_azure_iot_pnp_client_reported_property_component_begin(iotpnp_client_ptr, &json_writer,
                                                                handle ->component_name_ptr,
                                                                handle -> component_name_length) ||
      nx_azure_iot_pnp_client_reported_property_status_begin(iotpnp_client_ptr,
                                                             &json_writer, (UCHAR *)target_StdComponent_mag_fullscale_property_name,
                                                             sizeof(target_StdComponent_mag_fullscale_property_name) - 1,
                                                             status_code, version,
                                                             (const UCHAR *)description, strlen(description)) ||
        nx_azure_iot_json_writer_append_int32(&json_writer,Value) ||
          //nx_azure_iot_json_writer_append_bool(&json_writer,Value) ||
          //nx_azure_iot_json_writer_append_double(&json_writer,
          //                                       Value, DOUBLE_DECIMAL_PLACE_DIGITS) ||
          nx_azure_iot_pnp_client_reported_property_status_end(iotpnp_client_ptr, &json_writer) ||
            nx_azure_iot_pnp_client_reported_property_component_end(iotpnp_client_ptr, &json_writer)) {
              nx_azure_iot_json_writer_deinit(&json_writer);
              AZURE_PRINTF("Failed to build reported response\r\n");
            } else {
              if (nx_azure_iot_pnp_client_reported_properties_send(iotpnp_client_ptr,
                                                                   &json_writer, NX_NULL,
                                                                   &response_status, NX_NULL,
                                                                   handle->SendIntervalSec)) {
                                                                     AZURE_PRINTF("Failed to send reported response\r\n");
                                                                   }
              
              nx_azure_iot_json_writer_deinit(&json_writer);
            }
}

VOID sample_send_target_StdComponent_gyro_fullscale_report(STD_COMPONENT *handle,
                                                    NX_AZURE_IOT_PNP_CLIENT *iotpnp_client_ptr, int32_t Value,
                                                    INT status_code, UINT version, const CHAR *description)
{
  UINT response_status;
  NX_AZURE_IOT_JSON_WRITER json_writer;
  
  if (nx_azure_iot_pnp_client_reported_properties_create(iotpnp_client_ptr,
                                                         &json_writer, NX_WAIT_FOREVER)) {
                                                           AZURE_PRINTF("Failed to build reported response\r\n");
                                                           return;
                                                         }
  
  if (nx_azure_iot_pnp_client_reported_property_component_begin(iotpnp_client_ptr, &json_writer,
                                                                handle ->component_name_ptr,
                                                                handle -> component_name_length) ||
      nx_azure_iot_pnp_client_reported_property_status_begin(iotpnp_client_ptr,
                                                             &json_writer, (UCHAR *)target_StdComponent_gryo_fullscale_property_name,
                                                             sizeof(target_StdComponent_gryo_fullscale_property_name) - 1,
                                                             status_code, version,
                                                             (const UCHAR *)description, strlen(description)) ||
        nx_azure_iot_json_writer_append_int32(&json_writer,Value) ||
          //nx_azure_iot_json_writer_append_bool(&json_writer,Value) ||
          //nx_azure_iot_json_writer_append_double(&json_writer,
          //                                       Value, DOUBLE_DECIMAL_PLACE_DIGITS) ||
          nx_azure_iot_pnp_client_reported_property_status_end(iotpnp_client_ptr, &json_writer) ||
            nx_azure_iot_pnp_client_reported_property_component_end(iotpnp_client_ptr, &json_writer)) {
              nx_azure_iot_json_writer_deinit(&json_writer);
              AZURE_PRINTF("Failed to build reported response\r\n");
            } else {
              if (nx_azure_iot_pnp_client_reported_properties_send(iotpnp_client_ptr,
                                                                   &json_writer, NX_NULL,
                                                                   &response_status, NX_NULL,
                                                                   handle->SendIntervalSec)) {
                                                                     AZURE_PRINTF("Failed to send reported response\r\n");
                                                                   }
              
              nx_azure_iot_json_writer_deinit(&json_writer);
            }
}

VOID sample_send_target_StdComponent_telemetry_interval_report(STD_COMPONENT *handle,
                                                    NX_AZURE_IOT_PNP_CLIENT *iotpnp_client_ptr, double Value,
                                                    UINT status_code, UINT version, const CHAR *description)
{
  UINT response_status;
  NX_AZURE_IOT_JSON_WRITER json_writer;
  
  if (nx_azure_iot_pnp_client_reported_properties_create(iotpnp_client_ptr,
                                                         &json_writer, NX_WAIT_FOREVER)) {
                                                           AZURE_PRINTF("Failed to build reported response\r\n");
                                                           return;
                                                         }
  
  if (nx_azure_iot_pnp_client_reported_property_component_begin(iotpnp_client_ptr, &json_writer,
                                                                handle ->component_name_ptr,
                                                                handle -> component_name_length) ||
      nx_azure_iot_pnp_client_reported_property_status_begin(iotpnp_client_ptr,
                                                             &json_writer, (UCHAR *)target_StdComponent_telemetry_interval_property_name,
                                                             sizeof(target_StdComponent_telemetry_interval_property_name) - 1,
                                                             status_code, version,
                                                             (const UCHAR *)description, strlen(description)) ||
        //nx_azure_iot_json_writer_append_int32(&json_writer,Value) ||
          //nx_azure_iot_json_writer_append_bool(&json_writer,Value) ||
          nx_azure_iot_json_writer_append_double(&json_writer,
                                                 Value, DOUBLE_DECIMAL_PLACE_DIGITS) ||
          nx_azure_iot_pnp_client_reported_property_status_end(iotpnp_client_ptr, &json_writer) ||
            nx_azure_iot_pnp_client_reported_property_component_end(iotpnp_client_ptr, &json_writer)) {
              nx_azure_iot_json_writer_deinit(&json_writer);
              AZURE_PRINTF("Failed to build reported response\r\n");
            } else {
              if (nx_azure_iot_pnp_client_reported_properties_send(iotpnp_client_ptr,
                                                                   &json_writer, NX_NULL,
                                                                   &response_status, NX_NULL,
                                                                   handle->SendIntervalSec)) {
                                                                     AZURE_PRINTF("Failed to send reported response\r\n");
                                                                   }
              
              nx_azure_iot_json_writer_deinit(&json_writer);
            }
}

UINT StdComponent_init(STD_COMPONENT *handle,
                            UCHAR *component_name_ptr, UINT component_name_length)
{
  float Odr;
  int32_t Fullscale;
  
  if (handle == NX_NULL)
  {
    return(NX_NOT_SUCCESSFUL);
  }
  
  handle -> component_name_ptr = component_name_ptr;
  handle -> component_name_length = component_name_length;
  
  handle -> Acc_X = 0.0;
  handle -> Acc_Y = 0.0;
  handle -> Acc_Z = 0.0;
  
  handle -> Gyro_X = 0.0;
  handle -> Gyro_Y = 0.0;
  handle -> Gyro_Z = 0.0;
  
  handle -> currentAccFS  = STD_COMP_ACC_FS_4G;
  Fullscale =4;
  BSP_MOTION_SENSOR_SetFullScale(ISM330DHCX_0,MOTION_ACCELERO, Fullscale);
  handle -> currentGyroFS  = STD_COMP_GYRO_FS_500_DPS;
  Fullscale=500;
  BSP_MOTION_SENSOR_SetFullScale(ISM330DHCX_0,MOTION_GYRO, Fullscale);
  
  Odr=52.0;
  BSP_MOTION_SENSOR_SetOutputDataRate(ISM330DHCX_0,MOTION_ACCELERO,Odr);
  BSP_MOTION_SENSOR_SetOutputDataRate(ISM330DHCX_0,MOTION_GYRO,Odr);
  
  handle -> currentMagFS  = STD_COMP_MAG_FS_16GAUSS;//fake
  //The fullScale of the Magneto is fixed to 50
  Fullscale=50;
  BSP_MOTION_SENSOR_SetFullScale(IIS2MDC_0, MOTION_MAGNETO,Fullscale);
  Odr=50;
  BSP_MOTION_SENSOR_SetOutputDataRate(IIS2MDC_0, MOTION_MAGNETO,Odr);
  
  
  handle -> Mag_X = 0.0;
  handle -> Mag_Y = 0.0;
  handle -> Mag_Z = 0.0;
  
  handle->Humidity = 0.0;
  handle->Temperature = 0.0;
  Odr = 12.5;
  BSP_ENV_SENSOR_SetOutputDataRate(HTS221_0, ENV_TEMPERATURE, Odr);
  BSP_ENV_SENSOR_SetOutputDataRate(HTS221_0, ENV_HUMIDITY, Odr);
  
  
  handle->Pressure = 0.0;
  Odr = 50.0;
  BSP_ENV_SENSOR_SetOutputDataRate(LPS22HH_0,ENV_PRESSURE, Odr);
  
  handle -> CurrentTelemetryInterval= 5.0*NX_IP_PERIODIC_RATE;
  handle->SendIntervalSec = 5*NX_IP_PERIODIC_RATE;
  
  handle -> ReceivedTelemetyInterval =0;
  handle -> ReceivedDesiredAccFS     =0;
  handle -> ReceivedDesiredMagFS     =0;
  handle -> ReceivedDesiredGyroFS    =0;
  
  return(NX_AZURE_IOT_SUCCESS);
}

UINT StdComponent_telemetry_send(STD_COMPONENT *handle, NX_AZURE_IOT_PNP_CLIENT *iotpnp_client_ptr)
{
  UINT status;
  NX_PACKET *packet_ptr;
  NX_AZURE_IOT_JSON_WRITER json_writer;
  UINT buffer_length;
  
  UINT BuildMessageStatus= NX_AZURE_IOT_SUCCESS;
  
  BSP_MOTION_SENSOR_Axes_t MOTION_Value;
  float SensorValue;
  
  if (handle == NX_NULL)
  {
    return(NX_NOT_SUCCESSFUL);
  }
  
  /* Create a telemetry message packet. */
  if ((status = nx_azure_iot_pnp_client_telemetry_message_create(iotpnp_client_ptr, handle -> component_name_ptr,
                                                                 handle -> component_name_length,
                                                                 &packet_ptr, NX_WAIT_FOREVER)))
  {
    AZURE_PRINTF("Telemetry message create failed!: error code = 0x%08x\r\n", status);
    return(status);
  }
  
  /* Build telemetry JSON payload */
  if (nx_azure_iot_json_writer_with_buffer_init(&json_writer, scratch_buffer, sizeof(scratch_buffer)))
  {
    AZURE_PRINTF("Telemetry message failed to build message\r\n");
    nx_azure_iot_pnp_client_telemetry_message_delete(packet_ptr);
    return(NX_NOT_SUCCESSFUL);
  }
  
  /* Read the Acc values */
  BSP_MOTION_SENSOR_GetAxes(ISM330DHCX_0,MOTION_ACCELERO,&MOTION_Value);
  handle -> Acc_X= (double)MOTION_Value.x;
  handle -> Acc_Y= (double)MOTION_Value.y;
  handle -> Acc_Z= (double)MOTION_Value.z;
  
  /* Appends the beginning of the JSON object (i.e. `{`) */
  if(nx_azure_iot_json_writer_append_begin_object (&json_writer))
    BuildMessageStatus= NX_NOT_SUCCESSFUL;
  
  /* Appends the property name as a JSON string */
  if(BuildMessageStatus!= NX_NOT_SUCCESSFUL) {
    if(nx_azure_iot_json_writer_append_property_name(&json_writer,
                                                     (UCHAR *)StdComp_acc_value_telemetry_name,
                                                     sizeof(StdComp_acc_value_telemetry_name) - 1))
      BuildMessageStatus= NX_NOT_SUCCESSFUL;
  }
  
  /* Appends the beginning of the JSON object (i.e. `{`) */
  if(BuildMessageStatus!= NX_NOT_SUCCESSFUL) {
    if(nx_azure_iot_json_writer_append_begin_object (&json_writer))
      BuildMessageStatus= NX_NOT_SUCCESSFUL;
  }
  
  /* Appends the objects name and related values in double format */
  if(BuildMessageStatus!= NX_NOT_SUCCESSFUL) {
    if(nx_azure_iot_json_writer_append_property_with_double_value(&json_writer,
                                                                  (UCHAR *)"a_x",
                                                                  sizeof("a_x") - 1,
                                                                  handle -> Acc_X,
                                                                  DOUBLE_DECIMAL_PLACE_DIGITS))
      BuildMessageStatus= NX_NOT_SUCCESSFUL;
  }
  
  if(BuildMessageStatus!= NX_NOT_SUCCESSFUL) {
    if(nx_azure_iot_json_writer_append_property_with_double_value(&json_writer,
                                                                  (UCHAR *)"a_y",
                                                                  sizeof("a_y") - 1,
                                                                  handle -> Acc_Y,
                                                                  DOUBLE_DECIMAL_PLACE_DIGITS))
      BuildMessageStatus= NX_NOT_SUCCESSFUL;
  }
  
  if(BuildMessageStatus!= NX_NOT_SUCCESSFUL) {
    if(nx_azure_iot_json_writer_append_property_with_double_value(&json_writer,
                                                                  (UCHAR *)"a_z",
                                                                  sizeof("a_z") - 1,
                                                                  handle -> Acc_Z,
                                                                  DOUBLE_DECIMAL_PLACE_DIGITS))
      BuildMessageStatus= NX_NOT_SUCCESSFUL;
  }
  
  /* Appends the end of the current JSON object (i.e. `}`) */
  if(BuildMessageStatus!= NX_NOT_SUCCESSFUL) {
    if(nx_azure_iot_json_writer_append_end_object(&json_writer))
      BuildMessageStatus= NX_NOT_SUCCESSFUL;
  }
  
   /* Read the gyro values */
  BSP_MOTION_SENSOR_GetAxes(ISM330DHCX_0,MOTION_GYRO,&MOTION_Value);
  handle -> Gyro_X= (double)MOTION_Value.x;
  handle -> Gyro_Y= (double)MOTION_Value.y;
  handle -> Gyro_Z= (double)MOTION_Value.z;
  
  /* Appends the property name as a JSON string */
  if(BuildMessageStatus!= NX_NOT_SUCCESSFUL) {
    if(nx_azure_iot_json_writer_append_property_name(&json_writer,
                                                     (UCHAR *)StdComp_gryo_value_telemetry_name,
                                                     sizeof(StdComp_gryo_value_telemetry_name) - 1))
      BuildMessageStatus= NX_NOT_SUCCESSFUL;
  }
  
  /* Appends the beginning of the JSON object (i.e. `{`) */
  if(BuildMessageStatus!= NX_NOT_SUCCESSFUL) {
    if(nx_azure_iot_json_writer_append_begin_object (&json_writer))
      BuildMessageStatus= NX_NOT_SUCCESSFUL;
  }
  
  /* Appends the objects name and related values in double format */
  if(BuildMessageStatus!= NX_NOT_SUCCESSFUL) {
    if(nx_azure_iot_json_writer_append_property_with_double_value(&json_writer,
                                                                  (UCHAR *)"g_x",
                                                                  sizeof("g_x") - 1,
                                                                  handle -> Gyro_X,
                                                                  DOUBLE_DECIMAL_PLACE_DIGITS))
      BuildMessageStatus= NX_NOT_SUCCESSFUL;
  }
  
  if(BuildMessageStatus!= NX_NOT_SUCCESSFUL) {
    if(nx_azure_iot_json_writer_append_property_with_double_value(&json_writer,
                                                                  (UCHAR *)"g_y",
                                                                  sizeof("g_y") - 1,
                                                                  handle -> Gyro_Y,
                                                                  DOUBLE_DECIMAL_PLACE_DIGITS))
      BuildMessageStatus= NX_NOT_SUCCESSFUL;
  }
  
  if(BuildMessageStatus!= NX_NOT_SUCCESSFUL) {
    if(nx_azure_iot_json_writer_append_property_with_double_value(&json_writer,
                                                                  (UCHAR *)"g_z",
                                                                  sizeof("g_z") - 1,
                                                                  handle -> Gyro_Z,
                                                                  DOUBLE_DECIMAL_PLACE_DIGITS))
      BuildMessageStatus= NX_NOT_SUCCESSFUL;
  }
  
  /* Appends the end of the current JSON object (i.e. `}`) */
  if(BuildMessageStatus!= NX_NOT_SUCCESSFUL) {
    if(nx_azure_iot_json_writer_append_end_object(&json_writer))
      BuildMessageStatus= NX_NOT_SUCCESSFUL;
  }
  
   /* Read the Mag values */
  BSP_MOTION_SENSOR_GetAxes(IIS2MDC_0,MOTION_MAGNETO,&MOTION_Value);
  handle -> Mag_X= (double)MOTION_Value.x;
  handle -> Mag_Y= (double)MOTION_Value.y;
  handle -> Mag_Z= (double)MOTION_Value.z;

  
  /* Appends the property name as a JSON string */
  if(BuildMessageStatus!= NX_NOT_SUCCESSFUL) {
    if(nx_azure_iot_json_writer_append_property_name(&json_writer,
                                                     (UCHAR *)StdComp_mag_value_telemetry_name,
                                                     sizeof(StdComp_mag_value_telemetry_name) - 1))
      BuildMessageStatus= NX_NOT_SUCCESSFUL;
  }
  
  /* Appends the beginning of the JSON object (i.e. `{`) */
  if(BuildMessageStatus!= NX_NOT_SUCCESSFUL) {
    if(nx_azure_iot_json_writer_append_begin_object (&json_writer))
      BuildMessageStatus= NX_NOT_SUCCESSFUL;
  }
  
  /* Appends the objects name and related values in double format */
  if(BuildMessageStatus!= NX_NOT_SUCCESSFUL) {
    if(nx_azure_iot_json_writer_append_property_with_double_value(&json_writer,
                                                                  (UCHAR *)"m_x",
                                                                  sizeof("m_x") - 1,
                                                                  handle -> Mag_X,
                                                                  DOUBLE_DECIMAL_PLACE_DIGITS))
      BuildMessageStatus= NX_NOT_SUCCESSFUL;
  }
  
  if(BuildMessageStatus!= NX_NOT_SUCCESSFUL) {
    if(nx_azure_iot_json_writer_append_property_with_double_value(&json_writer,
                                                                  (UCHAR *)"m_y",
                                                                  sizeof("m_y") - 1,
                                                                  handle -> Mag_Y,
                                                                  DOUBLE_DECIMAL_PLACE_DIGITS))
      BuildMessageStatus= NX_NOT_SUCCESSFUL;
  }
  
  if(BuildMessageStatus!= NX_NOT_SUCCESSFUL) {
    if(nx_azure_iot_json_writer_append_property_with_double_value(&json_writer,
                                                                  (UCHAR *)"m_z",
                                                                  sizeof("m_z") - 1,
                                                                  handle -> Mag_Z,
                                                                  DOUBLE_DECIMAL_PLACE_DIGITS))
      BuildMessageStatus= NX_NOT_SUCCESSFUL;
  }
  
  /* Appends the end of the current JSON object (i.e. `}`) */
  if(BuildMessageStatus!= NX_NOT_SUCCESSFUL) {
    if(nx_azure_iot_json_writer_append_end_object(&json_writer))
      BuildMessageStatus= NX_NOT_SUCCESSFUL;
  }
  
  /* Read temperature value */
  BSP_ENV_SENSOR_GetValue(HTS221_0,ENV_TEMPERATURE,&SensorValue);
  handle -> Temperature= (double)SensorValue;
  
  /* Read humidity value */
  BSP_ENV_SENSOR_GetValue(HTS221_0,ENV_HUMIDITY,&SensorValue);
  handle -> Humidity= (double)SensorValue;
  
   /* Read pressure value */
  BSP_ENV_SENSOR_GetValue(LPS22HH_0,ENV_PRESSURE,&SensorValue);
  handle -> Pressure= (double)SensorValue;
  
  if(BuildMessageStatus!= NX_NOT_SUCCESSFUL) {
    if(nx_azure_iot_json_writer_append_property_with_double_value(&json_writer,
                                                                  (UCHAR *)StdComp_temp_value_telemetry_name,
                                                                  sizeof(StdComp_temp_value_telemetry_name) - 1,
                                                                  handle -> Temperature,
                                                                  DOUBLE_DECIMAL_PLACE_DIGITS) ||
         nx_azure_iot_json_writer_append_property_with_double_value(&json_writer,
                                                                    (UCHAR *)StdComp_hum_value_telemetry_name,
                                                                    sizeof(StdComp_hum_value_telemetry_name) - 1,
                                                                    handle -> Humidity,
                                                                    DOUBLE_DECIMAL_PLACE_DIGITS) ||   
         nx_azure_iot_json_writer_append_property_with_double_value(&json_writer,
                                                                    (UCHAR *)StdComp_press_value_telemetry_name,
                                                                    sizeof(StdComp_press_value_telemetry_name) - 1,
                                                                    handle -> Pressure,
                                                                    DOUBLE_DECIMAL_PLACE_DIGITS))
    {
      BuildMessageStatus= NX_NOT_SUCCESSFUL;
    }
  }
 
  
  /* Appends the end of the current JSON object (i.e. `}`) */
  if(BuildMessageStatus!= NX_NOT_SUCCESSFUL) {
    if(nx_azure_iot_json_writer_append_end_object(&json_writer))
      BuildMessageStatus= NX_NOT_SUCCESSFUL;
  }
  
  if(BuildMessageStatus != NX_AZURE_IOT_SUCCESS)
  {
    AZURE_PRINTF("Telemetry message failed to build message for StdComp accelerometer sensor\r\n");
    nx_azure_iot_json_writer_deinit(&json_writer);
    nx_azure_iot_pnp_client_telemetry_message_delete(packet_ptr);
    return(NX_NOT_SUCCESSFUL);
  }
  
  buffer_length = nx_azure_iot_json_writer_get_bytes_used(&json_writer);
  if ((status = nx_azure_iot_pnp_client_telemetry_send(iotpnp_client_ptr, packet_ptr,
                                                       (UCHAR *)scratch_buffer, buffer_length, NX_WAIT_FOREVER)))
  {
    AZURE_PRINTF("STD_COMP Accelerometer telemetry message send failed!: error code = 0x%08x\r\n", status);
    nx_azure_iot_json_writer_deinit(&json_writer);
    nx_azure_iot_pnp_client_telemetry_message_delete(packet_ptr);
    return(status);
  }
  
  nx_azure_iot_json_writer_deinit(&json_writer);
  AZURE_PRINTF("\r\n");
  AZURE_PRINTF("STD_COMP Accelerometer Telemetry message send:\r\n");
  AZURE_PRINTF("\t- Component %.*s\r\n", handle -> component_name_length, handle -> component_name_ptr);
  AZURE_PRINTF("\t- Message: %.*s\r\n", buffer_length, scratch_buffer);
  
  return(status);
}


UINT StdComponent_process_property_update(STD_COMPONENT *handle,
                                                      NX_AZURE_IOT_PNP_CLIENT *iotpnp_client_ptr,
                                                      const UCHAR *component_name_ptr, UINT component_name_length,
                                                      NX_AZURE_IOT_JSON_READER *name_value_reader_ptr, UINT version)
{
  int32_t int32_parsed_value = 0;
  double double_parsed_value;
  UINT status_code = 200;
  const CHAR *description = response_description_success;
  
  if (handle == NX_NULL)
  {
    return(NX_NOT_SUCCESSFUL);
  }
  
  if (handle -> component_name_length != component_name_length ||
      strncmp((CHAR *)handle -> component_name_ptr, (CHAR *)component_name_ptr, component_name_length) != 0)
  {
    return(NX_NOT_SUCCESSFUL);
  }
  
  if ((nx_azure_iot_json_reader_token_is_text_equal(name_value_reader_ptr,
                                                   (UCHAR *)target_StdComponent_acc_fullscale_property_name,
                                                   sizeof(target_StdComponent_acc_fullscale_property_name) - 1) == NX_FALSE) &&
      (nx_azure_iot_json_reader_token_is_text_equal(name_value_reader_ptr,
                                                   (UCHAR *)target_StdComponent_gryo_fullscale_property_name,
                                                   sizeof(target_StdComponent_gryo_fullscale_property_name) - 1) == NX_FALSE) &&
        (nx_azure_iot_json_reader_token_is_text_equal(name_value_reader_ptr,
                                                   (UCHAR *)target_StdComponent_mag_fullscale_property_name,
                                                   sizeof(target_StdComponent_mag_fullscale_property_name) - 1) == NX_FALSE) &&
        (nx_azure_iot_json_reader_token_is_text_equal(name_value_reader_ptr,
                                                   (UCHAR *)target_StdComponent_telemetry_interval_property_name,
                                                   sizeof(target_StdComponent_telemetry_interval_property_name) - 1) == NX_FALSE))
      
  {
    AZURE_PRINTF("Unknown property for component %.*s received\r\n", component_name_length, component_name_ptr);
    status_code = 404;
    description = response_description_failed;
  }
  else
  {
    if (nx_azure_iot_json_reader_token_is_text_equal(name_value_reader_ptr,
                                                     (UCHAR *)target_StdComponent_acc_fullscale_property_name,
                                                     sizeof(target_StdComponent_acc_fullscale_property_name) - 1) == NX_TRUE)
    {
      if (nx_azure_iot_json_reader_next_token(name_value_reader_ptr) ||
          nx_azure_iot_json_reader_token_int32_get(name_value_reader_ptr, &int32_parsed_value))
      {
        status_code = 401;
        description = response_description_failed;
      }
      else {
        int32_t Fullscale;
        handle -> currentAccFS = (STD_COMP_AccFullScaleTypeDef)int32_parsed_value;
        AZURE_PRINTF("Received currentAccFS=%d\r\n",int32_parsed_value);
        switch( handle -> currentAccFS) {
        case STD_COMP_ACC_FS_2G:
          Fullscale=2;
          break;
        case STD_COMP_ACC_FS_4G:
          Fullscale=4;
          break;
        case STD_COMP_ACC_FS_8G:
          Fullscale=8;
          break;
        case STD_COMP_ACC_FS_16G:
          Fullscale=16;
          break;
        }
        BSP_MOTION_SENSOR_SetFullScale(ISM330DHCX_0, MOTION_ACCELERO, Fullscale);
        handle -> ReceivedDesiredAccFS=1;
      }
      
      sample_send_target_StdComponent_acc_fullscale_report(handle, iotpnp_client_ptr, int32_parsed_value, status_code, version, description);
    } else if (nx_azure_iot_json_reader_token_is_text_equal(name_value_reader_ptr,
                                                     (UCHAR *)target_StdComponent_gryo_fullscale_property_name,
                                                     sizeof(target_StdComponent_gryo_fullscale_property_name) - 1) == NX_TRUE)
    {
      if (nx_azure_iot_json_reader_next_token(name_value_reader_ptr) ||
          nx_azure_iot_json_reader_token_int32_get(name_value_reader_ptr, &int32_parsed_value))
      {
        status_code = 401;
        description = response_description_failed;
      }
      else {
        int32_t Fullscale;
        handle -> currentGyroFS = (STD_COMP_GyroFullScaleTypeDef)int32_parsed_value;
        AZURE_PRINTF("Received currentGyroFS=%d\r\n",int32_parsed_value);
        switch(handle -> currentGyroFS) {
        case STD_COMP_GYRO_FS_125_DPS:
          Fullscale=125;
          break;
        case STD_COMP_GYRO_FS_250_DPS:
          Fullscale=250;
          break;
        case STD_COMP_GYRO_FS_500_DPS:
          Fullscale=500;
          break;
        case STD_COMP_GYRO_FS_1000_DPS:
          Fullscale=1000;
          break;
        case STD_COMP_GYRO_FS_2000_DPS:
          Fullscale=2000;
          break;
        }
        BSP_MOTION_SENSOR_SetFullScale(ISM330DHCX_0,MOTION_GYRO, Fullscale);
        handle -> ReceivedDesiredGyroFS=1;
      }
      
      sample_send_target_StdComponent_gyro_fullscale_report(handle, iotpnp_client_ptr, int32_parsed_value, status_code, version, description);
    } else  if (nx_azure_iot_json_reader_token_is_text_equal(name_value_reader_ptr,
                                                     (UCHAR *)target_StdComponent_mag_fullscale_property_name,
                                                     sizeof(target_StdComponent_mag_fullscale_property_name) - 1) == NX_TRUE)
    {
      if (nx_azure_iot_json_reader_next_token(name_value_reader_ptr) ||
          nx_azure_iot_json_reader_token_int32_get(name_value_reader_ptr, &int32_parsed_value))
      {
        status_code = 401;
        description = response_description_failed;
      }
      else {
        int32_t Fullscale;
        handle -> currentMagFS = (STD_COMP_MagFullScaleTypeDef)int32_parsed_value;
        AZURE_PRINTF("Received currentMagFS=%d\r\n",int32_parsed_value);
        switch( handle -> currentMagFS) {
        case STD_COMP_MAG_FS_4GAUSS:
          Fullscale=4;
          break;
        case STD_COMP_MAG_FS_8GAUSS:
          Fullscale=8;
          break;
        case STD_COMP_MAG_FS_12GAUSS:
          Fullscale=12;
          break;
        case STD_COMP_MAG_FS_16GAUSS:
          Fullscale=16;
          break;
        }
        Fullscale=50;//the Fullscale for IIS2MDC is fixed to 50
        BSP_MOTION_SENSOR_SetFullScale(IIS2MDC_0, MOTION_MAGNETO,Fullscale);
        handle -> ReceivedDesiredMagFS=1;
      }
      
      sample_send_target_StdComponent_mag_fullscale_report(handle, iotpnp_client_ptr, int32_parsed_value, status_code, version, description);
    } else  if (nx_azure_iot_json_reader_token_is_text_equal(name_value_reader_ptr,
                                                     (UCHAR *)target_StdComponent_telemetry_interval_property_name,
                                                     sizeof(target_StdComponent_telemetry_interval_property_name) - 1) == NX_TRUE)
    {
      if (nx_azure_iot_json_reader_next_token(name_value_reader_ptr) ||
          nx_azure_iot_json_reader_token_double_get(name_value_reader_ptr, &double_parsed_value))
      {
        status_code = 401;
        description = response_description_failed;
        AZURE_PRINTF("Received Telemetry Intervall ERROR\r\n");
      }
      else {
        handle -> CurrentTelemetryInterval = double_parsed_value*NX_IP_PERIODIC_RATE;
        handle -> SendIntervalSec = (int) handle -> CurrentTelemetryInterval;
        AZURE_PRINTF("Received Telemetry Intervall=%f\r\n",double_parsed_value);
        handle -> ReceivedTelemetyInterval=1;
      }
      
      sample_send_target_StdComponent_telemetry_interval_report(handle, iotpnp_client_ptr, double_parsed_value, status_code, version, description);
    }
    
  }
  
  return(NX_AZURE_IOT_SUCCESS);
}
