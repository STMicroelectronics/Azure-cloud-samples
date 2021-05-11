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

#include "nx_api.h"
#include "wifi.h"
#include "nx_wifi.h"
#include "nxd_dns.h"
#include "nxd_sntp_client.h"
#include "nx_secure_tls_api.h"
#include "stm32l4xx_hal.h"
#include "azure_customizations.h"

/* Include the sample.  */
extern VOID sample_entry(NX_IP* ip_ptr, NX_PACKET_POOL* pool_ptr, NX_DNS* dns_ptr, UINT (*unix_time_callback)(ULONG *unix_time));

/* Define the thread for running Azure sample on ThreadX (X-Ware IoT Platform).  */
#ifndef SAMPLE_THREAD_STACK_SIZE
#define SAMPLE_THREAD_STACK_SIZE        (6144)
#endif /* SAMPLE_THREAD_STACK_SIZE  */

#ifndef SAMPLE_THREAD_PRIORITY
#define SAMPLE_THREAD_PRIORITY          (4)
#endif /* SAMPLE_THREAD_PRIORITY  */

/* Define the memory area for the sample thread.  */
static UCHAR sample_thread_stack[SAMPLE_THREAD_STACK_SIZE];

/* Define the prototypes for the sample thread.  */
static TX_THREAD sample_thread;
static void sample_thread_entry(ULONG parameter);

/* Define the default thread priority, stack size, etc. The user can override this 
   via -D command line option or via project settings.  */

#ifndef SAMPLE_PACKET_COUNT
#define SAMPLE_PACKET_COUNT             (20)
#endif /* SAMPLE_PACKET_COUNT  */

#ifndef SAMPLE_PACKET_SIZE
#define SAMPLE_PACKET_SIZE              (1200)  /* Set the default value to 1200 since WIFI payload size (ES_WIFI_PAYLOAD_SIZE) is 1200.  */
#endif /* SAMPLE_PACKET_SIZE  */

#define SAMPLE_POOL_SIZE                ((SAMPLE_PACKET_SIZE + sizeof(NX_PACKET)) * SAMPLE_PACKET_COUNT)

/* Define the stack/cache for ThreadX.  */
static UCHAR sample_pool_stack[SAMPLE_POOL_SIZE];

/* Define the prototypes for ThreadX.  */
static NX_IP                            ip_0;
static NX_PACKET_POOL                   pool_0;
static NX_DNS     				    	dns_client;



/* Using SNTP to get unix time.  */
/* Define the address of SNTP Server. If not defined, use DNS module to resolve the host name sample_SNTP_SERVER_NAME.  */
/*
#define SAMPLE_SNTP_SERVER_ADDRESS     IP_ADDRESS(118, 190, 21, 209)
*/

#ifndef SAMPLE_SNTP_SERVER_NAME
#define SAMPLE_SNTP_SERVER_NAME           "0.pool.ntp.org"    /* SNTP Server.  */
#endif /* SAMPLE_SNTP_SERVER_NAME */

#ifndef SAMPLE_SNTP_SYNC_MAX
#define SAMPLE_SNTP_SYNC_MAX              10
#endif /* SAMPLE_SNTP_SYNC_MAX */

#ifndef SAMPLE_SNTP_UPDATE_MAX
#define SAMPLE_SNTP_UPDATE_MAX            10
#endif /* SAMPLE_SNTP_UPDATE_MAX */

#ifndef SAMPLE_SNTP_UPDATE_INTERVAL
#define SAMPLE_SNTP_UPDATE_INTERVAL       (NX_IP_PERIODIC_RATE / 2)
#endif /* SAMPLE_SNTP_UPDATE_INTERVAL */

/* Default time. GMT: Friday, Jan 1, 2021 12:00:00 AM. Epoch timestamp: 1609459200.  */
#ifndef SAMPLE_SYSTEM_TIME 
#define SAMPLE_SYSTEM_TIME                1609459200
#endif /* SAMPLE_SYSTEM_TIME  */

/* Seconds between Unix Epoch (1/1/1970) and NTP Epoch (1/1/1999) */
#define SAMPLE_UNIX_TO_NTP_EPOCH_SECOND   0x83AA7E80

static NX_SNTP_CLIENT   sntp_client;

/* System clock time for UTC.  */
static ULONG            unix_time_base;

static UINT dns_create();

static UINT sntp_time_sync();

static UINT unix_time_get(ULONG *unix_time);


VOID board_setup(void);

