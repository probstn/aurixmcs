#include <mcs.h>

#define CLK_PERIOD_TICKS   (100u)
#define DUTY_50            (CLK_PERIOD_TICKS/2u)
#define DUTY_LOW           (0u)
#define DUTY_HIGH          (CLK_PERIOD_TICKS)

#define MCS_MOVL(regA, imm24)   (0x10000000u | (((uint32)(regA) & 0xFu) << 24) | ((uint32)(imm24) & 0x00FFFFFFu))
#define MCS_AWRI(regA, regB)    (0xB0000000u | (((uint32)(regA) & 0xFu) << 24) | (((uint32)(regB) & 0xFu) << 20) | (0x5u << 16))
#define MCS_JMP(addr)           (0xE0000000u | ((uint32)(addr) & 0x0000FFFcu))

#define TX_BYTE   (0xA5u)  /* 1010 0101 */

#define BIT_DUTY(bitmask)  (((TX_BYTE & (bitmask)) != 0u) ? DUTY_HIGH : DUTY_LOW)

/*
  R2 = period
  R3 = clk duty (50%)
  R4 = data duty (0/100)
  R6 = write-index selector (R6[4:0]) for AWRI
*/
const uint32 MCS0_CH0_prog[] =
{
    /* init */
    /* 0x00 */ MCS_MOVL(2u, CLK_PERIOD_TICKS),
    /* 0x04 */ MCS_MOVL(3u, DUTY_50),

    /* ---- bit 7 ---- */
    /* 0x08 */ MCS_MOVL(4u, BIT_DUTY(0x80u)),
    /* write CLK to ARU write-index 0 */
    /* 0x0C */ MCS_MOVL(6u, 0u),
    /* 0x10 */ MCS_AWRI(2u, 3u),
    /* write DATA to ARU write-index 1 */
    /* 0x14 */ MCS_MOVL(6u, 1u),
    /* 0x18 */ MCS_AWRI(2u, 4u),

    /* ---- bit 6 ---- */
    /* 0x1C */ MCS_MOVL(4u, BIT_DUTY(0x40u)),
    /* 0x20 */ MCS_MOVL(6u, 0u),
    /* 0x24 */ MCS_AWRI(2u, 3u),
    /* 0x28 */ MCS_MOVL(6u, 1u),
    /* 0x2C */ MCS_AWRI(2u, 4u),

    /* ---- bit 5 ---- */
    /* 0x30 */ MCS_MOVL(4u, BIT_DUTY(0x20u)),
    /* 0x34 */ MCS_MOVL(6u, 0u),
    /* 0x38 */ MCS_AWRI(2u, 3u),
    /* 0x3C */ MCS_MOVL(6u, 1u),
    /* 0x40 */ MCS_AWRI(2u, 4u),

    /* ---- bit 4 ---- */
    /* 0x44 */ MCS_MOVL(4u, BIT_DUTY(0x10u)),
    /* 0x48 */ MCS_MOVL(6u, 0u),
    /* 0x4C */ MCS_AWRI(2u, 3u),
    /* 0x50 */ MCS_MOVL(6u, 1u),
    /* 0x54 */ MCS_AWRI(2u, 4u),

    /* ---- bit 3 ---- */
    /* 0x58 */ MCS_MOVL(4u, BIT_DUTY(0x08u)),
    /* 0x5C */ MCS_MOVL(6u, 0u),
    /* 0x60 */ MCS_AWRI(2u, 3u),
    /* 0x64 */ MCS_MOVL(6u, 1u),
    /* 0x68 */ MCS_AWRI(2u, 4u),

    /* ---- bit 2 ---- */
    /* 0x6C */ MCS_MOVL(4u, BIT_DUTY(0x04u)),
    /* 0x70 */ MCS_MOVL(6u, 0u),
    /* 0x74 */ MCS_AWRI(2u, 3u),
    /* 0x78 */ MCS_MOVL(6u, 1u),
    /* 0x7C */ MCS_AWRI(2u, 4u),

    /* ---- bit 1 ---- */
    /* 0x80 */ MCS_MOVL(4u, BIT_DUTY(0x02u)),
    /* 0x84 */ MCS_MOVL(6u, 0u),
    /* 0x88 */ MCS_AWRI(2u, 3u),
    /* 0x8C */ MCS_MOVL(6u, 1u),
    /* 0x90 */ MCS_AWRI(2u, 4u),

    /* ---- bit 0 ---- */
    /* 0x94 */ MCS_MOVL(4u, BIT_DUTY(0x01u)),
    /* 0x98 */ MCS_MOVL(6u, 0u),
    /* 0x9C */ MCS_AWRI(2u, 3u),
    /* 0xA0 */ MCS_MOVL(6u, 1u),
    /* 0xA4 */ MCS_AWRI(2u, 4u),

    /* idle low and loop */
    /* 0xA8 */ MCS_MOVL(3u, DUTY_LOW),
    /* 0xAC */ MCS_MOVL(4u, DUTY_LOW),
    /* 0xB0 */ MCS_MOVL(6u, 0u),
    /* 0xB4 */ MCS_AWRI(2u, 3u),
    /* 0xB8 */ MCS_MOVL(6u, 1u),
    /* 0xBC */ MCS_AWRI(2u, 4u),
    /* 0xC0 */ MCS_JMP(0xB0u)
};


