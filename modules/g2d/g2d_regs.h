/* g2d/g2d_regs.h
 *
 * Copyright (c)	2008 Samsung Electronics
 *			2010 Tomasz Figa
 *
 * Samsung S3C G2D driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA02111-1307USA
 */

#ifndef __ASM_ARM_REGS_G2D_H
#define __ASM_ARM_REGS_G2D_H

/*
	Graphics 2D General Registers
*/
#define G2D_CONTROL_REG		(0x00)		/* Control register			*/
#define G2D_INTEN_REG		(0x04)		/* Interrupt enable register		*/
#define G2D_FIFO_INTC_REG	(0x08)		/* Interrupt control register		*/
#define G2D_INTC_PEND_REG	(0x0c)		/* Interrupt control pending register	*/
#define G2D_FIFO_STAT_REG	(0x10)		/* Command FIFO status register		*/

/*
	Graphics 2D Command Registers
*/
#define G2D_CMD0_REG		(0x100)		/* Command register for Line/Point drawing			*/
#define G2D_CMD1_REG		(0x104)		/* Command register for BitBLT					*/
#define G2D_CMD2_REG		(0x108)		/* Command register for Host to Screen Bitblt transfer start 	*/
#define G2D_CMD3_REG		(0x10c)		/* Command register for Host to Screen Bitblt transfer continue	*/
#define G2D_CMD4_REG		(0x110)		/* Command register for Color expansion (Font start)		*/
#define G2D_CMD5_REG		(0x114)		/* Command register for Color expansion (Font continue)		*/
#define G2D_CMD6_REG		(0x118)		/* Reserved							*/
#define G2D_CMD7_REG		(0x11c)		/* Command register for Color expansion (memory to screen)	*/

/*
	Graphics 2D Parameter Setting Registers
*/

/* Resolution */
#define G2D_SRC_RES_REG		(0x200)		/* Source resolution			*/
#define G2D_SRC_HORI_REG	(0x204)		/* Source horizontal resolution		*/
#define G2D_SRC_VERT_REG	(0x208)		/* Source vertical resolution		*/
#define G2D_DST_RES_REG		(0x210)		/* Destination resolution		*/
#define G2D_DST_HORI_REG	(0x214)		/* Destination horizontal resolutuon	*/
#define G2D_DST_VERT_REG	(0x218)		/* Destination vertical resolution	*/

/* Clipping window */
#define G2D_CW_LT_REG		(0x220)		/* LeftTop coordinates of Clip Window	*/
#define G2D_CW_LT_X_REG		(0x224)		/* Left X coordinate of Clip Window	*/
#define G2D_CW_LT_Y_REG		(0x228)		/* Top Y coordinate of Clip Window	*/
#define G2D_CW_RB_REG		(0x230)		/* RightBottom coordinate of Clip Window*/
#define G2D_CW_RB_X_REG		(0x234)		/* Right X coordinate of Clip Window	*/
#define G2D_CW_RB_Y_REG		(0x238)		/* Bottom Y coordinate of Clip Window	*/

/* Coordinates */
#define G2D_COORD0_REG		(0x300)
#define G2D_COORD0_X_REG	(0x304)
#define G2D_COORD0_Y_REG	(0x308)
#define G2D_COORD1_REG		(0x310)
#define G2D_COORD1_X_REG	(0x314)
#define G2D_COORD1_Y_REG	(0x318)
#define G2D_COORD2_REG		(0x320)
#define G2D_COORD2_X_REG	(0x324)
#define G2D_COORD2_Y_REG	(0x328)
#define G2D_COORD3_REG		(0x330)
#define G2D_COORD3_X_REG	(0x334)
#define G2D_COORD3_Y_REG	(0x338)

/* Rotation */
#define G2D_ROT_OC_REG		(0x340)		/* Rotation Origin Coordinates			*/
#define G2D_ROT_OC_X_REG	(0x344)		/* X coordinate of Rotation Origin Coordinates	*/
#define G2D_ROT_OC_Y_REG	(0x348)		/* Y coordinate of Rotation Origin Coordinates	*/
#define G2D_ROTATE_REG		(0x34c)		/* Rotation Mode register			*/
#define G2D_END_RDSIZE_REG	(0x350)		/* Endianess and read size register		*/

/* X,Y Increment */
#define G2D_X_INCR_REG		(0x400)
#define G2D_Y_INCR_REG		(0x404)
#define G2D_ROP_REG		(0x410)
#define G2D_ALPHA_REG		(0x420)

/* Color */
#define G2D_FG_COLOR_REG	(0x500)		/* Foreground Color /Alpha register	*/
#define G2D_BG_COLOR_REG	(0x504)		/* Background Color register		*/
#define G2D_BS_COLOR_REG	(0x508)		/* Blue Screen Color register		*/
#define G2D_SRC_FORMAT_REG	(0x510)		/* Src Image Color Mode register	*/
#define G2D_DST_FORMAT_REG	(0x514)		/* Dest Image Color Mode register	*/

