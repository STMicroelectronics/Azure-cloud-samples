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

#include <stdio.h>

#include "nx_api.h"
#include "nx_azure_iot_pnp_client.h"
#include "nx_azure_iot_provisioning_client.h"

/* These are sample files, user can build their own certificate and ciphersuites.  */
#include "nx_azure_iot_cert.h"
#include "nx_azure_iot_ciphersuites.h"
#include "sample_config.h"
#include "DeviceInfoComponent.h"

#include "StdComponent.h"
#include "SampleCommonDefine.h"

#define ENDPOINT  "global.azure-devices-provisioning.net"
//#define DEBUG_FUNCTION_CALL

/* exported variables */
const CHAR response_description_success[] = "success";
const CHAR response_description_failed[]  = "failed";

#ifndef SAMPLE_MAX_EXPONENTIAL_BACKOFF_IN_SEC
#define SAMPLE_MAX_EXPONENTIAL_BACKOFF_IN_SEC                           (10 * 60)
#endif /* SAMPLE_MAX_EXPONENTIAL_BACKOFF_IN_SEC */

#ifndef SAMPLE_INITIAL_EXPONENTIAL_BACKOFF_IN_SEC
#define SAMPLE_INITIAL_EXPONENTIAL_BACKOFF_IN_SEC                       (3)
#endif /* SAMPLE_INITIAL_EXPONENTIAL_BACKOFF_IN_SEC */

#ifndef SAMPLE_MAX_EXPONENTIAL_BACKOFF_JITTER_PERCENT
#define SAMPLE_MAX_EXPONENTIAL_BACKOFF_JITTER_PERCENT                   (60)
#endif /* SAMPLE_MAX_EXPONENTIAL_BACKOFF_JITTER_PERCENT */

#ifndef SAMPLE_WAIT_OPTION
#define SAMPLE_WAIT_OPTION                                              (NX_NO_WAIT)
#endif /* SAMPLE_WAIT_OPTION */

/* Sample events.  */

/* Sample events.  */
#define SAMPLE_ALL_EVENTS                                               ((ULONG)0xFFFFFFFF)
#define SAMPLE_CONNECT_EVENT                                            ((ULONG)0x00000001)
#define SAMPLE_INITIALIZATION_EVENT                                     ((ULONG)0x00000002)
#define SAMPLE_COMMAND_MESSAGE_EVENT                                    ((ULONG)0x00000004)
#define SAMPLE_DEVICE_PROPERTIES_GET_EVENT                              ((ULONG)0x00000008)
#define SAMPLE_DEVICE_DESIRED_PROPERTIES_EVENT                          ((ULONG)0x00000010)
#define SAMPLE_TELEMETRY_SEND_EVENT                                     ((ULONG)0x00000020)
#define SAMPLE_DEVICE_REPORTED_PROPERTIES_EVENT                         ((ULONG)0x00000040)
#define SAMPLE_DISCONNECT_EVENT                                         ((ULONG)0x00000080)
#define SAMPLE_RECONNECT_EVENT                                          ((ULONG)0x00000100)
#define SAMPLE_CONNECTED_EVENT                                          ((ULONG)0x00000200)

/* Sample states.  */
#define SAMPLE_STATE_NONE                                               (0)
#define SAMPLE_STATE_INIT                                               (1)
#define SAMPLE_STATE_CONNECTING                                         (2)
#define SAMPLE_STATE_CONNECT                                            (3)
#define SAMPLE_STATE_CONNECTED                                          (4)
#define SAMPLE_STATE_DISCONNECTED                                       (5)

#define MODULE_ID    ""
#define SAMPLE_PNP_MODEL_ID       "dtmi:stmicroelectronics:steval_stwinkt1b:standard_fw;1"
#define SAMPLE_PNP_DPS_PAYLOAD    "{\"modelId\":\"" SAMPLE_PNP_MODEL_ID "\"}"

/* Define Sample context.  */
typedef struct SAMPLE_CONTEXT_STRUCT
{
  UINT                                state;
  UINT                                action_result;
  ULONG                               last_periodic_action_tick;
  
  TX_EVENT_FLAGS_GROUP                sample_events;
  
  /* Generally, IoTHub Client and DPS Client do not run at the same time, user can use union as below to
  share the memory between IoTHub Client and DPS Client.
  
  NOTE: If user can not make sure sharing memory is safe, IoTHub Client and DPS Client must be defined seperately.  */
  union SAMPLE_CLIENT_UNION
  {
    NX_AZURE_IOT_PNP_CLIENT             iotpnp_client;
    NX_AZURE_IOT_PROVISIONING_CLIENT    prov_client;
  } client;
  
#define iotpnp_client client.iotpnp_client
#define prov_client client.prov_client
  
} SAMPLE_CONTEXT;

VOID sample_entry(NX_IP *ip_ptr, NX_PACKET_POOL *pool_ptr, NX_DNS *dns_ptr, UINT (*unix_time_callback)(ULONG *unix_time));

static UINT sample_dps_entry(NX_AZURE_IOT_PROVISIONING_CLIENT *prov_client_ptr,
                             UCHAR **iothub_hostname, UINT *iothub_hostname_length,
                             UCHAR **iothub_device_id, UINT *iothub_device_id_length);

