#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "stm32l4xx_hal.h"  
#include "stm32l4s5i_iot01.h"
#include "stm32l4s5i_iot01a_tsensor.h"
#include "stm32l4s5i_iot01a_hsensor.h"
#include "stm32l4s5i_iot01a_psensor.h"
#include "stm32l4s5i_iot01a_proximity.h"
#include "stm32l4s5i_iot01a_accelero.h"
#include "stm32l4s5i_iot01a_gyro.h"
#include "stm32l4s5i_iot01a_magneto.h"
#include "wifi.h"

#include "azure_customizations.h"
#include "MetaDataManager.h"

extern ES_WIFIObject_t    EsWifiObj;
UART_HandleTypeDef UartHandle;

#define WIFI_FAIL 1
#define WIFI_OK   0

extern  SPI_HandleTypeDef hspi;

#define REG32(x) (*(volatile unsigned int *)(x))


/* Define RCC register.  */
#define STM32L4_RCC                         0x40021000
#define STM32L4_RCC_AHB2ENR                 REG32(STM32L4_RCC + 0x4C)
#define STM32L4_RCC_AHB2ENR_RNGEN           0x00040000

/* Define RNG registers.  */
#define STM32_RNG                           0x50060800
#define STM32_RNG_CR                        REG32(STM32_RNG + 0x00)
#define STM32_RNG_SR                        REG32(STM32_RNG + 0x04)
#define STM32_RNG_DR                        REG32(STM32_RNG + 0x08)

#define STM32_RNG_CR_RNGEN                  0x00000004
#define STM32_RNG_CR_IE                     0x00000008
#define STM32_RNG_CR_CED                    0x00000020

#define STM32_RNG_SR_DRDY                   0x00000001
#define STM32_RNG_SR_CECS                   0x00000002
#define STM32_RNG_SR_SECS                   0x00000004
#define STM32_RNG_SR_CEIS                   0x00000020
#define STM32_RNG_SR_SEIS                   0x00000040


typedef struct {
   char SSID[64];
   char Password[64];
   WIFI_Ecn_t ecnWiFi;
} WIFI_CredAcc_t;

typedef enum
{
  WS_IDLE = 0,
  WS_CONNECTED,
  WS_DISCONNECTED,
  WS_ERROR,
} WebServerState_t;

static volatile int32_t UserButtonPressed =0;

extern SPI_HandleTypeDef hspi_wifi;

AzureConnection_t AzureConnectionInfo;


static void Init_MEMS_Sensors(void);

void hardware_rand_initialize(void)
{
  /* Enable clock for the RNG.  */
  STM32L4_RCC_AHB2ENR |= STM32L4_RCC_AHB2ENR_RNGEN;
  
  /* Enable the random number generator.  */
  STM32_RNG_CR = STM32_RNG_CR_RNGEN;
}

/**
  * @brief  Gets the page of a given address
  * @param  Addr: Address of the FLASH Memory
  * @retval The page of a given address
  */
static uint32_t GetPage(uint32_t Addr)
{
  uint32_t page = 0;

  if (Addr < (FLASH_BASE + FLASH_BANK_SIZE)) {
    /* Bank 1 */
    page = (Addr - FLASH_BASE) / FLASH_PAGE_SIZE;
  } else {
    /* Bank 2 */
    page = (Addr - (FLASH_BASE + FLASH_BANK_SIZE)) / FLASH_PAGE_SIZE;
  }
  return page;
}

/**
  * @brief  Gets the bank of a given address
  * @param  Addr: Address of the FLASH Memory
  * @retval The bank of a given address
  */
static uint32_t GetBank(uint32_t Addr)
{
  uint32_t bank = 0;

  if (READ_BIT(SYSCFG->MEMRMP, SYSCFG_MEMRMP_FB_MODE) == 0){
    /* No Bank swap */
    if (Addr < (FLASH_BASE + FLASH_BANK_SIZE)) {
      bank = FLASH_BANK_1;
    } else {
      bank = FLASH_BANK_2;
    }
  } else {
    /* Bank swap */
    if (Addr < (FLASH_BASE + FLASH_BANK_SIZE)) {
      bank = FLASH_BANK_2;
    } else {
      bank = FLASH_BANK_1;
    }
  }
  return bank;
}

