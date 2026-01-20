#include "endat.h"

#include "Ifx_Types.h"
#include "IfxGtm.h"
#include "IfxGtm_Cmu.h"
#include "IfxGtm_Atom_Pwm.h"
#include "IfxGtm_PinMap.h"

/* Pick a real TOUT pin from IfxGtm_PinMap.h for your board/package */
#define GPIO_TOUT_PIN   IfxGtm_ATOM0_0_TOUT8_P02_8_OUT   /* Example only */
#define GPIO_PERIOD     1000u                              /* any small period is fine */

static Ifx_GTM *gtm = &MODULE_GTM;
static IfxGtm_Atom_Pwm_Driver g_atomDrv;

/* Keep the ATOM/channel indices handy */
static uint8 g_atomIdx;
static uint8 g_atomCh;

static void gtmClockInit(void)
{
    IfxGtm_enable(gtm);

    /* Set clocks: ATOM uses CMU CLK0 (typical) */
    float32 f = IfxGtm_Cmu_getModuleFrequency(gtm);
    IfxGtm_Cmu_setGclkFrequency(gtm, f);
    IfxGtm_Cmu_setClkFrequency(gtm, IfxGtm_Cmu_Clk_0, f);

    /* Enable the needed clocks (CLK0 for ATOM) */
    IfxGtm_Cmu_enableClocks(gtm, IFXGTM_CMU_CLKEN_CLK0);
}

static void atomAsGpioInit(void)
{
    IfxGtm_Atom_Pwm_Config cfg;
    IfxGtm_Atom_Pwm_initConfig(&cfg, gtm);

    g_atomIdx = GPIO_TOUT_PIN.atom;
    g_atomCh  = GPIO_TOUT_PIN.channel;

    cfg.atom        = g_atomIdx;
    cfg.atomChannel = g_atomCh;

    /* We run PWM mode but use it as a static output by forcing duty 0% or 100% */
    cfg.period      = GPIO_PERIOD;

    /* Start LOW: 0% duty */
    cfg.dutyCycle   = 0u;

    /* Pin routing */
    cfg.pin.outputPin = &GPIO_TOUT_PIN;

    /* Use synchronous updates (writes go to SRx; applied by AGC trigger) */
    cfg.synchronousUpdateEnabled = TRUE;
    cfg.immediateStartEnabled    = TRUE;

    IfxGtm_Atom_Pwm_init(&g_atomDrv, &cfg);

    /* Ensure channel is enabled and output is enabled via AGC */
    {
        Ifx_GTM_ATOM *atom = &gtm->ATOM[g_atomIdx];
        Ifx_GTM_ATOM_AGC *agc = &atom->AGC;

        /* Enable update + enable channel + enable output */
        IfxGtm_Atom_Agc_enableChannelUpdate(agc, g_atomCh, TRUE);
        IfxGtm_Atom_Agc_enableChannel(agc, g_atomCh, TRUE, FALSE);
        IfxGtm_Atom_Agc_enableChannelOutput(agc, g_atomCh, TRUE, FALSE);

        /* Apply */
        IfxGtm_Atom_Agc_trigger(agc);
    }
}

void atomOut_setState(boolean state)
{
    g_atomDrv.atom->CH0.SR1.U = state ? GPIO_PERIOD : 0;     /* duty ~ 100% (see note below) */
}


void endatInit(void)
{
    gtmClockInit();
    atomAsGpioInit();
}
