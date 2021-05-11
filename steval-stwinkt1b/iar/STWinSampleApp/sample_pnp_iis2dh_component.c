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
#include "sample_pnp_iis2dh_component.h"
#include "STWIN_env_sensors.h"
#include "STWIN_motion_sensors.h"
#include "sample_pnp_common_defines.h"

/* exported variables  */
UINT iis2dh_property_sent;

static UCHAR scratch_buffer[512];

/* Telemetry key */
static const CHAR iis2dh_acc_value_telemetry_name[] = "iis2dh_acc_value";

/* Property Names  */
static const CHAR target_iis2dh_acc_enable_property_name[]    = "iis2dh_acc_enable";
static const CHAR target_iis2dh_acc_odr_property_name[]       = "iis2dh_acc_odr";
static const CHAR target_iis2dh_acc_fullscale_property_name[] = "iis2dh_acc_fullscale";

static UINT append_properties(SAMPLE_PNP_IIS2DH_COMPONENT *handle,NX_AZURE_IOT_JSON_WRITER *json_writer)
{
  UINT status;
  
  if (nx_azure_iot_json_writer_append_property_with_bool_value(json_writer,
                                                                 (UCHAR *)target_iis2dh_acc_enable_property_name,
                                                                 sizeof(target_iis2dh_acc_enable_property_name) - 1,
                                                                 handle->iis2dh_AccEnableFlag) ||
      nx_azure_iot_json_writer_append_property_with_int32_value(json_writer,
                                                                 (UCHAR *)target_iis2dh_acc_odr_property_name,
                                                                 sizeof(target_iis2dh_acc_odr_property_name) - 1,
                                                                 handle->iis2dh_currentAccODR) ||
        nx_azure_iot_json_writer_append_property_with_int32_value(json_writer,
                                                                 (UCHAR *)target_iis2dh_acc_fullscale_property_name,
                                                                 sizeof(target_iis2dh_acc_fullscale_property_name) - 1,
                                                                 handle->iis2dh_currentAccFS))
  {
    status = NX_NOT_SUCCESSFUL;
  }
  else
  {
    status = NX_AZURE_IOT_SUCCESS;
  }
  
  return(status);
}


UINT sample_pnp_iis2dh_report_all_properties(SAMPLE_PNP_IIS2DH_COMPONENT *handle,UCHAR *component_name_ptr, UINT component_name_len,
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
                                                                 (5 * NX_IP_PERIODIC_RATE))))
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

VOID sample_send_target_iis2dh_acc_enable_report(SAMPLE_PNP_IIS2DH_COMPONENT *handle,
                                                 NX_AZURE_IOT_PNP_CLIENT *iotpnp_client_ptr, UINT Value,
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
                                                             &json_writer, (UCHAR *)target_iis2dh_acc_enable_property_name,
                                                             sizeof(target_iis2dh_acc_enable_property_name) - 1,
                                                             status_code, version,
                                                             (const UCHAR *)description, strlen(description)) ||
        //nx_azure_iot_json_writer_append_int32(&json_writer,Value) ||
        nx_azure_iot_json_writer_append_bool(&json_writer,Value) ||
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
                                                                   (5 * NX_IP_PERIODIC_RATE))) {
                                                                     AZURE_PRINTF("Failed to send reported response\r\n");
                                                                   }
              
              nx_azure_iot_json_writer_deinit(&json_writer);
            }
}

VOID sample_send_target_iis2dh_acc_odr_report(SAMPLE_PNP_IIS2DH_COMPONENT *handle,
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
                                                             &json_writer, (UCHAR *)target_iis2dh_acc_odr_property_name,
                                                             sizeof(target_iis2dh_acc_odr_property_name) - 1,
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
                                                                   (5 * NX_IP_PERIODIC_RATE))) {
                                                                     AZURE_PRINTF("Failed to send reported response\r\n");
                                                                   }
              
              nx_azure_iot_json_writer_deinit(&json_writer);
            }
}

