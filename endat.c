#include "IfxGtm.h"
#include "IfxGtm_Cmu.h"
#include "IfxGtm_Atom.h"
#include "IfxGtm_PinMap.h"
#include "endat.h"

#define CLK_PERIOD_TICKS   (100u)
#define CLK_DUTY_50        (CLK_PERIOD_TICKS/2u)

/* MCS0_WRADDR[0] ARU address (per your UM table) */
#define MCS0_WRADDR0   (0x077u)
#define MCS0_WRADDR1   (0x078u)

/* Choose TWO output pins (example: replace DATA_TOUT_PIN with your real data pin) */
#define CLK_TOUT_PIN       IfxGtm_ATOM0_0_TOUT8_P02_8_OUT
#define DATA_TOUT_PIN      IfxGtm_ATOM0_1_TOUT1_P02_1_OUT

#define CLK_ATOM           (CLK_TOUT_PIN.atom)
#define CLK_ATOM_CH        (CLK_TOUT_PIN.channel)

#define DATA_ATOM          (DATA_TOUT_PIN.atom)
#define DATA_ATOM_CH       (DATA_TOUT_PIN.channel)

static Ifx_GTM *gtm = &MODULE_GTM;

static void gtmClockInit(void)
{
    IfxGtm_enable(gtm);

    float32 f = IfxGtm_Cmu_getModuleFrequency(gtm);

    /* simplest: run GCLK and CLK0 at module frequency */
    IfxGtm_Cmu_setGclkFrequency(gtm, f);
    IfxGtm_Cmu_setClkFrequency(gtm, IfxGtm_Cmu_Clk_0, f);
    IfxGtm_Cmu_enableClocks(gtm, IFXGTM_CMU_CLKEN_CLK0);
}

static void atomClockInit(void)
{
    Ifx_GTM_ATOM     *atom = &MODULE_GTM.ATOM[CLK_ATOM];
    Ifx_GTM_ATOM_CH  *ch   = IfxGtm_Atom_Ch_getChannelPointer(atom, CLK_ATOM_CH);
    Ifx_GTM_ATOM_AGC *agc  = &atom->AGC;

    IfxGtm_PinMap_setAtomTout(&CLK_TOUT_PIN,
                             IfxPort_OutputMode_pushPull,
                             IfxPort_PadDriver_cmosAutomotiveSpeed1);

    /* SOMP, ARU disabled. (CTRL bitfields differ by device; this is your proven style) */
    ch->CTRL.U = (2u << 0);      /* SOMP */
    ch->CTRL.B.ARU_EN = 0u;      /* be explicit */

    /* initial PWM: 50% duty clock */
    ch->SR0.U = CLK_PERIOD_TICKS;
    ch->SR1.U = 0u;

    /* point this channel at the ARU address produced by MCS0 WRADDR[0] */
    ch->RDADDR.U = MCS0_WRADDR0;

    IfxGtm_Atom_Agc_enableChannelUpdate(agc, CLK_ATOM_CH, TRUE);
    IfxGtm_Atom_Agc_enableChannel(agc, CLK_ATOM_CH, TRUE, FALSE);
    IfxGtm_Atom_Agc_enableChannelOutput(agc, CLK_ATOM_CH, TRUE, FALSE);
    IfxGtm_Atom_Agc_trigger(agc);

    ch->CTRL.B.ARU_EN = 1u;
}

/* DATA: SOMP PWM fed from ARU (MCS0_WRADDR0) */
static void atomDataInit(void)
{
    Ifx_GTM_ATOM     *atom = &MODULE_GTM.ATOM[DATA_ATOM];
    Ifx_GTM_ATOM_CH  *ch   = IfxGtm_Atom_Ch_getChannelPointer(atom, DATA_ATOM_CH);
    Ifx_GTM_ATOM_AGC *agc  = &atom->AGC;

    IfxGtm_PinMap_setAtomTout(&DATA_TOUT_PIN,
                             IfxPort_OutputMode_pushPull,
                             IfxPort_PadDriver_cmosAutomotiveSpeed1);

    /* SOMP, keep ARU disabled initially */
    ch->CTRL.U = (2u << 0);
    ch->CTRL.B.ARU_EN = 0u;

    /* seed with safe initial values so it runs even before ARU data arrives */
    ch->SR0.U = CLK_PERIOD_TICKS;
    ch->SR1.U = 0u;

    /* point this channel at the ARU address produced by MCS0 WRADDR[0] */
    ch->RDADDR.U = MCS0_WRADDR1;

    IfxGtm_Atom_Agc_enableChannelUpdate(agc, DATA_ATOM_CH, TRUE);
    IfxGtm_Atom_Agc_enableChannel(agc, DATA_ATOM_CH, TRUE, FALSE);
    IfxGtm_Atom_Agc_enableChannelOutput(agc, DATA_ATOM_CH, TRUE, FALSE);

    /* force first update (loads SR0/SR1 into CM0/CM1) */
    IfxGtm_Atom_Agc_trigger(agc);

    /* now enable ARU so next cycle boundary will fetch from ARU */
    ch->CTRL.B.ARU_EN = 1u;
}

void initAtom(void)
{
    gtmClockInit();
    atomClockInit();
    atomDataInit();
}