/* Define Azure RTOS TLS info.  */
static NX_SECURE_X509_CERT root_ca_cert;
static UCHAR nx_azure_iot_tls_metadata_buffer[NX_AZURE_IOT_TLS_METADATA_BUFFER_SIZE];
static ULONG nx_azure_iot_thread_stack[NX_AZURE_IOT_STACK_SIZE / sizeof(ULONG)];

/* Define buffer for IoTHub info.  */
static UCHAR sample_iothub_hostname[SAMPLE_MAX_BUFFER];
static UCHAR sample_iothub_device_id[SAMPLE_MAX_BUFFER];

/* Define the prototypes for AZ IoT.  */
static NX_AZURE_IOT nx_azure_iot;

static SAMPLE_CONTEXT sample_context;
static volatile UINT sample_connection_status = NX_NOT_CONNECTED;
static UINT exponential_retry_count;

/* PNP model id.  */
static STD_COMPONENT sample_StdComp;
static const CHAR sample_StdComponent_component[] = "std_comp";

static const CHAR sample_device_info_component[] = "deviceinfo";
static UINT sample_device_info_sent;

static UCHAR scratch_buffer[512];

static UINT exponential_backoff_with_jitter()
{
  double jitter_percent = (SAMPLE_MAX_EXPONENTIAL_BACKOFF_JITTER_PERCENT / 100.0) * (rand() / ((double)RAND_MAX));
  UINT base_delay = SAMPLE_MAX_EXPONENTIAL_BACKOFF_IN_SEC;
  uint64_t delay;
  
  if (exponential_retry_count < (sizeof(UINT) * 8))
  {
    delay = (uint64_t)((1 << exponential_retry_count) * SAMPLE_INITIAL_EXPONENTIAL_BACKOFF_IN_SEC);
    if (delay <= (UINT)(-1))
    {
      base_delay = (UINT)delay;
    }
  }
  
  if (base_delay > SAMPLE_MAX_EXPONENTIAL_BACKOFF_IN_SEC)
  {
    base_delay = SAMPLE_MAX_EXPONENTIAL_BACKOFF_IN_SEC;
  }
  else
  {
    exponential_retry_count++;
  }
  
  return((UINT)(base_delay * (1 + jitter_percent)) * NX_IP_PERIODIC_RATE) ;
}

static VOID exponential_backoff_reset()
{
  exponential_retry_count = 0;
}

static VOID connection_status_callback(NX_AZURE_IOT_PNP_CLIENT *hub_client_ptr, UINT status)
{
  NX_PARAMETER_NOT_USED(hub_client_ptr);
  
  sample_connection_status = status;
  
#ifdef DEBUG_FUNCTION_CALL
  AZURE_PRINTF("-->connection_status_callback\r\n");
#endif /* DEBUG_FUNCTION_CALL */
  
  if (status)
  {
    AZURE_PRINTF("Disconnected from IoTHub!: error code = 0x%08x\r\n", status);
    tx_event_flags_set(&(sample_context.sample_events), SAMPLE_DISCONNECT_EVENT, TX_OR);
  }
  else
  {
    AZURE_PRINTF("Connected to IoTHub.\r\n");
    tx_event_flags_set(&(sample_context.sample_events), SAMPLE_CONNECTED_EVENT, TX_OR);
    exponential_backoff_reset();
  }
}

static VOID message_receive_callback_properties(NX_AZURE_IOT_PNP_CLIENT *hub_client_ptr, VOID *context)
{
  SAMPLE_CONTEXT *sample_ctx = (SAMPLE_CONTEXT *)context;
  
#ifdef DEBUG_FUNCTION_CALL
  AZURE_PRINTF("-->message_receive_callback_properties\r\n");
#endif /* DEBUG_FUNCTION_CALL */
  
  NX_PARAMETER_NOT_USED(hub_client_ptr);
  tx_event_flags_set(&(sample_ctx -> sample_events),
                     SAMPLE_DEVICE_PROPERTIES_GET_EVENT, TX_OR);
}

static VOID message_receive_callback_command(NX_AZURE_IOT_PNP_CLIENT *hub_client_ptr, VOID *context)
{
  SAMPLE_CONTEXT *sample_ctx = (SAMPLE_CONTEXT *)context;
  
#ifdef DEBUG_FUNCTION_CALL
  AZURE_PRINTF("-->message_receive_callback_command\r\n");
#endif /* DEBUG_FUNCTION_CALL */
  
  NX_PARAMETER_NOT_USED(hub_client_ptr);
  tx_event_flags_set(&(sample_ctx -> sample_events),
                     SAMPLE_COMMAND_MESSAGE_EVENT, TX_OR);
}

static VOID message_receive_callback_desired_property(NX_AZURE_IOT_PNP_CLIENT *hub_client_ptr, VOID *context)
{
  SAMPLE_CONTEXT *sample_ctx = (SAMPLE_CONTEXT *)context;
  
#ifdef DEBUG_FUNCTION_CALL
  AZURE_PRINTF("-->message_receive_callback_desired_property\r\n");
#endif /* DEBUG_FUNCTION_CALL */
  
  NX_PARAMETER_NOT_USED(hub_client_ptr);
  tx_event_flags_set(&(sample_ctx -> sample_events),
                     SAMPLE_DEVICE_DESIRED_PROPERTIES_EVENT, TX_OR);
}