/**
 * @brief User function for Erasing the MDM on Flash
 * @param None
 * @retval uint32_t Success/NotSuccess [1/0]
 */
uint32_t UserFunctionForErasingFlash(void) {
  FLASH_EraseInitTypeDef EraseInitStruct;
  uint32_t SectorError = 0;
  uint32_t Success=1;

  EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
  EraseInitStruct.Banks       = GetBank(MDM_FLASH_ADD);
  EraseInitStruct.Page        = GetPage(MDM_FLASH_ADD);

  EraseInitStruct.NbPages     = 1; /* Each page is 4k */


  /* Unlock the Flash to enable the flash control register access *************/
  HAL_FLASH_Unlock();
  

   /* Clear OPTVERR bit set on virgin samples */
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR);
  /* Clear PEMPTY bit set (as the code is executed from Flash which is not empty) */
  if (__HAL_FLASH_GET_FLAG(FLASH_FLAG_PEMPTY) != 0) {
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_PEMPTY);
  }


  if(HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError) != HAL_OK){
    /* Error occurred while sector erase.
      User can add here some code to deal with this error.
      SectorError will contain the faulty sector and then to know the code error on this sector,
      user can call function 'HAL_FLASH_GetError()'
      FLASH_ErrorTypeDef errorcode = HAL_FLASH_GetError(); */
    Success=0;
    AZURE_PRINTF("MetaDataManager Flash sector erase error\r\n");
    while(1);
  }

  /* Lock the Flash to disable the flash control register access (recommended
  to protect the FLASH memory against possible unwanted operation) *********/
  HAL_FLASH_Lock();

  return Success;
}

/**
 * @brief User function for Saving the MDM  on the Flash
 * @param void * InitMetaDataVector Pointer to the MDM beginning
 * @param void * EndMetaDataVector Pointer to the MDM end
 * @retval uint32_t Success/NotSuccess [1/0]
 */
uint32_t UserFunctionForSavingFlash(void *InitMetaDataVector,void *EndMetaDataVector)
{
  uint32_t Success=1;

  /* Store in Flash Memory */
  uint32_t Address = MDM_FLASH_ADD;
  uint64_t *WriteIndex;

  /* Unlock the Flash to enable the flash control register access *************/
  HAL_FLASH_Unlock();
  for(WriteIndex =((uint64_t *) InitMetaDataVector); WriteIndex<((uint64_t *) EndMetaDataVector); WriteIndex++) {
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, Address,*WriteIndex) == HAL_OK){
      Address = Address + 8;
    } else {
      /* Error occurred while writing data in Flash memory.
         User can add here some code to deal with this error
         FLASH_ErrorTypeDef errorcode = HAL_FLASH_GetError(); */
      Success =0;
      while(1);
    }
  }

  /* Lock the Flash to disable the flash control register access (recommended
   to protect the FLASH memory against possible unwanted operation) *********/
  HAL_FLASH_Lock();
 
  return Success;
}

int hardware_rand(void)
{
  /* Wait for data ready.  */
  while((STM32_RNG_SR & STM32_RNG_SR_DRDY) == 0);
  
  /* Return the random number.  */
  return STM32_RNG_DR;
}

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follows :
  *            System Clock source            = PLL (MSI MSE)
  *            SYSCLK(Hz)                     = 80000000
  *            HCLK(Hz)                       = 80000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 1
  *            APB2 Prescaler                 = 1
  *            MSI Frequency(Hz)              = 48000000
  *            PLL_M                          = 6
  *            PLL_N                          = 20
  *            PLL_R                          = 2
  *            PLL_P                          = 7
  *            PLL_Q                          = 2
  *            Flash Latency(WS)              = 4
  * @param  None
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /**Configure LSE Drive Capability 
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);
  /**Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSE|RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_11;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 6;
  RCC_OscInitStruct.PLL.PLLN = 20;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    while(1);
  }
  /**Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    while(1);
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1|RCC_PERIPHCLK_USART3
                              |RCC_PERIPHCLK_I2C2|RCC_PERIPHCLK_DFSDM1
                              |RCC_PERIPHCLK_USB|RCC_PERIPHCLK_RNG;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
  PeriphClkInit.Usart3ClockSelection = RCC_USART3CLKSOURCE_PCLK1;
  PeriphClkInit.I2c2ClockSelection = RCC_I2C2CLKSOURCE_PCLK1;
  PeriphClkInit.Dfsdm1ClockSelection = RCC_DFSDM1CLKSOURCE_PCLK;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLLSAI1;
  PeriphClkInit.RngClockSelection = RCC_RNGCLKSOURCE_PLLSAI1;
  PeriphClkInit.PLLSAI1.PLLSAI1Source = RCC_PLLSOURCE_MSI;
  PeriphClkInit.PLLSAI1.PLLSAI1M = 6;
  PeriphClkInit.PLLSAI1.PLLSAI1N = 12;
  PeriphClkInit.PLLSAI1.PLLSAI1P = RCC_PLLP_DIV7;
  PeriphClkInit.PLLSAI1.PLLSAI1Q = RCC_PLLQ_DIV2;
  PeriphClkInit.PLLSAI1.PLLSAI1R = RCC_PLLR_DIV2;
  PeriphClkInit.PLLSAI1.PLLSAI1ClockOut = RCC_PLLSAI1_48M2CLK;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    while(1);
  }
  /**Configure the main internal regulator output voltage 
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    while(1);
  }
  /**Enable MSI Auto calibration 
  */
  HAL_RCCEx_EnableMSIPLLMode();
}

