/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */

/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of MediaTek Inc. (C) 2008
*
*  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
*  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
*  RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
*  AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
*  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
*  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
*  NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
*  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
*  SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
*  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
*  NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S
*  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
*
*  BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE
*  LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
*  AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
*  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
*  MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
*
*  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
*  WITH THE LAWS OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF
*  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
*  RELATED THERETO SHALL BE SETTLED BY ARBITRATION IN SAN FRANCISCO, CA, UNDER
*  THE RULES OF THE INTERNATIONAL CHAMBER OF COMMERCE (ICC).
*
*****************************************************************************/


#ifndef BUILD_LK
#include <linux/string.h>
#include <linux/kernel.h>
#endif
#include "lcm_drv.h"

#ifdef BUILD_LK
	#include <platform/mt_gpio.h>
	#include <string.h>
#elif defined(BUILD_UBOOT)
	#include <asm/arch/mt_gpio.h>
#else
	#include <mach/mt_gpio.h>
#endif
// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  (480)
#define FRAME_HEIGHT (800)
//#define LCM_DSI_CMD_MODE

#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif

//static unsigned int lcm_esd_test = FALSE;      ///only for ESD test

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))

#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))

#define REGFLAG_DELAY             							0XFE
#define REGFLAG_END_OF_TABLE      							0xFD   // END OF REGISTERS MARKER

#define LCM_ID       (0x9806)

// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)									lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)				lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg											lcm_util.dsi_read_reg()
#define read_reg_v2(cmd, buffer, buffer_size)   				lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)     

// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

struct LCM_setting_table {
    unsigned char cmd;
    unsigned char count;
    unsigned char para_list[64];
};

#define LCD_ID_ADC_CHANNEL    1
#define LCD_ID_READ_TIMES     3
#define LCD_ID_VALUE_OFFSET    300

#define ADC_ILI9806E_TCL_VALUE    	0
#define  ADC_ILI9806E_YASSY_VALUE   2487

typedef enum
{
    LCM_TYPE_TCL,
    LCM_TYPE_YASSY,

    LCM_TYPE_MAX,
} LCM_TYPE_E;

static int g_Cust_Max_Level = 239;
static int g_Cust_Min_Level = 12;

static int lcm_brightness_mapping(int level)
{
  int mapped_level;

  mapped_level = ((g_Cust_Max_Level - g_Cust_Min_Level)*(int)level + \
  (255*g_Cust_Min_Level - 30*g_Cust_Max_Level)*4)/(255 - 30);

   #ifdef BUILD_LK
   printf("lcm_brightness_mapping= %d\n", mapped_level);
   #else
   printk("lcm_brightness_mapping= %d\n", mapped_level);
   #endif
  if (mapped_level > 1023) mapped_level = 1023;
  if (mapped_level < 0) mapped_level = 0;

  return mapped_level;
}

static unsigned int lcm_type = 0xFFFF;
extern int IMM_GetOneChannelValue(int dwChannel, int data[4], int* rawdata);
static void lcm_adc_id(void)
{
    int lcmid_adc = 0, ret_temp = 0, i = 0;
    int data[4] = {0,0,0,0};
    int res =0;
    
    i = LCD_ID_READ_TIMES;
    
    while (i--)
    {
        res = IMM_GetOneChannelValue(LCD_ID_ADC_CHANNEL,data,&ret_temp);
        lcmid_adc += ret_temp;
#ifdef BUILD_LK
        printf("YYYYY 111[%d] = temp:%d,val:%d\n", i, ret_temp, lcmid_adc);
#else
        printk("YYYYY 222[%d] = temp:%d,val:%d\n", i, ret_temp, lcmid_adc);
#endif
    }
    
    lcmid_adc = lcmid_adc/LCD_ID_READ_TIMES;

    if((lcmid_adc > ((int)(ADC_ILI9806E_TCL_VALUE - LCD_ID_VALUE_OFFSET))) && (lcmid_adc < (ADC_ILI9806E_TCL_VALUE + LCD_ID_VALUE_OFFSET)))
    {
        lcm_type = LCM_TYPE_TCL;
    }
    else if((lcmid_adc > ((int)(ADC_ILI9806E_YASSY_VALUE - LCD_ID_VALUE_OFFSET))) && (lcmid_adc < (ADC_ILI9806E_YASSY_VALUE + LCD_ID_VALUE_OFFSET)))
    {
        lcm_type = LCM_TYPE_YASSY;
    }
    else
    {
        lcm_type = LCM_TYPE_MAX;
    }
    
#ifdef BUILD_LK
    printf("YYYYY 333 = lcm_type:%d\n", lcm_type);
#else
    printk("YYYYY 444 = lcm_type:%d\n", lcm_type);
#endif

    return ;
}