static VOID sample_connect_action(SAMPLE_CONTEXT *context)
{
#ifdef DEBUG_FUNCTION_CALL
  AZURE_PRINTF("-->sample_connect_action\r\n");
#endif /* DEBUG_FUNCTION_CALL */
  
  if (context -> state != SAMPLE_STATE_CONNECT)
  {
    return;
  }
  
  context -> action_result = nx_azure_iot_pnp_client_connect(&(context -> iotpnp_client), NX_FALSE, SAMPLE_WAIT_OPTION);
  
  if (context -> action_result == NX_AZURE_IOT_CONNECTING)
  {
    context -> state = SAMPLE_STATE_CONNECTING;
  }
  else if (context -> action_result != NX_SUCCESS)
  {
    sample_connection_status = context -> action_result;
    context -> state = SAMPLE_STATE_DISCONNECTED;
  }
  else
  {
    context -> state = SAMPLE_STATE_CONNECTED;
    
    context -> action_result = nx_azure_iot_pnp_client_properties_request(&(context -> iotpnp_client), NX_WAIT_FOREVER);
  }
}

static VOID sample_disconnect_action(SAMPLE_CONTEXT *context)
{
#ifdef DEBUG_FUNCTION_CALL
  AZURE_PRINTF("-->sample_disconnect_action\r\n");
#endif /* DEBUG_FUNCTION_CALL */
  
  if (context -> state != SAMPLE_STATE_CONNECTED &&
      context -> state != SAMPLE_STATE_CONNECTING)
  {
    return;
  }
  
  context -> action_result = nx_azure_iot_pnp_client_disconnect(&(context -> iotpnp_client));
  context -> state = SAMPLE_STATE_DISCONNECTED;
}

static VOID sample_connected_action(SAMPLE_CONTEXT *context)
{
  
#ifdef DEBUG_FUNCTION_CALL
  AZURE_PRINTF("-->sample_connected_action\r\n");
#endif /* DEBUG_FUNCTION_CALL */

  if (context -> state != SAMPLE_STATE_CONNECTING)
  {
    return;
  }
  
  context -> state = SAMPLE_STATE_CONNECTED;
  
  context -> action_result = nx_azure_iot_pnp_client_properties_request(&(context -> iotpnp_client), NX_WAIT_FOREVER);
}

static VOID sample_initialize_iothub(SAMPLE_CONTEXT *context)
{
  UINT status;
  UCHAR *iothub_hostname = NX_NULL;
  UCHAR *iothub_device_id = NX_NULL;
  UINT iothub_hostname_length = 0;
  UINT iothub_device_id_length = 0;
 NX_AZURE_IOT_PNP_CLIENT *iotpnp_client_ptr = &(context -> iotpnp_client);

#ifdef DEBUG_FUNCTION_CALL
  AZURE_PRINTF("-->sample_initialize_iothub\r\n");
#endif /* DEBUG_FUNCTION_CALL */
  
  if (context -> state != SAMPLE_STATE_INIT)
  {
    return;
  }
  
  
  /* Run DPS.  */
  if ((status = sample_dps_entry(&(context -> prov_client), &iothub_hostname, &iothub_hostname_length,
                                 &iothub_device_id, &iothub_device_id_length)))
  {
    AZURE_PRINTF("Failed on sample_dps_entry!: error code = 0x%08x\r\n", status);
    context -> action_result = status;
    return;
  }
  
  //    AZURE_PRINTF("IoTHub Host Name: %.*s; Device ID: %.*s.\r\n",
  //           iothub_hostname_length, iothub_hostname, iothub_device_id_length, iothub_device_id);
  AZURE_PRINTF("IoTHub Host Name: %.*s\r\n",
         iothub_hostname_length, iothub_hostname);
  AZURE_PRINTF("Device ID: %.*s\r\n",
         iothub_device_id_length, iothub_device_id);
  
  /* Initialize IoTHub client.  */
  if ((status = nx_azure_iot_pnp_client_initialize(iotpnp_client_ptr, &nx_azure_iot,
                                                   iothub_hostname, iothub_hostname_length,
                                                   iothub_device_id, iothub_device_id_length,
                                                   (const UCHAR *)MODULE_ID, sizeof(MODULE_ID) - 1,
                                                   (const UCHAR *)SAMPLE_PNP_MODEL_ID, sizeof(SAMPLE_PNP_MODEL_ID) - 1,
                                                   _nx_azure_iot_tls_supported_crypto,
                                                   _nx_azure_iot_tls_supported_crypto_size,
                                                   _nx_azure_iot_tls_ciphersuite_map,
                                                   _nx_azure_iot_tls_ciphersuite_map_size,
                                                   nx_azure_iot_tls_metadata_buffer,
                                                   sizeof(nx_azure_iot_tls_metadata_buffer),
                                                   &root_ca_cert)))
  {
    AZURE_PRINTF("Failed on nx_azure_iot_pnp_client_initialize!: error code = 0x%08x\r\n", status);
    context -> action_result = status;
    return;
  }
  
  /* Set symmetric key.  */
  if ((status = nx_azure_iot_pnp_client_symmetric_key_set(iotpnp_client_ptr,
                                                          (UCHAR *)(AzureConnectionInfo.PrimaryKey),
                                                          strlen((char *)AzureConnectionInfo.PrimaryKey))))
  {
    AZURE_PRINTF("Failed on nx_azure_iot_pnp_client_symmetric_key_set! error: 0x%08x\r\n", status);
  }
  
  /* Set connection status callback.  */
    else if ((status = nx_azure_iot_pnp_client_connection_status_callback_set(iotpnp_client_ptr,
                                                                              connection_status_callback)))
    {
        AZURE_PRINTF("Failed on connection_status_callback!\r\n");
    }
    else if ((status = nx_azure_iot_pnp_client_receive_callback_set(iotpnp_client_ptr,
                                                                    NX_AZURE_IOT_PNP_PROPERTIES,
                                                                    message_receive_callback_properties,
                                                                    (VOID *)context)))
    {
        AZURE_PRINTF("device properties callback set!: error code = 0x%08x\r\n", status);
    }
    else if ((status = nx_azure_iot_pnp_client_receive_callback_set(iotpnp_client_ptr,
                                                                    NX_AZURE_IOT_PNP_COMMAND,
                                                                    message_receive_callback_command,
                                                                    (VOID *)context)))
    {
        AZURE_PRINTF("device command callback set!: error code = 0x%08x\r\n", status);
    }
    else if ((status = nx_azure_iot_pnp_client_receive_callback_set(iotpnp_client_ptr,
                                                                    NX_AZURE_IOT_PNP_DESIRED_PROPERTIES,
                                                                    message_receive_callback_desired_property,
                                                                    (VOID *)context)))
    {
        AZURE_PRINTF("device desired property callback set!: error code = 0x%08x\r\n", status);
    }
    else if ((status = nx_azure_iot_pnp_client_component_add(iotpnp_client_ptr,
                                                             (const UCHAR *)sample_StdComponent_component,
                                                             sizeof(sample_StdComponent_component) - 1)) ||
              (status = nx_azure_iot_pnp_client_component_add(iotpnp_client_ptr,
                                                             (const UCHAR *)sample_device_info_component,
                                                             sizeof(sample_device_info_component) - 1)))
    {
        AZURE_PRINTF("Failed to add component to pnp client!: error code = 0x%08x\r\n", status);
    }

    if (status)
    {
        nx_azure_iot_pnp_client_deinitialize(iotpnp_client_ptr);
    }

    context -> action_result = status;

    if (status == NX_AZURE_IOT_SUCCESS)
    {
        context -> state = SAMPLE_STATE_CONNECT;
    }
}

