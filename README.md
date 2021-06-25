# Azure-Cloud-Samples

This repository includes code samples to connect selected STM32 microcontroller development kits to [Azure IoT Plug-and-Play](https://docs.microsoft.com/en-us/azure/iot-pnp/overview-iot-plug-and-play).

The code has been adapted from original Microsoft samples, which can be downloaded [here](https://github.com/azure-rtos/samples/releases/download/rel_6.1_pnp_beta/Azure_RTOS_6.1_PnP_STM32L4+-DISCO_IAR_Sample_2021_03_18.zip).

Microsoft original code runs on STM32L4+ based B-L4S5I-IOT01A development kit; 
it uses [Azure RTOS](https://github.com/azure-rtos) and it implements the emulated temperature controller IoT Plug-and-Play example, based on Microsoft [Azure IoT embedded SDK for C](https://github.com/Azure/azure-iot-sdk-c).

The code available in this repo implements the following changes:

* reads real sensor values (temperature, humidity, acceleration, gyroscope, distance, pressure)
* implements IoT Plug-and-Play [DTDL](https://aka.ms/dtdl) device templates published in the official Azure [repository](https://github.com/Azure/iot-plugandplay-models/tree/main/dtmi/stmicroelectronics)
* runs on STM32L4+ based [B-L4S5I-IOT01A](https://www.st.com/en/evaluation-tools/b-l4s5i-iot01a.html) and [STEVAL-STWINKT1B boards](https://www.st.com/en/evaluation-tools/steval-stwinkt1b.html)
* runs on STM32U5 based [B-U585I-IOT02A](https://www.st.com/en/evaluation-tools/b-u585i-iot02a.html) board
* can take device connection settings from UART and saves them in flash, for easier provisioning

## Getting started
In order to setup development kits to connect to Azure IoT Plug-and-Play, please follow the guides available [here]().

## IoT Plug-and-Play Device Templates

Device templates use DTDLv2 and include two components: the standard [Azure device info](https://github.com/Azure/iot-plugandplay-models/blob/main/dtmi/azure/devicemanagement/deviceinformation-1.json) 
and STMicroelectronics standard interface, which demonstrates the usage of sensors.

### Telemetry
Depending on the development kit model, different sensor values are sampled periodically and sent to the Azure IoT hub that has been configured; the sampling and transmission period is a writeable property.

### Properties
Published templates enable changing the accelerometer and gyroscope full scale values by selecting from a list of sensor specific available values; moreover the telemetry sending interval can be modified as well.

### Commands
Publihed code does not implement any cloud to device commands.

## Licenses
Microsoft and STMicroelectronics are the owners of respective code, for licensing terms, please refer to the files present in the root directory of the repository.

## Support
For support about this software, customers can follow this link.
[Online Support (st.com)](https://community.st.com/s/onlinesupport)