/* This function uses th CPU to copy the MCSx code image in its dedicated MCS-RAM portion. The MCS code image is stored
 * into the Aurix Flash memory.
 * */
static void copy_Mcs_Image(uint32 mcsCore, uint32 *image, uint32 imageSize)
{
    uint32 i;
    /* Calculate base address of the specific MCS core RAM */
    uint32 *mcsRam = (uint32 *)((uint32)GTM_MCS0_MEM + ((uint32)GTM_MCS1_MEM - (uint32)GTM_MCS0_MEM) * mcsCore);

    /* Calculate the total size of MCS RAM in words (approx 0x1000 bytes for typical Aurix, but dynamic calc is safer) */
    /* Note: Using the distance between MCS0 and MCS1 as a proxy for RAM size is common practice */
    uint32 ramSizeWords = ((uint32)GTM_MCS1_MEM - (uint32)GTM_MCS0_MEM) / 4;

    /* 1. Initialize ENTIRE MCS RAM to 0 to fix ECC checksums */
    for (i = 0; i < ramSizeWords; i++)
    {
        mcsRam[i] = 0x00000000;
    }

    /* 2. Write program code into MCS RAM (overwriting the zeros at the start) */
    for (i = 0; i < (imageSize / 4); i++)
    {
        mcsRam[i] = image[i];
    }

    /* 3. Data Sync Barrier: Ensure CPU writes complete before MCS tries to read */
    __dsync();
}

/* This function configures and starts GTM-MCS0_CH0-7
 * - calls the function "copy_Mcs_Image" to copy MCSx program image into its dedicate GTM-RAM portion
 * - enables XOREG registers
 * - starts MCSx execution
 * */
void start_Mcs0(void)
{
    /* Disable all channels first */
    GTM_MCS0_CH0_CTRL.B.EN = 0u;
    GTM_MCS0_CH1_CTRL.B.EN = 0u;
    GTM_MCS0_CH2_CTRL.B.EN = 0u;
    GTM_MCS0_CH3_CTRL.B.EN = 0u;
    GTM_MCS0_CH4_CTRL.B.EN = 0u;
    GTM_MCS0_CH5_CTRL.B.EN = 0u;
    GTM_MCS0_CH6_CTRL.B.EN = 0u;
    GTM_MCS0_CH7_CTRL.B.EN = 0u;

    /* Enable XOREG register set */
    GTM_MCS0_CTRL_STAT.B.EN_XOREG = 1u;

    /* Clear any previous errors */
    GTM_MCS0_ERR.U = 0xFFFFFFFFu;

    /* Load program (and clear ECC) */
    copy_Mcs_Image(0, (uint32 *)MCS0_CH0_prog, sizeof(MCS0_CH0_prog));

    /* Select MCS0_WRADDR[0] via R6[4:0] */
    //GTM_MCS0_CH0_R6.U  = (GTM_MCS0_CH0_R6.U & ~0x1Fu) | 0u;

    /* ACB = 0 (first demo) */
    GTM_MCS0_CH0_ACB.U = 0u;

    /* Enable only CH0 last */
    GTM_MCS0_CH0_CTRL.B.EN = 1u;
}