/* Define main entry point.  */
int main(void)
{
  /* Setup platform. */
  board_setup();
  
  /* Enter the ThreadX kernel.  */
  tx_kernel_enter();
}

/* Define what the initial system looks like.  */
void    tx_application_define(void *first_unused_memory)
{
  
  UINT    status;
  
  /* Initialize the NetX system.  */
  nx_system_initialize();
  
  /* Create a packet pool.  */
  status = nx_packet_pool_create(&pool_0, "NetX Main Packet Pool", SAMPLE_PACKET_SIZE,
                                 sample_pool_stack , SAMPLE_POOL_SIZE);
  
  /* Check for pool creation error.  */
  if (status)
  {
    AZURE_PRINTF("PACKET POOL CREATE FAIL.\r\n");
    return;
  }
  
  /* Create an IP instance.  */
  status = nx_ip_create(&ip_0, "NetX IP Instance 0", 0, 0,
                        &pool_0, NULL, NULL, NULL, 0);
  
  /* Check for IP create errors.  */
  if (status)
  {
    AZURE_PRINTF("IP CREATE FAIL.\r\n");
    return;
  }
  
  /* Initialize THREADX Wifi.  */
  status = nx_wifi_initialize(&ip_0, &pool_0);
  
  /* Check status.  */
  if (status)
  {
    AZURE_PRINTF("WIFI INITIALIZE FAIL.\r\n");
    return;
  }     
  
  /* Initialize TLS.  */
  nx_secure_tls_initialize();
  
  /* Create Threadx Azure thread. */
  status = tx_thread_create(&sample_thread, "Sample Thread",
                            sample_thread_entry, 0,
                            sample_thread_stack, SAMPLE_THREAD_STACK_SIZE,
                            SAMPLE_THREAD_PRIORITY, SAMPLE_THREAD_PRIORITY, 
                            TX_NO_TIME_SLICE, TX_AUTO_START);    
  
  /* Check status.  */
  if (status)
  {
    AZURE_PRINTF("Azure sample Thread Create Failed.\r\n");
  }
}

/* Define sample helper thread entry.  */
void sample_thread_entry(ULONG parameter)
{
  UINT    status;
  
  /* Create DNS.  */
  status = dns_create();
  
  /* Check for DNS create errors.  */
  if (status)
  {
    AZURE_PRINTF("dns_create fail: %u\r\n", status);
    return;
  }
  
  /* Sync up time by SNTP at start up.  */
  for (UINT i = 0; i < SAMPLE_SNTP_SYNC_MAX; i++)
  {
    
    /* Start SNTP to sync the local time.  */
    status = sntp_time_sync();
    
    /* Check status.  */
    if(status == NX_SUCCESS)
      break;
  }
  
  /* Check status.  */
  if (status)
  {
    AZURE_PRINTF("SNTP Time Sync failed.\r\n");
    AZURE_PRINTF("Set Time to default value: SAMPLE_SYSTEM_TIME.\r\n");
    unix_time_base = SAMPLE_SYSTEM_TIME;
  }
  else
  {
    AZURE_PRINTF("SNTP Time Sync successfully.\r\n");
    /* Start sample.  */
    sample_entry(&ip_0, &pool_0, &dns_client, unix_time_get);
  }
}


static UINT	dns_create(ULONG dns_server_address)
{
  
  UINT	status; 
  UCHAR   dns_address_1[4]; 
  UCHAR   dns_address_2[4];
  
  /* Create a DNS instance for the Client.  Note this function will create
  the DNS Client packet pool for creating DNS message packets intended
  for querying its DNS server. */
  status = nx_dns_create(&dns_client, &ip_0, (UCHAR *)"DNS Client");
  if (status)
  {
    return(status);
  }
  
  /* Is the DNS client configured for the host application to create the pecket pool? */
#ifdef NX_DNS_CLIENT_USER_CREATE_PACKET_POOL   
  
  /* Yes, use the packet pool created above which has appropriate payload size
  for DNS messages. */
  status = nx_dns_packet_pool_set(&dns_client, &pool_0);
  if (status)
  {
    return(status);
  }
#endif /* NX_DNS_CLIENT_USER_CREATE_PACKET_POOL */  
  
  /* Get the DNS address.  */
  if (WIFI_GetDNS_Address(dns_address_1, dns_address_2) != WIFI_STATUS_OK)
  {
    nx_dns_delete(&dns_client);
    return(NX_NOT_SUCCESSFUL);
  }
  
  /* Add an IPv4 server address to the Client list. */
  status = nx_dns_server_add(&dns_client, IP_ADDRESS(dns_address_1[0], dns_address_1[1], dns_address_1[2], dns_address_1[3]));
  if (status)
  {
    nx_dns_delete(&dns_client);
    return(status);
  }
  
  return(NX_SUCCESS);
}