VOID sample_send_target_iis2dh_acc_fullscale_report(SAMPLE_PNP_IIS2DH_COMPONENT *handle,
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
                                                             &json_writer, (UCHAR *)target_iis2dh_acc_fullscale_property_name,
                                                             sizeof(target_iis2dh_acc_fullscale_property_name) - 1,
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
                                                                   (5 * NX_IP_PERIODIC_RATE))) {
                                                                     AZURE_PRINTF("Failed to send reported response\r\n");
                                                                   }
              
              nx_azure_iot_json_writer_deinit(&json_writer);
            }
}

UINT sample_pnp_iis2dh_init(SAMPLE_PNP_IIS2DH_COMPONENT *handle,
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
  
  handle -> iis2dh_AccEnableFlag= 1;
  
  handle -> iis2dh_currentAccFS= IIS2DH_ACC_FS_4G;
  Fullscale =4;
  BSP_MOTION_SENSOR_SetFullScale(IIS2DH_0, MOTION_ACCELERO, Fullscale);
  
  handle -> iis2dh_currentAccODR= IIS2DH_ACC_ODR_50_HZ;
  Odr=50.0;
  BSP_MOTION_SENSOR_SetOutputDataRate(IIS2DH_0, MOTION_ACCELERO, Odr);
  
  handle -> iis2dh_Acc_X = 0;
  handle -> iis2dh_Acc_Y = 0;
  handle -> iis2dh_Acc_Z = 0;
  
  handle -> ReceivedDesiredAccODR =0;
  handle -> ReceivedDesiredAccFS =0;
  handle -> ReceivedDesiredAccEnableFlag=0;
  
  return(NX_AZURE_IOT_SUCCESS);
}

UINT sample_pnp_iis2dh_telemetry_send(SAMPLE_PNP_IIS2DH_COMPONENT *handle, NX_AZURE_IOT_PNP_CLIENT *iotpnp_client_ptr)
{
  UINT status;
  NX_PACKET *packet_ptr;
  NX_AZURE_IOT_JSON_WRITER json_writer;
  UINT buffer_length;
  
  UINT BuildMessageStatus= NX_AZURE_IOT_SUCCESS;
  
  BSP_MOTION_SENSOR_Axes_t ACC_Value;
  
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
  BSP_MOTION_SENSOR_GetAxes(IIS2DH_0,MOTION_ACCELERO,&ACC_Value);
  handle -> iis2dh_Acc_X= (double)ACC_Value.x;
  handle -> iis2dh_Acc_Y= (double)ACC_Value.y;
  handle -> iis2dh_Acc_Z= (double)ACC_Value.z;
  
  /* Appends the beginning of the JSON object (i.e. `{`) */
  if(nx_azure_iot_json_writer_append_begin_object (&json_writer))
    BuildMessageStatus= NX_NOT_SUCCESSFUL;
  
  /* Appends the property name as a JSON string */
  if(BuildMessageStatus!= NX_NOT_SUCCESSFUL) {
    if(nx_azure_iot_json_writer_append_property_name(&json_writer,
                                                     (UCHAR *)iis2dh_acc_value_telemetry_name,
                                                     sizeof(iis2dh_acc_value_telemetry_name) - 1))
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
                                                                  handle -> iis2dh_Acc_X,
                                                                  DOUBLE_DECIMAL_PLACE_DIGITS))
      BuildMessageStatus= NX_NOT_SUCCESSFUL;
  }
  
  if(BuildMessageStatus!= NX_NOT_SUCCESSFUL) {
    if(nx_azure_iot_json_writer_append_property_with_double_value(&json_writer,
                                                                  (UCHAR *)"a_y",
                                                                  sizeof("a_y") - 1,
                                                                  handle -> iis2dh_Acc_Y,
                                                                  DOUBLE_DECIMAL_PLACE_DIGITS))
      BuildMessageStatus= NX_NOT_SUCCESSFUL;
  }
  
  if(BuildMessageStatus!= NX_NOT_SUCCESSFUL) {
    if(nx_azure_iot_json_writer_append_property_with_double_value(&json_writer,
                                                                  (UCHAR *)"a_z",
                                                                  sizeof("a_z") - 1,
                                                                  handle -> iis2dh_Acc_Z,
                                                                  DOUBLE_DECIMAL_PLACE_DIGITS))
      BuildMessageStatus= NX_NOT_SUCCESSFUL;
  }
  
  /* Appends the end of the current JSON object (i.e. `}`) */
  if(BuildMessageStatus!= NX_NOT_SUCCESSFUL) {
    if(nx_azure_iot_json_writer_append_end_object(&json_writer))
      BuildMessageStatus= NX_NOT_SUCCESSFUL;
  }
  
  /* Appends the end of the current JSON object (i.e. `}`) */
  if(BuildMessageStatus!= NX_NOT_SUCCESSFUL) {
    if(nx_azure_iot_json_writer_append_end_object(&json_writer))
      BuildMessageStatus= NX_NOT_SUCCESSFUL;
  }
  
  if(BuildMessageStatus != NX_AZURE_IOT_SUCCESS)
  {
    AZURE_PRINTF("Telemetry message failed to build message for iis2dh accelerometer sensor\r\n");
    nx_azure_iot_json_writer_deinit(&json_writer);
    nx_azure_iot_pnp_client_telemetry_message_delete(packet_ptr);
    return(NX_NOT_SUCCESSFUL);
  }
  
  buffer_length = nx_azure_iot_json_writer_get_bytes_used(&json_writer);
  if ((status = nx_azure_iot_pnp_client_telemetry_send(iotpnp_client_ptr, packet_ptr,
                                                       (UCHAR *)scratch_buffer, buffer_length, NX_WAIT_FOREVER)))
  {
    AZURE_PRINTF("IIS2DH Accelerometer telemetry message send failed!: error code = 0x%08x\r\n", status);
    nx_azure_iot_json_writer_deinit(&json_writer);
    nx_azure_iot_pnp_client_telemetry_message_delete(packet_ptr);
    return(status);
  }
  
  nx_azure_iot_json_writer_deinit(&json_writer);
  AZURE_PRINTF("\r\n");
  AZURE_PRINTF("IIS2DH Accelerometer Telemetry message send:\r\n");
  AZURE_PRINTF("\t- Component %.*s\r\n", handle -> component_name_length, handle -> component_name_ptr);
  AZURE_PRINTF("\t- Message: %.*s\r\n", buffer_length, scratch_buffer);
  
  return(status);
}

