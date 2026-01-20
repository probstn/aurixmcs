#include "IfxGtm.h"
#include "IfxGtm_Cmu.h"
#include "IfxGtm_PinMap.h"

#define CLK_TOUT_PIN   IfxGtm_ATOM0_0_TOUT8_P02_8_OUT
#define ATOM_IDX       (CLK_TOUT_PIN.atom)
#define ATOM_CH        (CLK_TOUT_PIN.channel)

/* From TC3xx UM table: MCS0_WRADDR[0] = 0x077 */
#define MCS0_WRADDR0   (0x077u)

/* Slow first */
#define CLK_PERIOD_TICKS   (100u)
#define CLK_DUTY_50        (CLK_PERIOD_TICKS/2u)

static Ifx_GTM *gtm = &MODULE_GTM;

static void gtmClockInit(void)
{
    IfxGtm_enable(gtm);
    float32 f = IfxGtm_Cmu_getModuleFrequency(gtm);
    IfxGtm_Cmu_setGclkFrequency(gtm, f);
    IfxGtm_Cmu_setClkFrequency(gtm, IfxGtm_Cmu_Clk_0, f);
    IfxGtm_Cmu_enableClocks(gtm, IFXGTM_CMU_CLKEN_CLK0);
}

static void atomClk_withAruInit(void)
{
    Ifx_GTM_ATOM *atom = &gtm->ATOM[ATOM_IDX];
    Ifx_GTM_ATOM_CH *ch = &atom->CH0;
    Ifx_GTM_ATOM_AGC *agc = &atom->AGC;

    /* 1. Route TOUT */
    IfxGtm_PinMap_setAtomTout(&CLK_TOUT_PIN,
                             IfxPort_OutputMode_pushPull,
                             IfxPort_PadDriver_cmosAutomotiveSpeed1);

   /* 2. Configure for SOMP mode but KEEP ARU DISABLED initially [cite: 17, 1459] */
    /* Mode=SOMP (0x2), ARU_EN=0 */
    ch->CTRL.U = (2u << 0);

    /* 3. Write initial period/duty to Shadow Registers */
    ch->SR0.U = CLK_PERIOD_TICKS;
    ch->SR1.U = 0u;

    /* 4. Set the ARU Read Address */
    ch->RDADDR.U = MCS0_WRADDR0;

    /* 5. Enable Channel and Output */
    IfxGtm_Atom_Agc_enableChannelUpdate(agc, ATOM_CH, TRUE);
    IfxGtm_Atom_Agc_enableChannel(agc, ATOM_CH, TRUE, FALSE);
    IfxGtm_Atom_Agc_enableChannelOutput(agc, ATOM_CH, TRUE, FALSE);

    /* 6. FORCE UPDATE [cite: 1459]
     * This moves SR0 (200) -> CM0 and SR1 (0) -> CM1.
     * Now the ATOM has a valid period and starts counting.
     */
    IfxGtm_Atom_Agc_trigger(agc);

    /* 7. NOW Enable ARU Input [cite: 1459]
     * The ATOM is currently running. When CN0 hits 200, it will
     * automatically request new data from the ARU address 0x77.
     */
    ch->CTRL.B.ARU_EN = 1u;
}
void atomCpuPwmTest(void)
{
    Ifx_GTM_ATOM *atom = &MODULE_GTM.ATOM[ATOM_IDX];
    Ifx_GTM_ATOM_CH *ch = &atom->CH0;
    Ifx_GTM_ATOM_AGC *agc = &atom->AGC;

    /* SOMP, no ARU */
    ch->CTRL.U = (2u << 0) + (0u << 11) + (0u << 12);  /* SOMP, SL low, CLK0 */

    ch->SR0.U = CLK_PERIOD_TICKS;
    ch->SR1.U = CLK_DUTY_50;

    IfxGtm_Atom_Agc_enableChannelUpdate(agc, ATOM_CH, TRUE);
    IfxGtm_Atom_Agc_enableChannel(agc, ATOM_CH, TRUE, FALSE);
    IfxGtm_Atom_Agc_enableChannelOutput(agc, ATOM_CH, TRUE, FALSE);
    IfxGtm_Atom_Agc_trigger(agc);
}


void initAtom(void)
{
    gtmClockInit();
    atomClk_withAruInit();
}
