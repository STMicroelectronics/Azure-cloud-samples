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

#ifndef DEVICEINFO_COMPONENT_H
#define DEVICEINFO_COMPONENT_H

#ifdef __cplusplus
extern   "C" {
#endif

#include "nx_azure_iot_pnp_client.h"
#include "nx_api.h"

extern UINT DeviceInfo_report_all_properties(int SendIntervalSec,
                                      UCHAR *component_name_ptr,
                                      UINT component_name_len,
                                      NX_AZURE_IOT_PNP_CLIENT *iotpnp_client_ptr);

#ifdef __cplusplus
}
#endif
#endif /* DEVICEINFO_COMPONENT_H */