static VOID sample_connection_error_recover(SAMPLE_CONTEXT *context)
{
#ifdef DEBUG_FUNCTION_CALL
  AZURE_PRINTF("-->sample_connection_error_recover\r\n");
#endif /* DEBUG_FUNCTION_CALL */

  if (context -> state != SAMPLE_STATE_DISCONNECTED)
  {
    return;
  }
  
  switch (sample_connection_status)
  {
  case NX_AZURE_IOT_SUCCESS:
    {
      AZURE_PRINTF("already connected\r\n");
    }
    break;
    
    /* Something bad has happened with client state, we need to re-initialize it.  */
  case NX_DNS_QUERY_FAILED :
  case NXD_MQTT_ERROR_BAD_USERNAME_PASSWORD :
  case NXD_MQTT_ERROR_NOT_AUTHORIZED :
    {
      AZURE_PRINTF("re-initializing iothub connection, after backoff\r\n");
      
      tx_thread_sleep(exponential_backoff_with_jitter());
      nx_azure_iot_pnp_client_deinitialize(&(context -> iotpnp_client));
      context -> state = SAMPLE_STATE_INIT;
    }
    break;
    
  default :
    {
      AZURE_PRINTF("reconnecting iothub, after backoff\r\n");
      
      tx_thread_sleep(exponential_backoff_with_jitter());
      context -> state = SAMPLE_STATE_CONNECT;
    }
    break;
  }
}

static VOID sample_trigger_action(SAMPLE_CONTEXT *context)
{
#ifdef DEBUG_FUNCTION_CALL
  AZURE_PRINTF("-->sample_trigger_action\r\n");
#endif /* DEBUG_FUNCTION_CALL */
  
  switch (context -> state)
  {
  case SAMPLE_STATE_INIT:
    {
      tx_event_flags_set(&(context -> sample_events), SAMPLE_INITIALIZATION_EVENT, TX_OR);
    }
    break;
    
  case SAMPLE_STATE_CONNECT:
    {
      tx_event_flags_set(&(context -> sample_events), SAMPLE_CONNECT_EVENT, TX_OR);
    }
    break;
    
  case SAMPLE_STATE_CONNECTED:
    {
      if ((tx_time_get() - context -> last_periodic_action_tick) >= (sample_StdComp.SendIntervalSec))
      {
        context -> last_periodic_action_tick = tx_time_get();
        AZURE_PRINTF("last_periodic_action_tick=%ld\r\n",context -> last_periodic_action_tick);
        tx_event_flags_set(&(context -> sample_events), SAMPLE_TELEMETRY_SEND_EVENT, TX_OR);
        tx_event_flags_set(&(context -> sample_events), SAMPLE_DEVICE_REPORTED_PROPERTIES_EVENT, TX_OR);
      }
    }
    break;
    
  case SAMPLE_STATE_DISCONNECTED:
    {
      tx_event_flags_set(&(context -> sample_events), SAMPLE_RECONNECT_EVENT, TX_OR);
    }
    break;
  }
}

