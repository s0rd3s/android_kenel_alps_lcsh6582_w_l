/* drivers/input/touchscreen/test_params.h
 * 
 * 2010 - 2014 Shenzhen Huiding Technology Co.,Ltd.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be a reference 
 * to you, when you are integrating the GOODiX's CTP IC into your system, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * General Public License for more details.
 * 
 * Version: 1.0
 * Release Date: 2014/11/14
 *
 */
 
#ifndef _TEST_PARAMS_HEADER_
#define _TEST_PARAMS_HEADER_

#ifdef __cplusplus
//extern "C" {
#endif

#define TEST_TYPES                         0x40000
#define MAX_LIMIT_VALUE                    3000
#define MIN_LIMIT_VALUE                    300
#define ACCORD_LIMIT                       500
#define OFFEST_LIMIT                       30
#define PERMIT_JITTER_LIMIT                30
#define SPECIAL_LIMIT                      30
#define MAX_KEY_LIMIT_VALUE                3000
#define MIN_KEY_LIMIT_VALUE                3000
#define RAWDATA_UNIFORMITY                 250
#define MODULE_TYPE                        1
#define VERSION_EQU                        "1143_1.1.0_0.1"
#define VERSION_GREATER                    "1143_1.1.0_0.1"
#define VERSION_BETWEEN1                   "1143_1.1.0_0.1"
#define VERSION_BETWEEN2                   "1143_1.1.0_0.1"
#define GT900_SHORT_THRESHOLD              30
#define GT900_DRV_DRV_RESISTOR_THRESHOLD   800
#define GT900_DRV_SEN_RESISTOR_THRESHOLD   500
#define GT900_SEN_SEN_RESISTOR_THRESHOLD   300
#define GT900_RESISTOR_WARN_THRESHOLD      800
#define GT900_DRV_GND_RESISTOR_THRESHOLD   300
#define GT900_SEN_GND_RESISTOR_THRESHOLD   300
#define GT900_SHORT_TEST_TIMES             3
#define TRI_PATTERN_R_RATIO                150
#define AVDD                               280

#define NC               {\
	0\
	}
#define KEY_NC           {\
	}
#define MODULE_CFG       {\
	}
#define SEPCIALTESTNODE  {\
	0,6900,6300,0.050,4,5600,5100,0.500,8,5700,5200,0.500,12,5900,5400,0.500,\
	16,6000,5500,0.500,20,5850,5350,0.500,24,5900,5400,0.500,28,5900,5400,0.500,\
	32,6500,6000,0.500,36,3500,3200,0.500,1,6100,5600,0.500,5,5600,5100,0.500,\
	9,5600,5100,0.500,13,6000,5500,0.500,17,5800,5300,0.500,21,5800,5300,0.500,\
	25,6000,5500,0.500,29,5900,5400,0.500,33,6300,5800,0.500,37,4100,3800,0.500,\
	2,6300,5800,0.500,6,5650,5150,0.500,10,5650,5150,0.500,14,5600,5100,0.500,\
	18,4800,4300,0.500,22,5400,4900,0.500,26,6450,5950,0.500,30,6650,6150,0.500,\
	34,6200,5700,0.500,3,6900,6400,0.500,7,5250,4750,0.500,11,5300,4800,0.500,\
	15,5500,5000,0.500,19,4800,4300,0.500,23,4900,4400,0.500,27,5400,4900,0.500,\
	31,6100,5600,0.500,35,6500,6000,0.500\
	}

#ifdef __cplusplus
//}
#endif
#endif
