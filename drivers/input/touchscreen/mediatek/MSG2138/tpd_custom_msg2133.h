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

#ifndef __TPD_CUSTOM_MSG2133_H__
#define __TPD_CUSTOM_MSG2133_H__

#include <linux/xlog.h>
/* Pre-defined definition */
#define TPD_TYPE_CAPACITIVE
#define TPD_I2C_NUMBER           0

//#define TOUCH_ADDR_MSG20XX 0Xc0

#define TPD_DELAY                (2*HZ/100)

//#define SWAP_X_Y
//#define REVERSE_Y
//#define REVERSE_X

#define TPD_POWER_SOURCE_CUSTOM         MT6323_POWER_LDO_VGP1

#define MAX_TOUCH_FINGER      2
#define MS_TS_MSG21XX_X_MAX   480//480
#define MS_TS_MSG21XX_Y_MAX   800//800
#define REPORT_PACKET_LENGTH  8//2--8--128--80

#define TPD_HAVE_BUTTON
#define HAVE_TOUCH_KEY
#define TPD_BUTTON_HEIGH        (60)
#define TPD_KEY_COUNT           3

#define TPD_KEYS                {KEY_BACK, KEY_HOMEPAGE,KEY_MENU }
#define TPD_KEYS_DIM        {{80,900,80,60},{240,900,80,60},{400,900,80,60}}

#define MSG_DMA_MODE


#define FIRST_TP "shenyue"
#define SECOND_TP "shenyue"
 #define UNKNOWN_TP "unknown tp"

#define PROC_FIRMWARE_UPDATE

#define __FIRMWARE_UPDATE__

//#define TPD_PROXIMITY


#define FW_ADDR_MSG21XX   (0xC4>>1)
#define FW_ADDR_MSG21XX_TP   (0x4C>>1)
#define FW_UPDATE_ADDR_MSG21XX   (0x92>>1)
//#define MSG_AUTO_UPDATE 

#define WT_CTP_GESTURE_SUPPORT
#ifdef WT_CTP_GESTURE_SUPPORT
#define GTP_GESTURE_TPYE_STR  "K"
#define GTP_GLOVE_SUPPORT_ONOFF  'N'	// 'N' is off
#define GTP_GESTURE_SUPPORT_ONOFF   'Y'	// 'N' is off
#define GTP_DRIVER_VERSION          "GTP_V1.0_20140327"
#endif

#define WT_CTP_OPEN_SHORT_TEST

#define CTP_ADD_HW_INFO  1
#define CTP_DEBUG_ON    0

#define MSG_TAG "MSTAR-TP-TAG"
#define MSG_DMESG(fmt,arg...)          xlog_printk(ANDROID_LOG_INFO,MSG_TAG,"<<-CTP-INFO->> "fmt"\n",##arg)
#define MSG_ERROR(fmt,arg...)          xlog_printk(ANDROID_LOG_ERROR,MSG_TAG,"<<-CTP-ERROR->> "fmt"\n",##arg)
#define MSG_DEBUG(fmt,arg...)          do{\
                                            if(CTP_DEBUG_ON)\
                                            xlog_printk(ANDROID_LOG_DEBUG,MSG_TAG,"<<-CTP-DEBUG->> [%d]"fmt"\n",__LINE__, ##arg);\
                                        }while(0)
#endif /* TOUCHPANEL_H__ */