/**
* @brief This function handles EXTI line[15:10] interrupts.
*/
void EXTI15_10_IRQHandler(void)
{
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_13);
}

int  board_setup(void)
{
  WIFI_CredAcc_t WiFiCred;
  
  /* Enable execution profile.  */
  CoreDebug -> DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT -> CTRL |= DWT_CTRL_CYCCNTENA_Msk;

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();
  
  /* Configure the system clock */
  SystemClock_Config();

  UartHandle.Init.BaudRate       = 115200;
  UartHandle.Init.WordLength     = UART_WORDLENGTH_8B;
  UartHandle.Init.StopBits       = UART_STOPBITS_1;
  UartHandle.Init.Parity         = UART_PARITY_NONE;
  UartHandle.Init.HwFlowCtl      = UART_HWCONTROL_NONE;
  UartHandle.Init.Mode           = UART_MODE_TX_RX;
  UartHandle.Init.OverSampling   = UART_OVERSAMPLING_16;
  UartHandle.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  BSP_COM_Init(COM1, &UartHandle);

  /* Initialize the hardware random number generator.  */
  hardware_rand_initialize();
  
  srand(hardware_rand());

  /* Configure TIM8 for LED flashing.  */
  //TIM8_Init();
  
  /* Configure push button.  */
  BSP_PB_Init(BUTTON_USER, BUTTON_MODE_EXTI);
  
  Init_MEMS_Sensors();
  
  // Configure the Board's LEDs
  BSP_LED_Init(LED_GREEN);

  //Read the data from Flash, if they are present or set the default values
  {
    int32_t NecessityToSaveMMD=0;
    int32_t ChangeDefaultValue=0;
    
    MDM_knownGMD_t known_MetaData[]={
      {GMD_WIFI,sizeof(WIFI_CredAcc_t)},
      {GMD_AZURE,sizeof(AzureConnection_t)},
      {GMD_END    ,0}/* THIS MUST BE THE LAST ONE */
    };
    
    /* Initialize the MetaDataManager */
    InitMetaDataManager((void *)&known_MetaData,MDM_DATA_TYPE_GMD,NULL);
    
    /* Reset the Values */
    memset(&WiFiCred,0,sizeof(WIFI_CredAcc_t));
    memset(&AzureConnectionInfo,0,sizeof(AzureConnection_t));

    /* Recall the saved information */
    MDM_ReCallGMD(GMD_WIFI,(void *)&WiFiCred);
    MDM_ReCallGMD(GMD_AZURE,(void *)&AzureConnectionInfo);
    
    AZURE_PRINTF("--------------------------\r\n");
    AZURE_PRINTF("|    WIFI Credential     |\r\n");
    AZURE_PRINTF("--------------------------\r\n");
    
    if(WiFiCred.SSID[0]!=0) {
      AZURE_PRINTF("\tSaved SSID   : %s\r\n",WiFiCred.SSID);
      AZURE_PRINTF("\tSaved PassWd : %s\r\n",WiFiCred.Password);
      AZURE_PRINTF("\tSaved EncMode: ");
      switch(WiFiCred.ecnWiFi) {
        case   WIFI_ECN_OPEN:
          AZURE_PRINTF("Open\r\n");
          break;
        case   WIFI_ECN_WEP:
          AZURE_PRINTF("WEP\r\n");
          break;
        case   WIFI_ECN_WPA_PSK:
          AZURE_PRINTF("WPA\r\n");
          break;
        case   WIFI_ECN_WPA2_PSK:
          AZURE_PRINTF("WPA2\r\n");
          break;
        case   WIFI_ECN_WPA_WPA2_PSK:
          AZURE_PRINTF("WPA/WPA2\r\n");
          break;
        }
    } else {
      ChangeDefaultValue=1;
    }
    
    /* Read the WIFI Credential from UART */
    if(ChangeDefaultValue==0) {
      AZURE_PRINTF("Press the User Button in 3 seconds for changing it\r\n");
    }
    {

      int32_t CountOn,CountOff;
      for(CountOn=0;((CountOn<3) & (ChangeDefaultValue==0));CountOn++) {
        BSP_LED_On(LED_GREEN);
        for(CountOff=0;((CountOff<10) & (ChangeDefaultValue==0));CountOff++) {
          if(CountOff==2) {
            BSP_LED_Off(LED_GREEN);
          }
          /* Handle user button */
          if(UserButtonPressed) {
            UserButtonPressed=0;
            ChangeDefaultValue=1;
          }
          HAL_Delay(100);
        }
      }
      if(ChangeDefaultValue) {
        char console_input[128];
  #define AZURE_SCAN_WITH_BACKSPACE()\
        {\
          scanf("%s",console_input);\
          int32_t IndexRead;\
          int32_t IndexWrite=0;\
          \
          for(IndexRead=0;((IndexRead<128) & (console_input[IndexRead]!='\0')) ;IndexRead++) {\
            if(console_input[IndexRead]!='\b') {\
              console_input[IndexWrite] =console_input[IndexRead];\
              IndexWrite++;\
            } else {\
              if(IndexWrite>0) {\
                IndexWrite--;\
              }\
            }\
          }\
          /* Add the termination string */\
          console_input[IndexWrite]='\0';\
        }
        ChangeDefaultValue=0;
        if(WiFiCred.SSID[0]!=0) {
          AZURE_PRINTF("\r\nDo you want to change them?(y/n)\r\n");
          AZURE_SCAN_WITH_BACKSPACE();
        } else {
          console_input[0]='y';
        }
        if(console_input[0]=='y') {
          NecessityToSaveMMD=1;
          AZURE_PRINTF("\r\nEnter the SSID:\r\n");
          AZURE_SCAN_WITH_BACKSPACE();
          sprintf(WiFiCred.SSID,"%s",console_input);
          AZURE_PRINTF("\r\nEnter the PassWd:\r\n");
          AZURE_SCAN_WITH_BACKSPACE();
          sprintf(WiFiCred.Password,"%s",console_input);
          AZURE_PRINTF("\r\nEnter the encryption mode(0:Open, 1:WEP, 2:WPA2/WPA2):\r\n");
          AZURE_SCAN_WITH_BACKSPACE();
          switch(console_input[0]){
            case '0':
              WiFiCred.ecnWiFi = WIFI_ECN_OPEN;
            break;
            case '1':
              WiFiCred.ecnWiFi = WIFI_ECN_WEP;
            break;
            case '2':
              WiFiCred.ecnWiFi = WIFI_ECN_WPA2_PSK;
            break;
          }
          
          AZURE_PRINTF("New     SSID  : [%s]\r\n",WiFiCred.SSID);
          AZURE_PRINTF("        PassWd: [%s]\r\n",WiFiCred.Password);
          AZURE_PRINTF("       EncMode: ");
          switch(WiFiCred.ecnWiFi) {
            case   WIFI_ECN_OPEN:
              AZURE_PRINTF("Open\r\n");
              break;
            case   WIFI_ECN_WEP:
              AZURE_PRINTF("WEP\r\n");
              break;
            case   WIFI_ECN_WPA_PSK:
              AZURE_PRINTF("WPA\r\n");
              break;
            case   WIFI_ECN_WPA2_PSK:
              AZURE_PRINTF("WPA2\r\n");
              break;
            case   WIFI_ECN_WPA_WPA2_PSK:
              AZURE_PRINTF("WPA/WPA2\r\n");
              break;
            }
          /* Update the MetaDataManager */
          MDM_SaveGMD(GMD_WIFI,(void *)&WiFiCred);
        }
      }
    }
    
    AZURE_PRINTF("-----------------------------\r\n");
    
    if(AzureConnectionInfo.ScopeID[0]!=0) {
       AZURE_PRINTF("Saved   ScopeID: [%s]\r\n",AzureConnectionInfo.ScopeID);
       AZURE_PRINTF("       DeviceID: [%s]\r\n",AzureConnectionInfo.DeviceID);
       AZURE_PRINTF("     PrimaryKey: [%s]\r\n",AzureConnectionInfo.PrimaryKey);
    } else {
      ChangeDefaultValue=1;
    }
    
    /* Read the Connection values from UART */
    if(ChangeDefaultValue==0) {
      AZURE_PRINTF("Press the User Button in 3 seconds for changing it\r\n");
    }
    {

      int32_t CountOn,CountOff;
      for(CountOn=0;((CountOn<3) & (ChangeDefaultValue==0));CountOn++) {
        BSP_LED_On(LED_GREEN);
        for(CountOff=0;((CountOff<10) & (ChangeDefaultValue==0));CountOff++) {
          if(CountOff==2) {
            BSP_LED_Off(LED_GREEN);
          }
          /* Handle user button */
          if(UserButtonPressed) {
            UserButtonPressed=0;
            ChangeDefaultValue=1;
          }
          HAL_Delay(100);
        }
      }
      if(ChangeDefaultValue) {
        char console_input[128];
  #define AZURE_SCAN_WITH_BACKSPACE()\
        {\
          scanf("%s",console_input);\
          int32_t IndexRead;\
          int32_t IndexWrite=0;\
          \
          for(IndexRead=0;((IndexRead<128) & (console_input[IndexRead]!='\0')) ;IndexRead++) {\
            if(console_input[IndexRead]!='\b') {\
              console_input[IndexWrite] =console_input[IndexRead];\
              IndexWrite++;\
            } else {\
              if(IndexWrite>0) {\
                IndexWrite--;\
              }\
            }\
          }\
          /* Add the termination string */\
          console_input[IndexWrite]='\0';\
        }
        ChangeDefaultValue=0;
       
        if(AzureConnectionInfo.ScopeID[0]!=0) {
          AZURE_PRINTF("\r\nDo you want to change them?(y/n)\r\n");
          AZURE_SCAN_WITH_BACKSPACE();
        } else {
          console_input[0]='y';
        }
        if(console_input[0]=='y') {
          NecessityToSaveMMD=1;      
          AZURE_PRINTF("\r\nEnter the Scope ID:\r\n");
          AZURE_SCAN_WITH_BACKSPACE();
          sprintf((char *)AzureConnectionInfo.ScopeID,"%s",console_input);
          AZURE_PRINTF("\r\nEnter the Device ID:\r\n");
          AZURE_SCAN_WITH_BACKSPACE();
          sprintf((char *)AzureConnectionInfo.DeviceID,"%s",console_input);
          AZURE_PRINTF("\r\nEnter the Primary Key:\r\n");
          AZURE_SCAN_WITH_BACKSPACE();
          sprintf((char *)AzureConnectionInfo.PrimaryKey,"%s",console_input);
          
          AZURE_PRINTF("New     ScopeID: [%s]\r\n",AzureConnectionInfo.ScopeID);
          AZURE_PRINTF("       DeviceID: [%s]\r\n",AzureConnectionInfo.DeviceID);
          AZURE_PRINTF("     PrimaryKey: [%s]\r\n",AzureConnectionInfo.PrimaryKey);
          /* Update the MetaDataManager */
          MDM_SaveGMD(GMD_AZURE,(void *)&AzureConnectionInfo);
        }
      }
    }
    
    AZURE_PRINTF("-----------------------------\r\n");

    /* Save the MetaDataManager in Flash if it's necessary */
    if(NecessityToSaveMMD) {
      EraseMetaDataManager();
      SaveMetaDataManager();
    }
    
  }
  
  
  
  /*Initialize and use WIFI module */
  if(WIFI_Init() ==  WIFI_STATUS_OK)
  {
    uint8_t  MAC_Addr[6];
    uint8_t  IP_Addr[4]; 

    uint8_t  Gateway_Addr[4];
    uint8_t  DNS1_Addr[4]; 
    uint8_t  DNS2_Addr[4];   
    /* Lib info.  */
    uint32_t hal_version = HAL_GetHalVersion();
    uint32_t bsp_version = BSP_GetVersion();
      
    AZURE_PRINTF("STM32L4XX Lib:\r\n");
    AZURE_PRINTF("\t- CMSIS Device Version: %d.%d.%d.%d.\r\n", __STM32L4_CMSIS_VERSION_MAIN, __STM32L4_CMSIS_VERSION_SUB1, __STM32L4_CMSIS_VERSION_SUB2, __STM32L4_CMSIS_VERSION_RC);
    AZURE_PRINTF("\t- HAL Driver Version: %d.%d.%d.%d.\r\n", ((hal_version >> 24U) & 0xFF), ((hal_version >> 16U) & 0xFF), ((hal_version >> 8U) & 0xFF), (hal_version & 0xFF));
    AZURE_PRINTF("\t- BSP Driver Version: %d.%d.%d.%d.\r\n", ((bsp_version >> 24U) & 0xFF), ((bsp_version >> 16U) & 0xFF), ((bsp_version >> 8U) & 0xFF), (bsp_version & 0xFF));
    AZURE_PRINTF("\r\n");
    
    /* ES-WIFI info.  */
    AZURE_PRINTF("ES-WIFI Firmware:\r\n");
    AZURE_PRINTF("\t- Product Name: %s\r\n", EsWifiObj.Product_Name);
    AZURE_PRINTF("\t- Product ID: %s\r\n", EsWifiObj.Product_ID);
    AZURE_PRINTF("\t- Firmware Version: %s\r\n", EsWifiObj.FW_Rev);
    AZURE_PRINTF("\t- API Version: %s\r\n", EsWifiObj.API_Rev);
    AZURE_PRINTF("\r\n"); 
    
    if(WIFI_GetMAC_Address(MAC_Addr) == WIFI_STATUS_OK)
    {       
      AZURE_PRINTF("ES-WIFI MAC Address: %X:%X:%X:%X:%X:%X\r\n\n",     
             MAC_Addr[0],
             MAC_Addr[1],
             MAC_Addr[2],
             MAC_Addr[3],
             MAC_Addr[4],
             MAC_Addr[5]);   
    }
    else
    { 
      AZURE_PRINTF("!!!ERROR: ES-WIFI Get MAC Address Failed.\r\n"); 
      return WIFI_FAIL;
    }
    
    if( WIFI_Connect(WiFiCred.SSID, WiFiCred.Password, WiFiCred.ecnWiFi) == WIFI_STATUS_OK)
    { 
      AZURE_PRINTF("ES-WIFI Connected:\r\n");
      
      if(WIFI_GetIP_Address(IP_Addr) == WIFI_STATUS_OK)
      {
        AZURE_PRINTF("\t- ES-WIFI IP Address: %d.%d.%d.%d\r\n",     
               IP_Addr[0],
               IP_Addr[1],
               IP_Addr[2],
               IP_Addr[3]); 
        
        if(WIFI_GetGateway_Address(Gateway_Addr) == WIFI_STATUS_OK)
        {
          AZURE_PRINTF("\t- ES-WIFI Gateway Address: %d.%d.%d.%d\r\n",     
                 Gateway_Addr[0],
                 Gateway_Addr[1],
                 Gateway_Addr[2],
                 Gateway_Addr[3]); 
        }
        
        if(WIFI_GetDNS_Address(DNS1_Addr,DNS2_Addr) == WIFI_STATUS_OK)
        {
          AZURE_PRINTF("\t- ES-WIFI DNS1 Address: %d.%d.%d.%d\r\n",     
                 DNS1_Addr[0],
                 DNS1_Addr[1],
                 DNS1_Addr[2],
                 DNS1_Addr[3]); 
          
          AZURE_PRINTF("\t- ES-WIFI DNS2 Address: %d.%d.%d.%d\r\n",     
                 DNS2_Addr[0],
                 DNS2_Addr[1],
                 DNS2_Addr[2],
                 DNS2_Addr[3]);           
        }        
        
        if((IP_Addr[0] == 0)&& 
           (IP_Addr[1] == 0)&& 
           (IP_Addr[2] == 0)&&
           (IP_Addr[3] == 0))
          return WIFI_FAIL;  
      }
      else
      {     
        AZURE_PRINTF("!!!ERROR: ES-WIFI Get IP Address Failed.\r\n");
        return WIFI_FAIL;
      }
	  
	  AZURE_PRINTF("\r\n");
    }
    else
    {   
		AZURE_PRINTF("!!!ERROR: ES-WIFI NOT connected.\r\n");
        return WIFI_FAIL;
    }
  }
  else
  {    
    AZURE_PRINTF("!!!ERROR: ES-WIFI Initialize Failed.\r\n"); 
    return WIFI_FAIL;
  }
  
  return WIFI_OK;
}