UINT sample_pnp_iis2dh_process_property_update(SAMPLE_PNP_IIS2DH_COMPONENT *handle,
                                                      NX_AZURE_IOT_PNP_CLIENT *iotpnp_client_ptr,
                                                      const UCHAR *component_name_ptr, UINT component_name_length,
                                                      NX_AZURE_IOT_JSON_READER *name_value_reader_ptr, UINT version)
{
  UINT uint_parsed_value = 0;
  int32_t int32_parsed_value = 0;
  INT status_code = 200;
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
                                                   (UCHAR *)target_iis2dh_acc_enable_property_name,
                                                   sizeof(target_iis2dh_acc_enable_property_name) - 1) == NX_FALSE) &&
      (nx_azure_iot_json_reader_token_is_text_equal(name_value_reader_ptr,
                                                   (UCHAR *)target_iis2dh_acc_odr_property_name,
                                                   sizeof(target_iis2dh_acc_odr_property_name) - 1) == NX_FALSE) &&
        (nx_azure_iot_json_reader_token_is_text_equal(name_value_reader_ptr,
                                                   (UCHAR *)target_iis2dh_acc_fullscale_property_name,
                                                   sizeof(target_iis2dh_acc_fullscale_property_name) - 1) == NX_FALSE))
      
  {
    AZURE_PRINTF("Unknown property for component %.*s received\r\n", component_name_length, component_name_ptr);
    status_code = 404;
    description = response_description_failed;
  }
  else
  {
    if (nx_azure_iot_json_reader_token_is_text_equal(name_value_reader_ptr,
                                                     (UCHAR *)target_iis2dh_acc_enable_property_name,
                                                     sizeof(target_iis2dh_acc_enable_property_name) - 1) == NX_TRUE)
    {
      if (nx_azure_iot_json_reader_next_token(name_value_reader_ptr) ||
          nx_azure_iot_json_reader_token_bool_get(name_value_reader_ptr, &uint_parsed_value))
      {
        status_code = 401;
        description = response_description_failed;
      }
      else {
        handle -> iis2dh_AccEnableFlag = (uint8_t)uint_parsed_value;
        handle -> ReceivedDesiredAccEnableFlag=1;
        AZURE_PRINTF("Received iis2dh_AccEnableFlag=%d\r\n",uint_parsed_value);
      }
      
      sample_send_target_iis2dh_acc_enable_report(handle, iotpnp_client_ptr, uint_parsed_value, status_code, version, description);
    } else if (nx_azure_iot_json_reader_token_is_text_equal(name_value_reader_ptr,
                                                     (UCHAR *)target_iis2dh_acc_odr_property_name,
                                                     sizeof(target_iis2dh_acc_odr_property_name) - 1) == NX_TRUE)
    {
      if (nx_azure_iot_json_reader_next_token(name_value_reader_ptr) ||
          nx_azure_iot_json_reader_token_int32_get(name_value_reader_ptr, &int32_parsed_value))
      {
        status_code = 401;
        description = response_description_failed;
      }
      else {
        float Odr;
        handle -> iis2dh_currentAccODR = (IIS2DH_AccOutputDataRateTypeDef)int32_parsed_value;
        AZURE_PRINTF("Received iis2dh_currentAccODR=%d\r\n",int32_parsed_value);
        
        switch(handle -> iis2dh_currentAccODR) {
        case IIS2DH_ACC_ODR_OFF:
          Odr=0.0;
          break;
        case IIS2DH_ACC_ODR_1_HZ:
          Odr=1.0;
          break;
        case IIS2DH_ACC_ODR_10_HZ:
          Odr=10.0;
          break;
        case IIS2DH_ACC_ODR_25_HZ:
          Odr=25.0;
          break;
        case IIS2DH_ACC_ODR_50_HZ:
          Odr=50.0;
          break;
        case IIS2DH_ACC_ODR_100_HZ:
          Odr=100.0;
          break;
        case IIS2DH_ACC_ODR_200_HZ:
          Odr=200.0;
          break;
        case IIS2DH_ACC_ODR_400_HZ:
          Odr=400.0;
          break;
        case IIS2DH_ACC_ODR_1344_HZ:
          Odr=1344.0;
          break;
        }
        BSP_MOTION_SENSOR_SetOutputDataRate(IIS2DH_0, MOTION_ACCELERO, Odr);
        
        handle -> ReceivedDesiredAccODR=1;
      }
      
      sample_send_target_iis2dh_acc_odr_report(handle, iotpnp_client_ptr, int32_parsed_value, status_code, version, description);
    } else if (nx_azure_iot_json_reader_token_is_text_equal(name_value_reader_ptr,
                                                     (UCHAR *)target_iis2dh_acc_fullscale_property_name,
                                                     sizeof(target_iis2dh_acc_fullscale_property_name) - 1) == NX_TRUE)
    {
      if (nx_azure_iot_json_reader_next_token(name_value_reader_ptr) ||
          nx_azure_iot_json_reader_token_int32_get(name_value_reader_ptr, &int32_parsed_value))
      {
        status_code = 401;
        description = response_description_failed;
      }
      else {
        int32_t Fullscale;
        handle -> iis2dh_currentAccFS = (IIS2DH_AccFullScaleTypeDef)int32_parsed_value;
        AZURE_PRINTF("Received iis2dh_currentAccFS=%d\r\n",int32_parsed_value);
        switch( handle -> iis2dh_currentAccFS) {
        case IIS2DH_ACC_FS_2G:
          Fullscale=2;
          break;
        case IIS2DH_ACC_FS_4G:
          Fullscale=4;
          break;
        case IIS2DH_ACC_FS_8G:
          Fullscale=8;
          break;
        case IIS2DH_ACC_FS_16G:
          Fullscale=16;
          break;
        }
        BSP_MOTION_SENSOR_SetFullScale(IIS2DH_0, MOTION_ACCELERO, Fullscale);
        handle -> ReceivedDesiredAccFS=1;
      }
      
      sample_send_target_iis2dh_acc_fullscale_report(handle, iotpnp_client_ptr, int32_parsed_value, status_code, version, description);
    }
    
  }
  
  return(NX_AZURE_IOT_SUCCESS);
}