static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;

    for(i = 0; i < count; i++)
    {
        unsigned cmd;
        cmd = table[i].cmd;
		
        switch (cmd)
        {			
            case REGFLAG_DELAY :
                MDELAY(table[i].count);
                break;

            case REGFLAG_END_OF_TABLE :
                break;

            default:
				dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
       	}
    }
}

static struct LCM_setting_table lcm_sleep_out_setting[] = {
    // Sleep Out
   {0xFF,5,{0xFF,0x98,0x06,0x04,0x00}},
   {REGFLAG_DELAY, 1, {}},
	{0x11, 0, {}},
    {REGFLAG_DELAY, 120, {}},

    // Display ON
	{0x29, 1, {0x00}},
	{REGFLAG_DELAY, 10, {}},

	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_sleep_in_setting[] = {
#if 0
	// Display off sequence
	{0x28, 1, {0x00}},
	{REGFLAG_DELAY, 10, {}},

    // Sleep Mode On
	{0x10, 1, {0x00}},
    {REGFLAG_DELAY, 120, {}},
#endif
    
    {0xFF,5,{0xFF,0x98,0x06,0x04,0x00}},     // Change to Page

// Display off sequence
{REGFLAG_DELAY, 1, {}},
{0x28, 0, {0x00}},

{REGFLAG_DELAY, 10, {}},
{0x10, 0, {0x00}},
{REGFLAG_DELAY, 120, {}},



//{REGFLAG_END_OF_TABLE, 0x00, {}}
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_initialization_setting_tcl[] = {
			{0xFF,5,{0xFF,0x98,0x06,0x04,0x01}},// EXTC Command Set enable register
			{0x08,1,{0x10}},// SPI Interface Setting
			{0x21,1,{0x01}},// Interface Mode Control
			{0x30,1,{0x02}},
			{0x31,1,{0x00}}, //02			
			
			{0x40,1,{0x16}},   //11-->16 20150104
			{0x41,1,{0x66}},  
			{0x42,1,{0x03}},   
			{0x43,1,{0x09}},  
			{0x44,1,{0x09}},
			
			{0x50,1,{0x88}},
			{0x51,1,{0x88}},
			{0x52,1,{0x00}},
			{0x53,1,{0x45}},//4b
		//	{0x56,1,{0x01}},
			{0x57,1,{0x50}},
			///{0x56,1,{0x11}},	///USE OTP VALUE 20150104
				
			{0x60,1,{0x14}},   //07--->14 20150104
			{0x61,1,{0x00}},
			{0x62,1,{0x0A}},
			{0x63,1,{0x00}},
			
			{0xA0,1,{0x00}},	
			{0xA1,1,{0x05}},//14
			{0xA2,1,{0x10}},//1f
			{0xA3,1,{0x11}},//0e
			{0xA4,1,{0x0a}},//08
			{0xA5,1,{0x1d}},//17
			{0xA6,1,{0x0a}},//08
			{0xA7,1,{0x0a}},//07
			{0xA8,1,{0x02}},//05
			{0xA9,1,{0x06}},//09
			{0xAA,1,{0x04}},//04
			{0xAB,1,{0x04}},
			{0xAC,1,{0x07}},	
			{0xAD,1,{0x34}},
			{0xAE,1,{0x2f}},
			{0xAF,1,{0x00}},
			
			{0xC0,1,{0x00}},
			{0xC1,1,{0x05}},//0e
			{0xC2,1,{0x0c}},//15
			{0xC3,1,{0x11}},//10
			{0xC4,1,{0x0a}},//09
			{0xC5,1,{0x16}},//15
			{0xC6,1,{0x0a}},//07
			{0xC7,1,{0x09}},//07
			{0xC8,1,{0x04}},//04
			{0xC9,1,{0x0a}},//09
			{0xCA,1,{0x06}},//06
			{0xCB,1,{0x05}},
			{0xCC,1,{0x0e}},
			{0xCD,1,{0x29}},
			{0xCE,1,{0x26}},
			{0xCF,1,{0x00}},
/*
	{0x57,1,{0x53}},//ql
*/
	
			
			{0xFF,5,{0xFF,0x98,0x06,0x04,0x06}},
			
			{0x00,1,{0x21}},
			{0x01,1,{0x06}},
			{0x02,1,{0x00}},
			{0x03,1,{0x00}},
			{0x04,1,{0x01}},
			{0x05,1,{0x05}},
			{0x06,1,{0x98}},
			{0x07,1,{0x02}},
			{0x08,1,{0x05}},
			{0x09,1,{0x00}},
			{0x0A,1,{0x00}},
			{0x0B,1,{0x00}},
			{0x0C,1,{0x01}},
			{0x0D,1,{0x01}},
			{0x0E,1,{0x00}},
			{0x0F,1,{0x00}},
			{0x10,1,{0x77}},
			{0x11,1,{0xF4}},
			{0x12,1,{0x00}},
			{0x13,1,{0x00}},
			{0x14,1,{0x00}},
			{0x15,1,{0xC0}},
			{0x16,1,{0x08}},
			{0x17,1,{0x00}},
			{0x18,1,{0x00}},
			{0x19,1,{0x00}},
			{0x1A,1,{0x00}},
			{0x1B,1,{0x00}},
			{0x1C,1,{0x00}},
			{0x1D,1,{0x00}},
			
			{0x20,1,{0x01}},
			{0x21,1,{0x23}},
			{0x22,1,{0x45}},
			{0x23,1,{0x67}},
			{0x24,1,{0x01}},
			{0x25,1,{0x23}},
			{0x26,1,{0x45}},
			{0x27,1,{0x67}},
			
			{0x30,1,{0x01}},    //03--->01 20150104
			{0x31,1,{0x22}},
			{0x32,1,{0x11}},
			{0x33,1,{0x00}},
			{0x34,1,{0x86}},
			{0x35,1,{0x68}},
			{0x36,1,{0x22}},
			{0x37,1,{0x22}},
			{0x38,1,{0xDA}},
			{0x39,1,{0xCB}},
			{0x3A,1,{0xBC}},
			{0x3B,1,{0xAD}},
			{0x3C,1,{0x22}},
			{0x3D,1,{0x22}},
			{0x3E,1,{0x22}},
			{0x3F,1,{0x22}},
			{0x40,1,{0x22}},
			{0x52,1,{0x10}},
			{0x53,1,{0x10}},				
			{0xFF,5,{0xFF,0x98,0x06,0x04,0x07}},
			{0x17,1,{0x22}},
			{0x02,1,{0x77}},
			{0xE1,1,{0x79}},
		//	{0x00,1,{0x11}},
						
			//{0x81,1,{0x22}},//QL
			//{0x80,1,{0x40}},//QL			
			{0xFF,5,{0xFF,0x98,0x06,0x04,0x00}},
			
		//	{0x36,1,{0x08}},
			{0x35,1,{0x00}},//QL
			{0x11,1,{0x00}},
			{REGFLAG_DELAY, 120, {}}, 
			{0x29,1,{0x00}},

		//REGFLAG_DELAY, 1, {}}, //QL
	//{0xFF,5,{0xFF,0x98,0x06,0x04,0x07}},	//QL	
//	{0x34,1,{0x01}},	//QL	
//	{0xFF,5,{0xFF,0x98,0x06,0x04,0x00}},//QL

			{REGFLAG_DELAY, 10, {}}, 
			//{REGFLAG_DELAY, 120, {}}, 
			//{0x2C,1,{0x00}},//GRAM start writing
			
			{REGFLAG_END_OF_TABLE, 0x00, {}}

};


static struct LCM_setting_table lcm_initialization_setting_yassy[] = {

			{0xFF,5,{0xFF,0x98,0x06,0x04,0x01}},// EXTC Command Set enable register
			{0x08,1,{0x10}},// SPI Interface Setting
			{0x21,1,{0x01}},// Interface Mode Control
			{0x30,1,{0x02}},
			{0x31,1,{0x02}}, //02
			
			{0x40,1,{0x11}},   
			{0x41,1,{0x66}},  
			{0x42,1,{0x03}},   
			{0x43,1,{0x09}},  
			{0x44,1,{0x09}},
			
			{0x50,1,{0x88}},
			{0x51,1,{0x88}},
			{0x52,1,{0x00}},
			{0x53,1,{0x4A}},//4b
			{0x57,1,{0x50}},
			//{0x56,1,{0x11}},	
				
			{0x60,1,{0x07}},
			{0x61,1,{0x00}},
			{0x62,1,{0x0A}},
			{0x63,1,{0x00}},
			
			{0xA0,1,{0x00}},	
			{0xA1,1,{0x0C}},//14
			{0xA2,1,{0x17}},//1f
			{0xA3,1,{0x11}},//0e
			{0xA4,1,{0x09}},//08
			{0xA5,1,{0x16}},//17
			{0xA6,1,{0x09}},//08
			{0xA7,1,{0x08}},//07
			{0xA8,1,{0x04}},//05
			{0xA9,1,{0x09}},//09
			{0xAA,1,{0x07}},//04
			{0xAB,1,{0x04}},
			{0xAC,1,{0x0B}},	
			{0xAD,1,{0x2E}},
			{0xAE,1,{0x2A}},
			{0xAF,1,{0x00}},
			
			{0xC0,1,{0x00}},
			{0xC1,1,{0x0C}},//0e
			{0xC2,1,{0x17}},//15
			{0xC3,1,{0x11}},//10
			{0xC4,1,{0x09}},//09
			{0xC5,1,{0x16}},//15
			{0xC6,1,{0x09}},//07
			{0xC7,1,{0x08}},//07
			{0xC8,1,{0x03}},//04
			{0xC9,1,{0x08}},//09
			{0xCA,1,{0x06}},//06
			{0xCB,1,{0x04}},
			{0xCC,1,{0x0B}},
			{0xCD,1,{0x2E}},
			{0xCE,1,{0x2A}},
			{0xCF,1,{0x00}},
/*
	{0x57,1,{0x53}},//ql
*/
	
			
			{0xFF,5,{0xFF,0x98,0x06,0x04,0x06}},
			
			{0x00,1,{0x21}},
			{0x01,1,{0x06}},
			{0x02,1,{0x00}},
			{0x03,1,{0x00}},
			{0x04,1,{0x01}},
			{0x05,1,{0x05}},
			{0x06,1,{0x98}},
			{0x07,1,{0x02}},
			{0x08,1,{0x05}},
			{0x09,1,{0x00}},
			{0x0A,1,{0x00}},
			{0x0B,1,{0x00}},
			{0x0C,1,{0x01}},
			{0x0D,1,{0x01}},
			{0x0E,1,{0x00}},
			{0x0F,1,{0x00}},
			{0x10,1,{0x77}},
			{0x11,1,{0xF4}},
			{0x12,1,{0x00}},
			{0x13,1,{0x00}},
			{0x14,1,{0x00}},
			{0x15,1,{0xC0}},
			{0x16,1,{0x08}},
			{0x17,1,{0x00}},
			{0x18,1,{0x00}},
			{0x19,1,{0x00}},
			{0x1A,1,{0x00}},
			{0x1B,1,{0x00}},
			{0x1C,1,{0x00}},
			{0x1D,1,{0x00}},
			
			{0x20,1,{0x01}},
			{0x21,1,{0x23}},
			{0x22,1,{0x45}},
			{0x23,1,{0x67}},
			{0x24,1,{0x01}},
			{0x25,1,{0x23}},
			{0x26,1,{0x45}},
			{0x27,1,{0x67}},
			
			{0x30,1,{0x03}},
			{0x31,1,{0x22}},
			{0x32,1,{0x11}},
			{0x33,1,{0x00}},
			{0x34,1,{0x86}},
			{0x35,1,{0x68}},
			{0x36,1,{0x22}},
			{0x37,1,{0x22}},
			{0x38,1,{0xDA}},
			{0x39,1,{0xCB}},
			{0x3A,1,{0xBC}},
			{0x3B,1,{0xAD}},
			{0x3C,1,{0x22}},
			{0x3D,1,{0x22}},
			{0x3E,1,{0x22}},
			{0x3F,1,{0x22}},
			{0x40,1,{0x22}},
			{0x52,1,{0x10}},
			{0x53,1,{0x10}},				
			{0xFF,5,{0xFF,0x98,0x06,0x04,0x07}},
			{0x17,1,{0x22}},
			{0x02,1,{0x77}},
			{0xE1,1,{0x79}},
			{0x00,1,{0x11}},
						
			//{0x81,1,{0x22}},//QL
			//{0x80,1,{0x40}},//QL			
			{0xFF,5,{0xFF,0x98,0x06,0x04,0x00}},
			
		//	{0x36,1,{0x08}},
			{0x35,1,{0x00}},//QL
			{0x11,1,{0x00}},
			{REGFLAG_DELAY, 120, {}}, 
			{0x29,1,{0x00}},

		//REGFLAG_DELAY, 1, {}}, //QL
	//{0xFF,5,{0xFF,0x98,0x06,0x04,0x07}},	//QL	
//	{0x34,1,{0x01}},	//QL	
//	{0xFF,5,{0xFF,0x98,0x06,0x04,0x00}},//QL

			{REGFLAG_DELAY, 10, {}}, 
			//{REGFLAG_DELAY, 120, {}}, 
			//{0x2C,1,{0x00}},//GRAM start writing
			
			{REGFLAG_END_OF_TABLE, 0x00, {}}

};


static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}

static void lcm_get_params(LCM_PARAMS *params)
{
    memset(params, 0, sizeof(LCM_PARAMS));

    params->type   = LCM_TYPE_DSI;

    params->width  = FRAME_WIDTH;
    params->height = FRAME_HEIGHT;

    // enable tearing-free
    params->dbi.te_mode 				= LCM_DBI_TE_MODE_DISABLED;
    params->dbi.te_edge_polarity		= LCM_POLARITY_RISING;

#if defined(LCM_DSI_CMD_MODE)
    params->dsi.mode   = CMD_MODE;
#else
    params->dsi.mode   = SYNC_PULSE_VDO_MODE;
#endif

    // DSI
    /* Command mode setting */
    params->dsi.LANE_NUM				= LCM_TWO_LANE;
    //The following defined the fomat for data coming from LCD engine.
    params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
    params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
    params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
    params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

    // Highly depends on LCD driver capability.
    // Not support in MT6573
    params->dsi.packet_size=256;

    // Video mode setting		
    params->dsi.intermediat_buffer_num = 2;//這個地方0也可以....有機會搞清楚because DSI/DPI HW design change, this parameters should be 0 when video mode in MT658X; or memory leakage

    params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
    params->dsi.word_count=480*3;	


    params->dsi.vertical_sync_active				= 6;  //---3
    params->dsi.vertical_backporch					= 14; //---14
    params->dsi.vertical_frontporch 				= 20;  //----8
    params->dsi.vertical_active_line				= FRAME_HEIGHT; 

    params->dsi.horizontal_sync_active				= 10;  //----2
    params->dsi.horizontal_backporch				= 30; //----28 80
    params->dsi.horizontal_frontporch				= 30; //----50 80
    params->dsi.horizontal_active_pixel 			= FRAME_WIDTH;
    
    params->dsi.ssc_disable = 0;
	  params->dsi.ssc_range = 8;

    // params->dsi.HS_PRPR=3;
    // params->dsi.CLK_HS_POST=22;
    // params->dsi.DA_HS_EXIT=20;
   
    // Bit rate calculation
    // 1 Every lane speed
    //params->dsi.pll_div1=1; 	// div1=0,1,2,3;div1_real=1,2,4,4 ----0: 546Mbps  1:273Mbps
    //params->dsi.pll_div2=1; 	// div2=0,1,2,3;div1_real=1,2,4,4	
   // params->dsi.fbk_div =28;// fref=26MHz, fvco=fref*(fbk_div+1)*2/(div1_real*div2_real)	
    //此porch定設定確認道頻率為56HZ  //30
    params->physical_width=51;
    params->physical_height=86; 
    params->dsi.PLL_CLOCK = 162;
}

static void lcm_init_tcl(void)
{
    SET_RESET_PIN(1);
    MDELAY(1);
    SET_RESET_PIN(0);
    MDELAY(10);
    SET_RESET_PIN(1);
    MDELAY(120);
    push_table(lcm_initialization_setting_tcl, sizeof(lcm_initialization_setting_tcl) / sizeof(struct LCM_setting_table), 1);
   // push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}
static void lcm_init_yassy(void)
{
    SET_RESET_PIN(1);
    MDELAY(1);
    SET_RESET_PIN(0);
    MDELAY(10);
    SET_RESET_PIN(1);
    MDELAY(120);
    push_table(lcm_initialization_setting_yassy, sizeof(lcm_initialization_setting_yassy) / sizeof(struct LCM_setting_table), 1);
   // push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}


static void lcm_suspend(void)
{
    push_table(lcm_sleep_in_setting, sizeof(lcm_sleep_in_setting) / sizeof(struct LCM_setting_table), 1);
}


static void lcm_resume_tcl(void)
{
	  
//		lcm_adc_id();
//if(LCM_TYPE_TCL== lcm_type)
//{
	    lcm_init_tcl();
//}

    //lcm_init_tcl();
//	push_table(lcm_sleep_out_setting, sizeof(lcm_sleep_out_setting) / sizeof(struct LCM_setting_table), 1);
}
static void lcm_resume_yassy(void)
{
	//lcm_adc_id();
//if(LCM_TYPE_YASSY== lcm_type)
//{
	    lcm_init_yassy();
//}

//	push_table(lcm_sleep_out_setting, sizeof(lcm_sleep_out_setting) / sizeof(struct LCM_setting_table), 1);
}


static void lcm_update(unsigned int x, unsigned int y,
                       unsigned int width, unsigned int height)
{
    unsigned int x0 = x;
    unsigned int y0 = y;
    unsigned int x1 = x0 + width - 1;
    unsigned int y1 = y0 + height - 1;

    unsigned char x0_MSB = ((x0>>8)&0xFF);
    unsigned char x0_LSB = (x0&0xFF);
    unsigned char x1_MSB = ((x1>>8)&0xFF);
    unsigned char x1_LSB = (x1&0xFF);
    unsigned char y0_MSB = ((y0>>8)&0xFF);
    unsigned char y0_LSB = (y0&0xFF);
    unsigned char y1_MSB = ((y1>>8)&0xFF);
    unsigned char y1_LSB = (y1&0xFF);

    unsigned int data_array[16];

    data_array[0]= 0x00053902;
    data_array[1]= (x1_MSB<<24)|(x0_LSB<<16)|(x0_MSB<<8)|0x2a;
    data_array[2]= (x1_LSB);
    dsi_set_cmdq(data_array, 3, 1);

    data_array[0]= 0x00053902;
    data_array[1]= (y1_MSB<<24)|(y0_LSB<<16)|(y0_MSB<<8)|0x2b;
    data_array[2]= (y1_LSB);
    dsi_set_cmdq(data_array, 3, 1);

//	data_array[0]= 0x00290508; //HW bug, so need send one HS packet
//	dsi_set_cmdq(data_array, 1, 1);

    data_array[0]= 0x002c3909;
    dsi_set_cmdq(data_array, 1, 0);

}

static unsigned int lcm_compare_id_tcl(void)
{

    

   unsigned int id1 = 0, id2 = 0, id3 = 0, id4 = 0,id;
    unsigned char buffer[6];

    unsigned int data_array[16];
	lcm_adc_id();


    SET_RESET_PIN(1);	
    MDELAY(5);
    SET_RESET_PIN(0);
    MDELAY(10);
    SET_RESET_PIN(1);
    MDELAY(130);


    data_array[0]=0x00063902;
    data_array[1]=0x0698FFFF;
    data_array[2]=0x00000104;
    dsi_set_cmdq(data_array, 3, 1);
    MDELAY(10);

    // read id return five byte : 0x01 0x8B 0x96 0x08 0xFF
    data_array[0] = 0x00023700;
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(10);

    read_reg_v2(0x01, buffer, 1);
    id1= buffer[0]; //should be 
    read_reg_v2(0x02, buffer, 1);
    id2= buffer[0]; //should be 0x98

    id=(id1 << 8) | id2;
#if defined(BUILD_LK)||defined(BUILD_UBOOT)
    printf("zhengzhou %s id=%x\r\n",__func__,id);
#else
    printk("zhengzhou %s id=%x\r\n",__func__,id);
#endif
  

   if ((0x0604==id)?1:0)
   	{
          if(LCM_TYPE_TCL== lcm_type)
	            return 1;
	       else
		    return 0;
	}
     return 0;
}


static unsigned int lcm_compare_id_yassy(void)
{
       

    unsigned int id1 = 0, id2 = 0, id3 = 0, id4 = 0,id;
    unsigned char buffer[6];

    unsigned int data_array[16];
	lcm_adc_id();


    SET_RESET_PIN(1);	
    MDELAY(5);
    SET_RESET_PIN(0);
    MDELAY(10);
    SET_RESET_PIN(1);
    MDELAY(130);


    data_array[0]=0x00063902;
    data_array[1]=0x0698FFFF;
    data_array[2]=0x00000104;
    dsi_set_cmdq(data_array, 3, 1);
    MDELAY(10);

    // read id return five byte : 0x01 0x8B 0x96 0x08 0xFF
    data_array[0] = 0x00023700;
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(10);

    read_reg_v2(0x01, buffer, 1);
    id1= buffer[0]; //should be 
    read_reg_v2(0x02, buffer, 1);
    id2= buffer[0]; //should be 0x98

    id=(id1 << 8) | id2;
#if defined(BUILD_LK)||defined(BUILD_UBOOT)
    printf("ili9806E %s id=%x\r\n",__func__, id);
#else
    printk("ili9806E %s id=%x\r\n",__func__, id);
#endif
		

          if((0x0604==id)?1:0)
          	{
		 if(LCM_TYPE_YASSY== lcm_type)
                                return 1;
		   else
		        return 0;
          	}
       return 0;
}

static unsigned int lcm_esd_check(void)
{
#ifndef BUILD_LK     
    unsigned int id = 0;  
    unsigned char buffer[6];   
    unsigned int data_array[16];   
    static int lcm_id;        
    data_array[0] = 0x00013700;// read id return two byte,version and id  
    dsi_set_cmdq(data_array, 1, 1);  
    //MDELAY(10);
    read_reg_v2(0x0A, buffer, 1);    // A1  
    id = buffer[0]; //we only need ID     
    printk("ghj ########## 8018 lcd_id=%x,\r\n",id);     	

    if (id == 0x9c)     
    {    
        return 0;   
    }      
    else      
    {         
        return 1; //TRUE
    }
#else
    return 0;
#endif
}

static unsigned int lcm_esd_recover_tcl(void)
{
    lcm_init_tcl();

    return TRUE;
}
static unsigned int lcm_esd_recover_yassy(void)
{
    lcm_init_yassy();

    return TRUE;
}


static unsigned int lcm_check_status(void)
{
    unsigned char buffer[2];

    read_reg_v2(0x0A, buffer, 2);
#ifdef BUILD_LK
    printf("Check LCM Status: 0x%08x\n", buffer[0]);
#else
    printk("Check LCM Status: 0x%08x\n", buffer[0]);
#endif
    return 0;
}


// ---------------------------------------------------------------------------
//  Get LCM Driver Hooks
// ---------------------------------------------------------------------------
LCM_DRIVER ili9806e_dsi_vdo_lcm_drv_tcl = 
{
    .name			= "ili9806e_dsi_vdo_tcl",
    .set_util_funcs = lcm_set_util_funcs,
    .compare_id     = lcm_compare_id_tcl,
    .get_params     = lcm_get_params,
    .init           = lcm_init_tcl,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume_tcl,
#if defined(LCM_DSI_CMD_MODE)
    .update         = lcm_update,
#endif
#ifdef WT_BRIGHTNESS_MAPPING_WITH_LCM
    .cust_mapping = lcm_brightness_mapping,
#endif
    //.esd_check      = lcm_esd_check,
    //.esd_recover    = lcm_esd_recover,
};
LCM_DRIVER ili9806e_dsi_vdo_lcm_drv_yassy = 
{
    .name			= "ili9806e_dsi_vdo_yassy",
    .set_util_funcs = lcm_set_util_funcs,
    .compare_id     = lcm_compare_id_yassy,
    .get_params     = lcm_get_params,
    .init           = lcm_init_yassy,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume_yassy,
#if defined(LCM_DSI_CMD_MODE)
    .update         = lcm_update,
#endif
#ifdef WT_BRIGHTNESS_MAPPING_WITH_LCM
    .cust_mapping = lcm_brightness_mapping,
#endif
    //.esd_check      = lcm_esd_check,
    //.esd_recover    = lcm_esd_recover,
};

