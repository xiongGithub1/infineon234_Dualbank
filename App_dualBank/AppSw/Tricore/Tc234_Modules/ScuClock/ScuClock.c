/******************************************************************************/
/*----------------------------------Includes----------------------------------*/
/******************************************************************************/

#include <AppSw/Tricore/Tc234_Modules/ScuClock/ScuClock.h>
#include <stdio.h>
#include "Cpu0_Main.h"
#include <Scu/Std/IfxScuCcu.h>
#include "ScuClock.h"

/******************************************************************************/
/*-----------------------------------Macros-----------------------------------*/
/******************************************************************************/

/******************************************************************************/
/*--------------------------------Enumerations--------------------------------*/
/******************************************************************************/

/******************************************************************************/
/*-----------------------------Data Structures--------------------------------*/
/******************************************************************************/

/******************************************************************************/
/*------------------------------Global variables------------------------------*/
/******************************************************************************/
App_ScuClock g_ScuClock; /**< \brief Scu Clock global data */




/******************************************************************************/
/*-------------------------Function Prototypes--------------------------------*/
/******************************************************************************/

/******************************************************************************/
/*------------------------Private Variables/Constants-------------------------*/
/******************************************************************************/
/** \brief Default configuration for the PLL initial steps
 */
static const IfxScuCcu_PllStepsConfig IfxScuCcu_testPllConfigSteps[] = {
    IFXSCU_CFG_PLL_STEPS
};

/** \brief Default configuration for the Clock Configuration
 */
IfxScuCcu_Config  IfxScuCcu_testClockConfig = {
    {
        sizeof(IfxScuCcu_testPllConfigSteps) / sizeof(IfxScuCcu_PllStepsConfig),
        (IfxScuCcu_PllStepsConfig *)IfxScuCcu_testPllConfigSteps,
        IFXSCU_CFG_PLL_INITIAL_STEP,
    },
    IFXSCU_CFG_CLK_DISTRIBUTION,
    IFXSCU_CFG_FLASH_WAITSTATE,
	IFX_CFG_SCU_XTAL_FREQUENCY
};

/******************************************************************************/
/*-------------------------Function Implementations---------------------------*/
/******************************************************************************/

/** \brief Demo init API
 *
 * This function is called from main during initialization phase
 */
void IfxScuClock_init(void)
{
    /* ensure that XTAL1 configured for X MHz */

    /* NOTE: Don't add any printf before the clock initialization. Asc won't be initialized properly. */

    /* standard PLL initialisation */
    IfxScuCcu_init(&IfxScuCcu_testClockConfig);

    g_ScuClock.pllfreq = IfxScuCcu_getPllFrequency();
    g_ScuClock.cpufreq = IfxScuCcu_getCpuFrequency(IfxCpu_getCoreIndex());
    g_ScuClock.spbfreq = IfxScuCcu_getSpbFrequency();
    g_ScuClock.gtmfreq = IfxScuCcu_getGtmFrequency();
    g_ScuClock.srifreq = IfxScuCcu_getSriFrequency();
    g_ScuClock.stmfreq = IfxScuCcu_getStmFrequency();
}


/** \brief Demo run API
 *
 * This function is called from main, background loop
 */
void IfxScuClock_run(void) {}