static VOID sample_command_action(SAMPLE_CONTEXT *sample_context_ptr)
{
UINT status;
USHORT context_length;
VOID *context_ptr;
UINT component_name_length;
const UCHAR *component_name_ptr;
UINT pnp_command_name_length;
const UCHAR *pnp_command_name_ptr;
NX_AZURE_IOT_JSON_WRITER json_writer;
NX_AZURE_IOT_JSON_READER json_reader;
UINT status_code;
UINT response_length;

#ifdef DEBUG_FUNCTION_CALL
  AZURE_PRINTF("-->sample_command_action\r\n");
#endif /* DEBUG_FUNCTION_CALL */

    /* Loop to receive command message.  */
    while (1)
    {

        if (sample_context_ptr -> state != SAMPLE_STATE_CONNECTED)
        {
            return;
        }

        if ((status = nx_azure_iot_pnp_client_command_receive(&(sample_context_ptr -> iotpnp_client),
                                                              &component_name_ptr, &component_name_length,
                                                              &pnp_command_name_ptr, &pnp_command_name_length,
                                                              &context_ptr, &context_length,
                                                              &json_reader, NX_NO_WAIT)))
        {
            return;
        }

        if (component_name_ptr != NX_NULL)
        {
            AZURE_PRINTF("Received component: %.*s ", component_name_length, component_name_ptr);
        }
        else
        {
            AZURE_PRINTF("Received component: root component ");
        }

        AZURE_PRINTF("command: %.*s", pnp_command_name_length, (CHAR *)pnp_command_name_ptr);
        AZURE_PRINTF("\r\n");

        if ((status = nx_azure_iot_json_writer_with_buffer_init(&json_writer,
                                                                scratch_buffer,
                                                                sizeof(scratch_buffer))))
        {
            AZURE_PRINTF("Failed to initialize json builder response \r\n");
            nx_azure_iot_json_reader_deinit(&json_reader);
            return;
        }
      
        {
            AZURE_PRINTF("Failed to find any handler for command %.*s\r\n", pnp_command_name_length, pnp_command_name_ptr);
            status_code = SAMPLE_COMMAND_NOT_FOUND_STATUS;
            response_length = 0;
        }

        nx_azure_iot_json_reader_deinit(&json_reader);

        if ((status = nx_azure_iot_pnp_client_command_message_response(&(sample_context_ptr -> iotpnp_client), status_code,
                                                                       context_ptr, context_length, scratch_buffer,
                                                                       response_length, NX_WAIT_FOREVER)))
        {
            AZURE_PRINTF("Command response failed!: error code = 0x%08x\r\n", status);
        }

        nx_azure_iot_json_writer_deinit(&json_writer);
    }
}

static VOID sample_desired_properties_parse(NX_AZURE_IOT_PNP_CLIENT *pnp_client_ptr,
                                            NX_AZURE_IOT_JSON_READER *json_reader_ptr,
                                            UINT message_type, ULONG version)
{
const UCHAR *component_ptr = NX_NULL;
UINT component_len = 0;
NX_AZURE_IOT_JSON_READER name_value_reader;

const UCHAR *last_component_ptr = NX_NULL;
UINT last_component_len = 0;

#ifdef DEBUG_FUNCTION_CALL
  AZURE_PRINTF("-->sample_desired_properties_parse\r\n");
#endif /* DEBUG_FUNCTION_CALL */
  
  
    
    AZURE_PRINTF("\t- Twin Desired Properties: %.*s\r\n", json_reader_ptr->json_reader._internal.json_buffer._internal.size, json_reader_ptr->json_reader._internal.json_buffer._internal.ptr);

    while (nx_azure_iot_pnp_client_desired_component_property_value_next(pnp_client_ptr,
                                                                         json_reader_ptr, message_type,
                                                                         &component_ptr, &component_len,
                                                                         &name_value_reader) == NX_AZURE_IOT_SUCCESS)
    {
      //workaround for answering to 2 desired properties for one component in the same time
      if((component_ptr==NX_NULL) & (component_len==0)) {
        //if we are still in the same component... use the previous name
        component_ptr = last_component_ptr;
        component_len = last_component_len;
      } else {
        last_component_ptr = component_ptr;
        last_component_len = component_len;
      }
        AZURE_PRINTF("Component= %.*s\r\n", component_len, component_ptr);
        if (StdComponent_process_property_update(&sample_StdComp,
                                                               pnp_client_ptr,
                                                               component_ptr, component_len,
                                                               &name_value_reader, version) == NX_AZURE_IOT_SUCCESS)
        {
            AZURE_PRINTF("property updated of StdComp\r\n");
        }
        else
        {
            if (component_ptr)
            {
                AZURE_PRINTF("Component=%.*s is not implemented by the Controller\r\n", component_len, component_ptr);
            }
            else
            {
                AZURE_PRINTF("Root component is not implemented by the Controller\r\n");
            }
        }
    }
    
    //Report the Initial values of Writable Desired Property if it's necessary
    if(sample_StdComp.ReceivedTelemetyInterval==0) {
      sample_send_target_StdComponent_telemetry_interval_report(&sample_StdComp, pnp_client_ptr, sample_StdComp.CurrentTelemetryInterval/NX_IP_PERIODIC_RATE, 200, 1, "success");
      sample_StdComp.ReceivedTelemetyInterval=1;
    }
}