/* Pattern */
#define G2D_PATTERN_REG		(0x600)
#define G2D_PATOFF_REG		(0x700)
#define G2D_PATOFF_X_REG	(0x704)
#define G2D_PATOFF_Y_REG	(0x708)
#define G2D_STENCIL_CNTL_REG	(0x720)
#define G2D_STENCIL_DR_MIN_REG	(0x724)
#define G2D_STENCIL_DR_MAX_REG	(0x728)

/*Image memory locations */
#define G2D_SRC_BASE_ADDR	(0x730)		/* Source image base address register	*/
#define G2D_DST_BASE_ADDR	(0x734)		/* Dest image base address register	*/

/*
	Register bits definitions
*/

/* interrupt mode select */
#define G2D_INTC_PEND_REG_CLRSEL_LEVEL		(1<<31)
#define G2D_INTC_PEND_REG_CLRSEL_PULSE		(0<<31)

#define G2D_INTEN_REG_FIFO_INT_E		(1<<0)
#define G2D_INTEN_REG_ACF			(1<<9)
#define G2D_INTEN_REG_CCF			(1<<10)

#define G2D_PEND_REG_INTP_ALL_FIN		(1<<9)
#define G2D_PEND_REG_INTP_CMD_FIN		(1<<10)

/* Line/Point drawing */
#define G2D_CMD0_REG_D_LAST			(0<<9)
#define G2D_CMD0_REG_D_NO_LAST			(1<<9)
#define G2D_CMD0_REG_M_Y			(0<<8)
#define G2D_CMD0_REG_M_X			(1<<8)
#define G2D_CMD0_REG_L				(1<<1)
#define G2D_CMD0_REG_P				(1<<0)

/* BitBLT */
#define G2D_CMD1_REG_S				(1<<1)
#define G2D_CMD1_REG_N				(1<<0)

/* resource color mode */
#define G2D_COLOR_MODE_REG_C3_32BPP		(1<<3)
#define G2D_COLOR_MODE_REG_C3_24BPP		(1<<3)
#define G2D_COLOR_MODE_REG_C2_18BPP		(1<<2)
#define G2D_COLOR_MODE_REG_C1_16BPP		(1<<1)
#define G2D_COLOR_MODE_REG_C0_15BPP		(1<<0)

#define G2D_COLOR_RGB_565			(0<<0)
#define G2D_COLOR_RGBA_5551			(1<<0)
#define G2D_COLOR_ARGB_1555			(2<<0)
#define G2D_COLOR_RGBA_8888			(3<<0)
#define G2D_COLOR_ARGB_8888			(4<<0)
#define G2D_COLOR_XRGB_8888			(5<<0)
#define G2D_COLOR_RGBX_8888			(6<<0)

/* rotation mode */
#define G2D_ROTATE_REG_FY			(1<<5)
#define G2D_ROTATE_REG_FX			(1<<4)
#define G2D_ROTATE_REG_R3_270			(1<<3)
#define G2D_ROTATE_REG_R2_180			(1<<2)
#define G2D_ROTATE_REG_R1_90			(1<<1)
#define G2D_ROTATE_REG_R0_0			(1<<0)

/* Endianess and read size */
#define G2D_ENDIAN_READSIZE_BIG_ENDIAN		(1<<4)
#define G2D_ENDIAN_READSIZE_SIZE_HW		(1<<2)
#define G2D_ENDIAN_READSIZE_READ_SIZE_1		(0<<0)
#define G2D_ENDIAN_READSIZE_READ_SIZE_4		(1<<0)
#define G2D_ENDIAN_READSIZE_READ_SIZE_8		(2<<0)
#define G2D_ENDIAN_READSIZE_READ_SIZE_16	(3<<0)

/* ROP Third Operand */
#define G2D_ROP_REG_OS_PATTERN			(0<<13)
#define G2D_ROP_REG_OS_FG_COLOR			(1<<13)

/* Alpha Blending Mode */
#define G2D_ROP_REG_ABM_NO_BLENDING		(0<<10)
#define G2D_ROP_REG_ABM_SRC_BITMAP		(1<<10)
#define G2D_ROP_REG_ABM_REGISTER		(2<<10)
#define G2D_ROP_REG_ABM_FADING 			(4<<10)

/* Raster operation mode */
#define G2D_ROP_REG_T_OPAQUE_MODE		(0<<9)
#define G2D_ROP_REG_T_TRANSP_MODE		(1<<9)
#define G2D_ROP_REG_B_BS_MODE_OFF		(0<<8)
#define G2D_ROP_REG_B_BS_MODE_ON		(1<<8)

/* stencil control */
#define G2D_STENCIL_CNTL_REG_STENCIL_ON_ON	(1<<31)
#define G2D_STENCIL_CNTL_REG_STENCIL_ON_OFF	(0<<31)
#define G2D_STENCIL_CNTL_REG_STENCIL_INVERSE	(1<<23)
#define G2D_STENCIL_CNTL_REG_STENCIL_SWAP	(1<<0)

#endif /* __ASM_ARM_REGS_G2D_H */
