/**********************************************************************************************************************
 * \file    uds_main.h
 * \brief
 * \version V1.0.0
 * \date    2022Äê3ÔÂ10ÈÕ
 * \author  Administrator
 *********************************************************************************************************************/
#ifndef UDS_MAIN_H_
#define UDS_MAIN_H_

#include "uds_tp.h"
#include "uds_app.h"

void UdsInit(tUdsId xRxFunId,tUdsId xRxPhyId,tUdsId xTxId);
void UdsMainProcess(void);

#endif /* UDS_MAIN_H_ */