/** @brief Initialize all the MEMS1 sensors
* @param None
* @retval None
*/
static void Init_MEMS_Sensors(void)
{
  AZURE_PRINTF("Code compiled for B-L4S5I-IOT01A board\r\n");
  
  BSP_TSENSOR_Init();
  BSP_HSENSOR_Init();
  BSP_PSENSOR_Init();
  BSP_Proximity_Init();
  BSP_ACCELERO_Init();
  BSP_GYRO_Init();
  BSP_MAGNETO_Init();
}

/** @brief Sends a character to serial port
* @param ch Character to send
* @retval Character sent
*/
int uartSendChar(int ch)
{
  HAL_UART_Transmit(&UartHandle, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
  return ch;
}

/** @brief Receives a character from serial port
* @param None
* @retval Character received
*/
int uartReceiveChar(void)
{
  uint8_t ch;
  HAL_UART_Receive(&UartHandle, &ch, 1, HAL_MAX_DELAY);
  
  /* Echo character back to console */
  HAL_UART_Transmit(&UartHandle, &ch, 1, HAL_MAX_DELAY);
  
  /* And cope with Windows */
  if(ch == '\r'){
    uint8_t ret = '\n';
    HAL_UART_Transmit(&UartHandle, &ret, 1, HAL_MAX_DELAY);
  }
  
  return ch;
}

#if (defined(__ICCARM__))
size_t __write(int handle, const unsigned char *buf, size_t bufSize)
{
  int i;
  /* Check for the command to flush all handles */
  if (handle == -1) 
  {    
    return 0;  
  }    
  /* Check for stdout and stderr      (only necessary if FILE descriptors are enabled.) */  
  if (handle != 1 && handle != 2)  
  {    
    return -1;  
  }   
  
  for(i=0; i< bufSize; i++)
    uartSendChar(buf[i]);
  
  return bufSize;
}

size_t __read(int handle, unsigned char *buf, size_t bufSize)
{  
  int i;
  /* Check for stdin      (only necessary if FILE descriptors are enabled) */ 
  if (handle != 0)  
  {    
    return -1;  
  }   
  
  for(i=0; i<bufSize; i++)
    buf[i] = uartReceiveChar();
  return bufSize;
}
#endif /* __ICCARM__ */

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
  /* UART2 for Console Messagges */
  GPIO_InitTypeDef  GPIO_InitStruct;
  
  /* Enable GPIO TX/RX clock */
  __GPIOD_CLK_ENABLE();
  
  /* Enable USARTx clock */
  __USART2_CLK_ENABLE();
  
  /* UART TX GPIO pin configuration  */
  GPIO_InitStruct.Pin       = GPIO_PIN_5;
  GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull      = GPIO_PULLUP;
  GPIO_InitStruct.Speed     = GPIO_SPEED_FAST;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
  
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
  
  /* UART RX GPIO pin configuration  */
  GPIO_InitStruct.Pin = GPIO_PIN_6;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
  
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
}

void user_button_callback()
{
  UserButtonPressed=1;
}

void EXTI1_IRQHandler(void)
{
 HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_1);
}

void SPI3_IRQHandler(void)
{
  HAL_SPI_IRQHandler(&hspi);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  switch (GPIO_Pin)
  {
    case (GPIO_PIN_1):
    {
      SPI_WIFI_ISR();
      break;
    }
    case (USER_BUTTON_PIN):
    {
      user_button_callback();
      break;
    }
    default:
    {
      break;
    }
  }
}
