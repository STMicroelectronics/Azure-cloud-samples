#ifndef AZURE_CUSTOMIZATION_H
#define AZURE_CUSTOMIZATION_H

#ifdef __cplusplus
extern   "C" {
#endif
  
/*********************** Don't change the following Section *******************/
#include "wifi.h"
  
#define AZURE_ENABLE_PRINTF

#ifdef AZURE_ENABLE_PRINTF
  #define AZURE_PRINTF(...) printf(__VA_ARGS__)
#else /* AZURE_ENABLE_PRINTF */
  #define AZURE_PRINTF(...)
#endif /* AZURE_ENABLE_PRINTF */
  
/**
 * @brief  Azure's IoT Central Information
 */
typedef struct
{
  char ScopeID[64];
  char DeviceID[128];
  char PrimaryKey[128];
} AzureConnection_t;


/**
 * @brief  Wi-Fi Credential
 */
typedef struct {
   char SSID[64];
   char Password[64];
   WIFI_Ecn_t ecnWiFi;
} WIFI_CredAcc_t;


/**
 * @brief  Global structure for saving Data on Flash
 */
typedef struct {
  uint32_t DataInitialized;
  AzureConnection_t AzureConnectionInfo;
  WIFI_CredAcc_t WiFiCred;
} AZURE_Customization_t;

extern AZURE_Customization_t AzureCustomization;


#ifdef __cplusplus
}
#endif
#endif /* AZURE_CUSTOMIZATION_H */