static VOID sample_device_desired_property_action(SAMPLE_CONTEXT *context)
{

UINT status = 0;
NX_AZURE_IOT_JSON_READER json_reader;
ULONG desired_properties_version;

#ifdef DEBUG_FUNCTION_CALL
  AZURE_PRINTF("-->sample_device_desired_property_action\r\n");
#endif /* DEBUG_FUNCTION_CALL */

    if (context -> state != SAMPLE_STATE_CONNECTED)
    {
        return;
    }

    if ((status = nx_azure_iot_pnp_client_desired_properties_receive(&(context -> iotpnp_client),
                                                                     &json_reader,
                                                                     &desired_properties_version,
                                                                     NX_WAIT_FOREVER)))
    {
        AZURE_PRINTF("desired properties receive failed!: error code = 0x%08x\r\n", status);
        return;
    }

    AZURE_PRINTF("Received desired property");
    AZURE_PRINTF("\r\n");

    sample_desired_properties_parse(&(context -> iotpnp_client), &json_reader,
                                    NX_AZURE_IOT_PNP_DESIRED_PROPERTIES,
                                    desired_properties_version);

    nx_azure_iot_json_reader_deinit(&json_reader);
}

static VOID sample_device_reported_property_action(SAMPLE_CONTEXT *context)
{
  
#ifdef DEBUG_FUNCTION_CALL
  AZURE_PRINTF("-->sample_device_reported_property_action\r\n");
#endif /* DEBUG_FUNCTION_CALL */
  
  if (context -> state != SAMPLE_STATE_CONNECTED)
  {
    return;
  }
  
  /* Device Info  */
  if (sample_device_info_sent == 0) {
    AZURE_PRINTF("DeviceInfo_report_all_properties\r\n");
    DeviceInfo_report_all_properties(sample_StdComp.SendIntervalSec,
                                     (UCHAR *)sample_device_info_component,
                                     sizeof(sample_device_info_component) - 1,
                                     &(context -> iotpnp_client));
    sample_device_info_sent = 1;
  }
  
  /* STD_COMP */
  if(StdComp_property_sent==0) {
    /*  Avoid to report the writable reported properties */
//    AZURE_PRINTF("StdComponent_report_all_properties\r\n");
//    StdComponent_report_all_properties(&sample_StdComp, (UCHAR *)sample_StdComponent_component,
//                                                              sizeof(sample_StdComponent_component) - 1,
//                                                              &(context -> iotpnp_client));
    StdComp_property_sent=1;
  }
}

static VOID sample_device_properties_get_action(SAMPLE_CONTEXT *context)
{
  UINT status = 0;
NX_AZURE_IOT_JSON_READER json_reader;
ULONG desired_properties_version;

#ifdef DEBUG_FUNCTION_CALL
  AZURE_PRINTF("-->sample_device_properties_get_action\r\n");
#endif /* DEBUG_FUNCTION_CALL */

    if (context -> state != SAMPLE_STATE_CONNECTED)
    {
        return;
    }

    if ((status = nx_azure_iot_pnp_client_properties_receive(&(context -> iotpnp_client),
                                                             &json_reader,
                                                             &desired_properties_version,
                                                             NX_WAIT_FOREVER)))
    {
        AZURE_PRINTF("Get all properties receive failed!: error code = 0x%08x\r\n", status);
        return;
    }

    AZURE_PRINTF("Received all properties");
    AZURE_PRINTF("\r\n");

    sample_desired_properties_parse(&(context -> iotpnp_client), &json_reader,
                                    NX_AZURE_IOT_PNP_PROPERTIES,
                                    desired_properties_version);

    nx_azure_iot_json_reader_deinit(&json_reader);
}

static VOID sample_telemetry_action(SAMPLE_CONTEXT *context)
{
  UINT status;
#ifdef DEBUG_FUNCTION_CALL
  AZURE_PRINTF("-->sample_telemetry_action\r\n");
#endif /* DEBUG_FUNCTION_CALL */
  
  if (context -> state != SAMPLE_STATE_CONNECTED)
  {
    return;
  }
  
  if ((status = StdComponent_telemetry_send(&sample_StdComp, &(context -> iotpnp_client))) != NX_AZURE_IOT_SUCCESS){
    AZURE_PRINTF("Failed to send StdComponent_telemetry_send, error: %d", status);
  }
}

