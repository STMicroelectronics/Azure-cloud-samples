#ifndef AZURE_CUSTOMIZATION_H
#define AZURE_CUSTOMIZATION_H

#ifdef __cplusplus
extern   "C" {
#endif
  
/*********************** Don't change the following Section *******************/
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
  unsigned char ScopeID[64];
  unsigned char DeviceID[128];
  unsigned char PrimaryKey[128];
} AzureConnection_t;

extern AzureConnection_t AzureConnectionInfo;

#ifdef __cplusplus
}
#endif
#endif /* AZURE_CUSTOMIZATION_H */