/* Sync up the local time.  */
static UINT sntp_time_sync()
{
  
  UINT    status;
  UINT    server_status;
  ULONG   sntp_server_address;
  UINT    i;
  
  
  AZURE_PRINTF("SNTP Time Sync...\r\n");
  
#ifndef SAMPLE_SNTP_SERVER_ADDRESS
  /* Look up SNTP Server address. */
  status = nx_dns_host_by_name_get(&dns_client, (UCHAR *)SAMPLE_SNTP_SERVER_NAME, &sntp_server_address, 5 * NX_IP_PERIODIC_RATE);
  
  /* Check status.  */
  if (status)
  {
    return(status);
  }
#else /* !SAMPLE_SNTP_SERVER_ADDRESS */
  sntp_server_address = SAMPLE_SNTP_SERVER_ADDRESS;
#endif /* SAMPLE_SNTP_SERVER_ADDRESS */
  
  /* Create the SNTP Client to run in broadcast mode.. */
  status =  nx_sntp_client_create(&sntp_client, &ip_0, 0, &pool_0,  
                                  NX_NULL,
                                  NX_NULL,
                                  NX_NULL /* no random_number_generator callback */);
  
  /* Check status.  */
  if (status)
  {
    return(status);
  }
  
  /* Use the IPv4 service to initialize the Client and set the IPv4 SNTP server. */
  status = nx_sntp_client_initialize_unicast(&sntp_client, sntp_server_address);
  
  /* Check status.  */
  if (status)
  {
    nx_sntp_client_delete(&sntp_client);
    return(status);
  }
  
  /* Set local time to 0 */
  status = nx_sntp_client_set_local_time(&sntp_client, 0, 0);
  
  /* Check status.  */
  if (status)
  {
    nx_sntp_client_delete(&sntp_client);
    return(status);
  }
  
  /* Run Unicast client */
  status = nx_sntp_client_run_unicast(&sntp_client);
  
  /* Check status.  */
  if (status)
  {
    nx_sntp_client_stop(&sntp_client);
    nx_sntp_client_delete(&sntp_client);
    return(status);
  }
  
  /* Wait till updates are received */
  for (i = 0; i < SAMPLE_SNTP_UPDATE_MAX; i++)
  {
    
    /* First verify we have a valid SNTP service running. */
    status = nx_sntp_client_receiving_updates(&sntp_client, &server_status);
    
    /* Check status.  */
    if ((status == NX_SUCCESS) && (server_status == NX_TRUE))
    {
      
      /* Server status is good. Now get the Client local time. */
      ULONG sntp_seconds, sntp_fraction;
      ULONG system_time_in_second;
      
      /* Get the local time.  */
      status = nx_sntp_client_get_local_time(&sntp_client, &sntp_seconds, &sntp_fraction, NX_NULL);
      
      /* Check status.  */
      if (status != NX_SUCCESS)
      {
        continue;
      }
      
      /* Get the system time in second.  */
      system_time_in_second = tx_time_get() / TX_TIMER_TICKS_PER_SECOND;
      
      /* Convert to Unix epoch and minus the current system time.  */
      unix_time_base = (sntp_seconds - (system_time_in_second + SAMPLE_UNIX_TO_NTP_EPOCH_SECOND));
      
      /* Time sync successfully.  */
      
      /* Stop and delete SNTP.  */
      nx_sntp_client_stop(&sntp_client);
      nx_sntp_client_delete(&sntp_client);
      
      return(NX_SUCCESS);
    }
    
    /* Sleep.  */
    tx_thread_sleep(SAMPLE_SNTP_UPDATE_INTERVAL);
  }
  
  /* Time sync failed.  */
  
  /* Stop and delete SNTP.  */
  nx_sntp_client_stop(&sntp_client);
  nx_sntp_client_delete(&sntp_client);
  
  /* Return success.  */
  return(NX_NOT_SUCCESSFUL);
}

static UINT unix_time_get(ULONG *unix_time)
{
  
  /* Return number of seconds since Unix Epoch (1/1/1970 00:00:00).  */
  *unix_time =  unix_time_base + (tx_time_get() / TX_TIMER_TICKS_PER_SECOND);
  
  return(NX_SUCCESS);
}