static UINT sample_dps_entry(NX_AZURE_IOT_PROVISIONING_CLIENT *prov_client_ptr,
                             UCHAR **iothub_hostname, UINT *iothub_hostname_length,
                             UCHAR **iothub_device_id, UINT *iothub_device_id_length)
{
  UINT status;
#ifdef DEBUG_FUNCTION_CALL
  AZURE_PRINTF("-->sample_dps_entry\r\n");
#endif /* DEBUG_FUNCTION_CALL */
  
  AZURE_PRINTF("Start Provisioning Client...\r\n");
  
  /* Initialize IoT provisioning client.  */
  if ((status = nx_azure_iot_provisioning_client_initialize(prov_client_ptr, &nx_azure_iot,
                                                            (UCHAR *)ENDPOINT, sizeof(ENDPOINT) - 1,
                                                            (UCHAR *)AzureConnectionInfo.ScopeID, strlen((char *)AzureConnectionInfo.ScopeID),
                                                            (UCHAR *)AzureConnectionInfo.DeviceID, strlen((char *)AzureConnectionInfo.DeviceID),
                                                            _nx_azure_iot_tls_supported_crypto,
                                                            _nx_azure_iot_tls_supported_crypto_size,
                                                            _nx_azure_iot_tls_ciphersuite_map,
                                                            _nx_azure_iot_tls_ciphersuite_map_size,
                                                            nx_azure_iot_tls_metadata_buffer,
                                                            sizeof(nx_azure_iot_tls_metadata_buffer),
                                                            &root_ca_cert)))
  {
    AZURE_PRINTF("Failed on nx_azure_iot_provisioning_client_initialize!: error code = 0x%08x\r\n", status);
    return(status);
  }
  
  /* Initialize length of hostname and device ID.  */
  *iothub_hostname_length = sizeof(sample_iothub_hostname);
  *iothub_device_id_length = sizeof(sample_iothub_device_id);
  
  /* Set symmetric key.  */ 
  if ((status = nx_azure_iot_provisioning_client_symmetric_key_set(prov_client_ptr, (UCHAR *)(AzureConnectionInfo.PrimaryKey),
                                                                   strlen((char *)AzureConnectionInfo.PrimaryKey))))
  {
    AZURE_PRINTF("Failed on nx_azure_iot_pnp_client_symmetric_key_set!: error code = 0x%08x\r\n", status);
  }

  else if ((status = nx_azure_iot_provisioning_client_registration_payload_set(prov_client_ptr, (UCHAR *)SAMPLE_PNP_DPS_PAYLOAD,
                                                                               sizeof(SAMPLE_PNP_DPS_PAYLOAD) - 1)))
  {
    AZURE_PRINTF("Failed on nx_azure_iot_provisioning_client_registration_payload_set!: error code = 0x%08x\r\n", status);
  }
  
  /* Register device */
  else if ((status = nx_azure_iot_provisioning_client_register(prov_client_ptr, NX_WAIT_FOREVER)))
  {
    AZURE_PRINTF("Failed on nx_azure_iot_provisioning_client_register!: error code = 0x%08x\r\n", status);
  }
  
  /* Get Device info */
  else if ((status = nx_azure_iot_provisioning_client_iothub_device_info_get(prov_client_ptr,
                                                                             sample_iothub_hostname, iothub_hostname_length,
                                                                             sample_iothub_device_id, iothub_device_id_length)))
  {
    AZURE_PRINTF("Failed on nx_azure_iot_provisioning_client_iothub_device_info_get!: error code = 0x%08x\r\n", status);
  }
  else
  {
    *iothub_hostname = sample_iothub_hostname;
    *iothub_device_id = sample_iothub_device_id;
    AZURE_PRINTF("Registered Device Successfully.\r\n");
  }
  
  /* Destroy Provisioning Client.  */
  nx_azure_iot_provisioning_client_deinitialize(prov_client_ptr);
  
  return(status);
}

/**
*
* Sample Event loop
*
*
*       +--------------+           +--------------+      +--------------+       +--------------+
*       |              |  INIT     |              |      |              |       |              |
*       |              | SUCCESS   |              |      |              |       |              +--------+
*       |    INIT      |           |    CONNECT   |      |  CONNECTING  |       |   CONNECTED  |        | (TELEMETRY |
*       |              +----------->              +----->+              +------->              |        |  COMMAND |
*       |              |           |              |      |              |       |              <--------+  PROPERTIES)
*       |              |           |              |      |              |       |              |
*       +-----+--------+           +----+---+-----+      +------+-------+       +--------+-----+
*             ^                         ^   |                   |                        |
*             |                         |   |                   |                        |
*             |                         |   |                   |                        |
*             |                         |   | CONNECT           | CONNECTING             |
*             |                         |   |  FAIL             |   FAIL                 |
* REINITIALIZE|                RECONNECT|   |                   |                        |
*             |                         |   |                   v                        |  DISCONNECT
*             |                         |   |        +----------+-+                      |
*             |                         |   |        |            |                      |
*             |                         |   +------->+            |                      |
*             |                         |            | DISCONNECT |                      |
*             |                         |            |            +<---------------------+
*             |                         +------------+            |
*             +--------------------------------------+            |
*                                                    +------------+
*
*
*
*/
static VOID sample_event_loop(SAMPLE_CONTEXT *context)
{
  ULONG app_events;
  UINT loop = NX_TRUE;
#ifdef DEBUG_FUNCTION_CALL
  AZURE_PRINTF("-->sample_event_loop\r\n");
#endif /* DEBUG_FUNCTION_CALL */
  
  while (loop)
  {
    
    /* Pickup IP event flags.  */
    if (tx_event_flags_get(&(context -> sample_events), SAMPLE_ALL_EVENTS, TX_OR_CLEAR, &app_events, sample_StdComp.SendIntervalSec))
    {
      if (context -> state == SAMPLE_STATE_CONNECTED)
      {
        sample_trigger_action(context);
      }
      
      continue;
    }
    
    AZURE_PRINTF("@%ld\r\n",tx_time_get());
    
    if (app_events & SAMPLE_CONNECT_EVENT)
    {
      AZURE_PRINTF("SAMPLE_CONNECT_EVENT\r\n");
      sample_connect_action(context);
    }
    
    if (app_events & SAMPLE_INITIALIZATION_EVENT)
    {
      AZURE_PRINTF("SAMPLE_INITIALIZATION_EVENT\r\n");
      sample_initialize_iothub(context);
    }
    
    if (app_events & SAMPLE_DEVICE_PROPERTIES_GET_EVENT)
    {
      AZURE_PRINTF("SAMPLE_DEVICE_PROPERTIES_GET_EVENT\r\n");
      sample_device_properties_get_action(context);
    }
    
    if (app_events & SAMPLE_COMMAND_MESSAGE_EVENT)
    {
      AZURE_PRINTF("SAMPLE_COMMAND_MESSAGE_EVENT\r\n");
      sample_command_action(context);
    }
    
    if (app_events & SAMPLE_DEVICE_DESIRED_PROPERTIES_EVENT)
    {
      AZURE_PRINTF("SAMPLE_DEVICE_DESIRED_PROPERTIES_EVENT\r\n");
      sample_device_desired_property_action(context);
    }
    
    if (app_events & SAMPLE_TELEMETRY_SEND_EVENT)
    {
      AZURE_PRINTF("SAMPLE_TELEMETRY_SEND_EVENT\r\n");
      sample_telemetry_action(context);
    }
    
    if (app_events & SAMPLE_DEVICE_REPORTED_PROPERTIES_EVENT)
    {
      AZURE_PRINTF("SAMPLE_DEVICE_REPORTED_PROPERTIES_EVENT\r\n");
      sample_device_reported_property_action(context);
    }
    
    if (app_events & SAMPLE_DISCONNECT_EVENT)
    {
      AZURE_PRINTF("SAMPLE_DISCONNECT_EVENT\r\n");
      sample_disconnect_action(context);
    }
    
    if (app_events & SAMPLE_CONNECTED_EVENT)
    {
      AZURE_PRINTF("SAMPLE_CONNECTED_EVENT\r\n");
      sample_connected_action(context);
    }
    
    if (app_events & SAMPLE_RECONNECT_EVENT)
    {
      AZURE_PRINTF("SAMPLE_RECONNECT_EVENT\r\n");
      sample_connection_error_recover(context);
    }
    
    sample_trigger_action(context);
  }
}

