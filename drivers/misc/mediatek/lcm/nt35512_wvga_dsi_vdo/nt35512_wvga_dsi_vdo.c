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

#define LCM_NT35512_ID  	(0x5512)

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

   {REGFLAG_DELAY, 1, {}},
	{0x11, 0, {}},
    {REGFLAG_DELAY, 150, {}},

    // Display ON
	{0x29, 1, {0x00}},
	{REGFLAG_DELAY, 50, {}},

	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_sleep_in_setting[] = {

{REGFLAG_DELAY, 1, {}},
{0x28, 0, {0x00}},

{REGFLAG_DELAY, 50, {}},
{0x10, 0, {0x00}},
{REGFLAG_DELAY, 150, {}},



//{REGFLAG_END_OF_TABLE, 0x00, {}}
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_initialization_setting[] = {
{0xFF,4,{0xAA,0x55,0xA5,0x80}},
	{0x6f,1,{0x0e}},
  {0xf4,1,{0x0a}},
  {0xFF,4,{0xAA,0x55,0xA5,0x00}},	
	{0xF0,5,{0x55,0xAA,0x52,0x08,0x02}},
  {0xb7,1,{0x01}},
  {0xF0,5,{0x55,0xAA,0x52,0x08,0x01}},
  {0xb0,1,{0x0d}},
  {0xb6,1,{0x24}},//36
  {0xb1,1,{0x0d}},
  {0xb7,1,{0x45}},//24//34
  {0xb2,1,{0x00}},
	{0xb8,1,{0x24}},
	{0xbf,1,{0x01}},
	{0xb3,1,{0x08}},//0f
	{0xb9,1,{0x34}},
	{0xb5,1,{0x08}},
	{0xc2,1,{0x03}},
	{0xba,1,{0x14}},
	{0xBC,3,{0x00,0x78,0x00}},	
	{0xBD,3,{0x00,0x78,0x00}},
	//{0xBE,2,{0x00,0x90}},	//5f
	{0xD1,52,{0x00,0x06,0x00,0x07,0x00,0x26,0x00,0x4C,0x00,0x70,0x00,0xA9,0x00,0xD2,0x01,0x0D,0x01,0x37,0x01,0x75,0x01,0xA0,0x01,0xE0,0x02,0x11,0x02,0x13,0x02,0x3D,0x02,0x6B,0x02,0x84,0x02,0x9F,0x02,0xAF,0x02,0xC5,0x02,0xD3,0x02,0xE7,0x02,0xF6,0x03,0x0E,0x03,0x48,0x03,0xFE}},
	{0xF0,5,{0x55,0xAA,0x52,0x08,0x00}},
	{0xb5,1,{0x50}},
	{0xB1,2,{0xfc,0x00}},
	{0xb6,1,{0x05}},
	{0xB7,2,{0x70,0x70}},
	{0xb8,4,{0x01,0x03,0x03,0x03}},
	{0xbc,1,{0x02}},
	{0xc9,5,{0xC0,0x02,0x50,0x50,0x50}},
	{0x11,1,{0x00}},
	{REGFLAG_DELAY,130,{}},
	{0x29,1,{0x00}},
	{REGFLAG_DELAY,10,{}},
	// Setting ending by predefined flag
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

	params->physical_width  = 62.06;
	params->physical_height = 110.42;
	
    // enable tearing-free
    params->dbi.te_mode 			= LCM_DBI_TE_MODE_DISABLED;
    //params->dbi.te_edge_polarity		= LCM_POLARITY_RISING;

#if (0)///(LCM_DSI_CMD_MODE)
    params->dsi.mode   = CMD_MODE;
#else
    params->dsi.mode   = SYNC_PULSE_VDO_MODE; 
#endif

    // DSI
    /* Command mode setting */
    // 1 Three lane or Four lane
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
    params->dsi.intermediat_buffer_num = 0;//because DSI/DPI HW design change, this parameters should be 0 when video mode in MT658X; or memory leakage

    params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
    params->dsi.word_count=480*3;	

    params->dsi.vertical_sync_active				= 6; // 3    2
    params->dsi.vertical_backporch					= 5; // 20   1 6
    params->dsi.vertical_frontporch					= 10; // 1  12
    params->dsi.vertical_active_line				= FRAME_HEIGHT; 

    params->dsi.horizontal_sync_active				= 20;// 50  2  40
    params->dsi.horizontal_backporch				= 30; //40  72
    params->dsi.horizontal_frontporch				= 30; //40  72
    params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

    //params->dsi.HS_TRAIL = 10;
    //params->dsi.HS_ZERO = 7;
    params->dsi.HS_PRPR = 4;
    //params->dsi.LPX = 3;
    //params->dsi.TA_SACK = 1;
    //params->dsi.TA_GET = 15;
    //params->dsi.TA_SURE = 3;
    //params->dsi.TA_GO = 12;
    //params->dsi.CLK_TRAIL = 10;
    //params->dsi.CLK_ZERO = 16;
    //params->dsi.LPX_WAIT = 10;
    //params->dsi.CONT_DET = 0;
    params->dsi.CLK_HS_PRPR = 6;
    //params->dsi.LPX=8; 

    // Bit rate calculation
    // 1 Every lane speed
	params->dsi.PLL_CLOCK 	= 162;
	params->dsi.ssc_disable = 1;	// ssc disable control (1: disable, 0: enable, default: 0)
	params->dsi.ssc_range 	= 5;	// ssc range control (1:min, 8:max, default: 5)
	params->physical_width=51;
  params->physical_height=86;
   // params->dsi.pll_div1=0;		// div1=0,1,2,3;div1_real=1,2,4,4 ----0: 546Mbps  1:273Mbps
   // params->dsi.pll_div2=1;		// div2=0,1,2,3;div1_real=1,2,4,4	
   // params->dsi.fbk_div =12 ;    // fref=26MHz, fvco = fref*(fbk_div)*2/(div1_real*div2_real) 
}

static void lcm_init(void)
{
    SET_RESET_PIN(1);
    MDELAY(1);
    SET_RESET_PIN(0);
    MDELAY(10);
    SET_RESET_PIN(1);
    MDELAY(120);
    push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
   // push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_suspend(void)
{
    push_table(lcm_sleep_in_setting, sizeof(lcm_sleep_in_setting) / sizeof(struct LCM_setting_table), 1);
}


static void lcm_resume(void)
{
    lcm_init();
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

	data_array[0]= 0x00290508; //HW bug, so need send one HS packet
	dsi_set_cmdq(data_array, 1, 1);
	
	data_array[0]= 0x002c3909;
	dsi_set_cmdq(data_array, 1, 0);

}
static struct LCM_setting_table lcm_compare_id_setting[] = {
        // Display off sequence
        {0xf0,  5,      {0x55,0xaa,0x52,0x08,0x01}},
        {REGFLAG_DELAY, 10, {}},
        {REGFLAG_END_OF_TABLE, 0x00, {}}
};
static unsigned int lcm_compare_id(void)
{
	unsigned int id = 0;
	unsigned char buffer[5];
	unsigned int array[16];

	SET_RESET_PIN(1);  //NOTE:should reset LCM firstly
	MDELAY(10);
	SET_RESET_PIN(0);
	MDELAY(50);
	SET_RESET_PIN(1);
	MDELAY(50);

	push_table(lcm_compare_id_setting, sizeof(lcm_compare_id_setting) / sizeof(struct LCM_setting_table), 1);

	array[0] = 0x00033700;
	dsi_set_cmdq(array,1,1);
	MDELAY(5);
	read_reg_v2(0xC5, buffer, 2);
	id = ((buffer[0] << 8) | buffer[1]); //we only need ID
	#ifdef BUILD_LK
	printf("zhengzhou nt35512 compare id==%d\n",id);
 #else
   printk("zhengzhou nt35512 compare id==%d\n",id);
 #endif

	return (LCM_NT35512_ID == id)?1:0;
}
// ---------------------------------------------------------------------------
//  Get LCM Driver Hooks
// ---------------------------------------------------------------------------
LCM_DRIVER nt35512_wvga_dsi_vdo_lcm_drv = 
{
    .name			= "nt35512_wvga_dsi_vdo_djn",
    .set_util_funcs = lcm_set_util_funcs,
    .compare_id     = lcm_compare_id,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
#if defined(LCM_DSI_CMD_MODE)
    .update         = lcm_update,
#endif
#ifdef WT_BRIGHTNESS_MAPPING_WITH_LCM
    .cust_mapping = lcm_brightness_mapping,
#endif
    //.esd_check      = lcm_esd_check,
    //.esd_recover    = lcm_esd_recover,
};