static VOID sample_context_init(SAMPLE_CONTEXT *context)
{
#ifdef DEBUG_FUNCTION_CALL
  AZURE_PRINTF("-->sample_context_init\r\n");
#endif /* DEBUG_FUNCTION_CALL */
  
  memset(context, 0, sizeof(SAMPLE_CONTEXT));
  tx_event_flags_create(&(context->sample_events), (CHAR*)"sample_app");
}

static VOID log_callback(az_log_classification classification, UCHAR *msg, UINT msg_len)
{ 
  if (classification == AZ_LOG_IOT_AZURERTOS)
  {
    AZURE_PRINTF("%.*s", msg_len, (CHAR *)msg);
  }
}

static UINT sample_components_init()
{
  UINT status;
  
#ifdef DEBUG_FUNCTION_CALL
  AZURE_PRINTF("-->sample_components_init\r\n");
#endif /* DEBUG_FUNCTION_CALL */
  
  if ((status = StdComponent_init(&sample_StdComp,
                                            (UCHAR *)sample_StdComponent_component,
                                            sizeof(sample_StdComponent_component) - 1)))
  {
    AZURE_PRINTF("Failed to initialize %s: error code = 0x%08x\r\n",
           sample_StdComponent_component, status);
  }
  
  
  sample_device_info_sent = 0;
  StdComp_property_sent=0;
  
  return(status);
}

VOID sample_entry(NX_IP *ip_ptr, NX_PACKET_POOL *pool_ptr, NX_DNS *dns_ptr, UINT (*unix_time_callback)(ULONG *unix_time))
{
  
#ifdef DEBUG_FUNCTION_CALL
  AZURE_PRINTF("-->sample_entry\r\n");
#endif /* DEBUG_FUNCTION_CALL */
  UINT status;
  
  nx_azure_iot_log_init(log_callback);
  
  if ((status = sample_components_init()))
  {
    AZURE_PRINTF("Failed on initialize sample components!: error code = 0x%08x\r\n", status);
    return;
  }
  
  /* Create Azure IoT handler.  */
  if ((status = nx_azure_iot_create(&nx_azure_iot, (UCHAR *)"Azure IoT", ip_ptr, pool_ptr, dns_ptr,
                                    nx_azure_iot_thread_stack, sizeof(nx_azure_iot_thread_stack),
                                    NX_AZURE_IOT_THREAD_PRIORITY, unix_time_callback)))
  {
    AZURE_PRINTF("Failed on nx_azure_iot_create!: error code = 0x%08x\r\n", status);
    return;
  }
  
  /* Initialize CA certificate.  */
  if ((status = nx_secure_x509_certificate_initialize(&root_ca_cert, (UCHAR *)_nx_azure_iot_root_cert,
                                                      (USHORT)_nx_azure_iot_root_cert_size,
                                                      NX_NULL, 0, NULL, 0, NX_SECURE_X509_KEY_TYPE_NONE)))
  {
    AZURE_PRINTF("Failed to initialize ROOT CA certificate!: error code = 0x%08x\r\n", status);
    nx_azure_iot_delete(&nx_azure_iot);
    return;
  }
  
  sample_context_init(&sample_context);
  
  sample_context.state = SAMPLE_STATE_INIT;
  tx_event_flags_set(&(sample_context.sample_events), SAMPLE_INITIALIZATION_EVENT, TX_OR);
  
  /* Handle event loop.  */
  sample_event_loop(&sample_context);
  
  nx_azure_iot_delete(&nx_azure_iot);
}
