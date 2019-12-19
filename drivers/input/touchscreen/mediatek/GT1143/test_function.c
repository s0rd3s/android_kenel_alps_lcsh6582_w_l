/* drivers/input/touchscreen/test_function.c
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

#ifdef __cplusplus
//extern "C" {
#endif

#include <linux/string.h>
#include "test_function.h"
#include "dsp_isp.h"

#if !GTP_TEST_PARAMS_FROM_INI 
#include "test_params.h"
#endif
/*----------------------------------- test param variable define-----------------------------------*/
/* sample data frame numbers */
int samping_set_number = 16;

/* raw data max threshold */
int *max_limit_value;
int max_limit_value_tmp;

/* raw data min threshold */
int *min_limit_value;
int min_limit_value_tmp;

/* ratio threshold between adjacent(up down right left) data */
long *accord_limit;
long accord_limit_tmp;

/* maximum deviation ratio,|(data - average)|/average */
long offset_limit = 150;

/* uniformity = minimum / maximum */
long uniformity_limit = 0;

/* further judgement for not meet the entire screen offset limit of the channel, 
   when the following conditions are satisfied that the legitimate:
   1 beyond the entire screen offset limit but not beyond the limits ,
   2 to meet the condition of 1 point number no more than 
   3 3 of these two non adjacent */
long special_limit = 250;

/* maximum jitter the entire screen data value */
int permit_jitter_limit = 20;

/* key raw data max threshold */
int *max_key_limit_value;
int max_key_limit_value_tmp;

/* key raw data min threshold */
int *min_key_limit_value;
int min_key_limit_value_tmp;

int ini_module_type;

unsigned char *ini_version1;

unsigned char *ini_version2;

unsigned short gt900_short_threshold = 10;
unsigned short gt900_short_dbl_threshold = 500;
unsigned short gt900_drv_drv_resistor_threshold = 500;;
unsigned short gt900_drv_sen_resistor_threshold = 500;
unsigned short gt900_sen_sen_resistor_threshold = 500;
unsigned short gt900_resistor_warn_threshold = 1000;
unsigned short gt900_drv_gnd_resistor_threshold = 400;
unsigned short gt900_sen_gnd_resistor_threshold = 400;
unsigned char gt900_short_test_times = 1;

long tri_pattern_r_ratio = 115;
int sys_avdd = 28;

/*----------------------------------- test input variable define-----------------------------------*/
unsigned short *current_data_temp;

unsigned short *channel_max_value;

unsigned short *channel_min_value;

/* average value of channel */
int *channel_average;

/* average current value of each channel of the square */
int *channel_square_average;

/* the test group number */
int current_data_index;

unsigned char *need_check;

unsigned char *channel_key_need_check;

/* max sample data number */
int samping_num = 64;

u8 *global_large_buf;

unsigned char *driver_status;
unsigned char *upload_short_data;

/*----------------------------------- test output variable define-----------------------------------*/
int test_error_code;

/* channel status,0 is normal,otherwise is anormaly */
unsigned short *channel_status;

long *channel_max_accord;

/* maximum number exceeds the set value */
unsigned char *beyond_max_limit_num;

/*  minimum number exceeds the set value */
unsigned char *beyond_min_limit_num;

/* deviation ratio exceeds the set value. */
unsigned char *beyond_accord_limit_num;

/* full screen data maximum deviation ratio exceeds the set value */
unsigned char *beyond_offset_limit_num;

/* maximum frequency jitter full screen data exceeds a set value */
unsigned char *beyond_jitter_limit_num;

/* the number of data consistency over the setting value */
unsigned char beyond_uniformity_limit_num;

/*----------------------------------- other define-----------------------------------*/
char save_result_dir[250] = "/sdcard/rawdata/";
char ini_find_dir1[250] = "/sdcard/";
char ini_find_dir2[250] = "/data/";
char ini_format[250] = "";

u8 *original_cfg;
u8 *module_cfg;

static s32 _node_in_key_chn(u16 node, u8 * config, u16 keyoffest)
{
	int ret = -1;
	u8 i, tmp, chn;

	if (node < sys.sc_sensor_num * sys.sc_driver_num) {
		return -1;
	}

	chn = node - sys.sc_sensor_num * sys.sc_driver_num;
	for (i = 0; i < 4; i++) {
		tmp = config[keyoffest + i];
		if ((tmp != 0) && (tmp % 8 != 0)) {
			return 1;
		}

		if (tmp == (chn + 1) * 8) {
			ret = 1;
		}
	}

	return ret;
}

static void _get_channel_min_value(void)
{
	int i;

	for (i = 0; i < sys.sc_sensor_num * sys.sc_driver_num; i++) {
		if (current_data_temp[i] < channel_min_value[i]) {
			channel_min_value[i] = current_data_temp[i];
		}
	}
}

static void _get_channel_max_value(void)
{
	int i;

	for (i = 0; i < sys.sc_sensor_num * sys.sc_driver_num; i++) {
		if (current_data_temp[i] > channel_max_value[i]) {
			channel_max_value[i] = current_data_temp[i];
		}
	}
}

static void _get_channel_average_value(void)
{
	int i;
	if (current_data_index == 0) {
		memset(channel_average, 0, sizeof(channel_average) / sizeof(channel_average[0]));
		memset(channel_average, 0, sizeof(channel_square_average) / sizeof(channel_square_average[0]));
	}

	for (i = 0; i < sys.sc_sensor_num * sys.sc_driver_num; i++) {
		channel_average[i] += current_data_temp[i] / samping_set_number;
		channel_square_average[i] += (current_data_temp[i] * current_data_temp[i]) / samping_set_number;
	}
}

static unsigned short _get_average_value(unsigned short *data)
{
	int i;
	int temp = 0;
	int not_check_num = 0;
	unsigned short average_temp = 0;

	for (i = 0; i < sys.sc_sensor_num * sys.sc_driver_num; i++) {
		if (need_check[i] == _NEED_NOT_CHECK) {
			not_check_num++;
			continue;
		}
		temp += data[i];
	}

	DEBUG("NOT CHECK NUM:%d\ntmp:%d\n", not_check_num, temp);
	if (not_check_num < sys.sc_sensor_num * sys.sc_driver_num) {
		average_temp = (unsigned short)(temp / (sys.sc_sensor_num * sys.sc_driver_num - not_check_num));
	}
	return average_temp;
}

#ifdef SIX_SIGMA_JITTER
static float _get_six_sigma_value(void)
{
	int i;
	float square_sigma = 0;
	for (i = 0; i < sys.sc_sensor_num * sys.sc_driver_num; i++) {
		square_sigma += channel_square_average[i] - (channel_average[i] * channel_average[i]);
	}
	square_sigma /= samping_set_number;
	return sqrt(square_sigma);
}
#endif /*
        */
static unsigned char _check_channel_min_value(void)
{
	int i;
	unsigned char test_result = 1;

	for (i = 0; i < sys.sc_sensor_num * sys.sc_driver_num; i++) {
		if (need_check[i] == _NEED_NOT_CHECK) {
			continue;
		}

		if (current_data_temp[i] < min_limit_value[i]) {
			channel_status[i] |= _BEYOND_MIN_LIMIT;
			test_error_code |= _BEYOND_MIN_LIMIT;
			beyond_min_limit_num[i]++;
			test_result = 0;
			//DEBUG("current[%d]%d,limit[%d]%d",i,current_data_temp[i],i,min_limit_value[i]);
		}
	}

	return test_result;
}

static unsigned char _check_channel_max_value(void)
{
	int i;
	unsigned char test_result = 1;
	for (i = 0; i < sys.sc_sensor_num * sys.sc_driver_num; i++) {
		if (need_check[i] == _NEED_NOT_CHECK) {
			continue;
		}

		if (current_data_temp[i] > max_limit_value[i]) {
			channel_status[i] |= _BEYOND_MAX_LIMIT;
			test_error_code |= _BEYOND_MAX_LIMIT;
			beyond_max_limit_num[i]++;
			test_result = 0;
		}
	}

	return test_result;
}

static unsigned char _check_key_min_value(void)
{
	int i, j;
	unsigned char test_result = 1;
	if (sys.key_number == 0) {
		return test_result;
	}

	for (i = sys.sc_sensor_num * sys.sc_driver_num, j = 0; i < sys.sc_sensor_num * sys.sc_driver_num + sys.key_number; i++) {
		if (channel_key_need_check[i - sys.sc_sensor_num * sys.sc_driver_num] == _NEED_NOT_CHECK) {
			continue;
		}

		if (_node_in_key_chn(i, module_cfg, sys.key_offest) < 0) {
			continue;
		}

		if (current_data_temp[i] < min_key_limit_value[j++]) {
			channel_status[i] |= _KEY_BEYOND_MIN_LIMIT;
			test_error_code |= _KEY_BEYOND_MIN_LIMIT;
			beyond_min_limit_num[i]++;
			test_result = 0;
		}
	}

	return test_result;
}

static unsigned char _check_key_max_value(void)
{
	int i, j;
	unsigned char test_result = 1;

	if (sys.key_number == 0) {
		return test_result;
	}

	for (i = sys.sc_sensor_num * sys.sc_driver_num, j = 0; i < sys.sc_sensor_num * sys.sc_driver_num + sys.key_number; i++) {
		if (channel_key_need_check[i - sys.sc_sensor_num * sys.sc_driver_num] == _NEED_NOT_CHECK) {
			continue;
		}

		if (_node_in_key_chn(i, module_cfg, sys.key_offest) < 0) {
			continue;
		}

		if (current_data_temp[i] > max_key_limit_value[j++]) {
			channel_status[i] |= _KEY_BEYOND_MAX_LIMIT;
			test_error_code |= _KEY_BEYOND_MAX_LIMIT;
			beyond_max_limit_num[i]++;
			test_result = 0;
		}
	}

	return test_result;
}

static unsigned char _check_area_accord(void)
{
	int i, j, index;
	long temp;
	long accord_temp;
	unsigned char test_result = 1;

	for (i = 0; i < sys.sc_sensor_num; i++) {
		for (j = 0; j < sys.sc_driver_num; j++) {
			index = i + j * sys.sc_sensor_num;

			accord_temp = 0;
			temp = 0;
			if (need_check[index] == _NEED_NOT_CHECK) {
				continue;
			}

			if (current_data_temp[index] == 0) {
				current_data_temp[index] = 1;
				continue;
			}

			if (j == 0) {
				if (need_check[i + (j + 1) * sys.sc_sensor_num] != _NEED_NOT_CHECK) {
					accord_temp =
					    (FLOAT_AMPLIFIER * abs((s16) (current_data_temp[i + (j + 1) * sys.sensor_num] - current_data_temp[index]))) / current_data_temp[index];
				}
			} else if (j == sys.sc_driver_num - 1) {
				if (need_check[i + (j - 1) * sys.sc_sensor_num] != _NEED_NOT_CHECK) {
					accord_temp =
					    (FLOAT_AMPLIFIER * abs((s16) (current_data_temp[i + (j - 1) * sys.sc_sensor_num] - current_data_temp[index]))) /
					    current_data_temp[index];
				}
			} else {
				if (need_check[i + (j + 1) * sys.sc_sensor_num] != _NEED_NOT_CHECK) {
					accord_temp =
					    (FLOAT_AMPLIFIER * abs((s16) (current_data_temp[i + (j + 1) * sys.sc_sensor_num] - current_data_temp[index]))) /
					    current_data_temp[index];
				}
				if (need_check[i + (j - 1) * sys.sc_sensor_num] != _NEED_NOT_CHECK) {
					temp =
					    (FLOAT_AMPLIFIER * abs((s16) (current_data_temp[i + (j - 1) * sys.sc_sensor_num] - current_data_temp[index]))) /
					    current_data_temp[index];
				}
				if (temp > accord_temp) {
					accord_temp = temp;
				}
			}

			if (i == 0) {
				if (need_check[i + 1 + j * sys.sc_sensor_num] != _NEED_NOT_CHECK) {
					temp =
					    (FLOAT_AMPLIFIER * abs((s16) (current_data_temp[i + 1 + j * sys.sc_sensor_num] - current_data_temp[index]))) / current_data_temp[index];
				}
				if (temp > accord_temp) {
					accord_temp = temp;
				}
			} else if (i == sys.sc_sensor_num - 1) {
				if (need_check[i - 1 + j * sys.sc_sensor_num] != _NEED_NOT_CHECK) {
					temp =
					    (FLOAT_AMPLIFIER * abs((s16) (current_data_temp[i - 1 + j * sys.sc_sensor_num] - current_data_temp[index]))) / current_data_temp[index];
				}
				if (temp > accord_temp) {
					accord_temp = temp;
				}
			} else {
				if (need_check[i + 1 + j * sys.sc_sensor_num] != _NEED_NOT_CHECK) {
					temp =
					    (FLOAT_AMPLIFIER * abs((s16) (current_data_temp[i + 1 + j * sys.sc_sensor_num] - current_data_temp[index]))) / current_data_temp[index];
				}
				if (temp > accord_temp) {
					accord_temp = temp;
				}
				if (need_check[i - 1 + j * sys.sc_sensor_num] != _NEED_NOT_CHECK) {
					temp =
					    (FLOAT_AMPLIFIER * abs((s16) (current_data_temp[i - 1 + j * sys.sc_sensor_num] - current_data_temp[index]))) / current_data_temp[index];
				}
				if (temp > accord_temp) {
					accord_temp = temp;
				}
			}

			channel_max_accord[index] = accord_temp;

			if (accord_temp > accord_limit[index]) {
				channel_status[index] |= _BEYOND_ACCORD_LIMIT;
				test_error_code |= _BEYOND_ACCORD_LIMIT;
				test_result = 0;
				beyond_accord_limit_num[index]++;
			}
		}
	}

	return test_result;
}

static unsigned char _check_full_screen_offest(unsigned char special_check)
{
	int average_temp = 0;
	int i, j;
	long offset_temp;
	int special_num = 0;
	int special_channel[_SPECIAL_LIMIT_CHANNEL_NUM];
	unsigned char test_result = 1;

	/* calculate the average value of total screen */
	average_temp = _get_average_value(current_data_temp);
	DEBUG("average:%d\n", average_temp);

	/* caculate the offset between the channel value and the average value */
	for (i = 0; i < sys.sc_sensor_num * sys.sc_driver_num; i++) {
		if (need_check[i] == _NEED_NOT_CHECK) {
			continue;
		}
		/* get the max ratio of the total screen,(current_chn_value - screen_average_value)/screen_average_value */
		offset_temp = abs((current_data_temp[i] - average_temp) * FLOAT_AMPLIFIER) / average_temp;

		/* if area accord test is pass,and then do not do the screen accord test */
		if ((channel_status[i] & _BEYOND_ACCORD_LIMIT) != _BEYOND_ACCORD_LIMIT) {
			continue;
		}
		/* current channel accord validity detection */
		if (offset_temp > offset_limit) {
			if ((special_check == _SPECIAL_CHECK) && (special_num < _SPECIAL_LIMIT_CHANNEL_NUM) && (offset_temp <= special_limit)) {
				if ((current_data_index == 0 || special_num == 0)) {
					special_channel[special_num] = i;
					special_num++;
				} else {
					for (j = 0; j < special_num; j++) {
						if (special_channel[j] == i) {
							break;
						}
					}
					if (j == special_num) {
						special_channel[special_num] = i;
						special_num++;
					}
				}
			} else {
				channel_status[i] |= _BEYOND_OFFSET_LIMIT;
				test_error_code |= _BEYOND_OFFSET_LIMIT;
				beyond_offset_limit_num[i]++;
				test_result = 0;
			}
		}		/* end of if (offset_temp > offset_limit) */
	}			/* end of for (i = 0; i < sys.sensor_num*sys.driver_num; i++) */
	if (special_check && test_result == 1) {
		for (i = special_num - 1; i > 0; i--) {
			for (j = i - 1; j >= 0; j--) {
				if ((special_channel[i] - special_channel[j] == 1)
				    || (special_channel[i] - special_channel[j] == sys.sc_driver_num)) {
					channel_status[special_channel[j]] |= _BEYOND_OFFSET_LIMIT;
					test_error_code |= _BEYOND_OFFSET_LIMIT;
					beyond_offset_limit_num[special_channel[j]]++;
					test_result = 0;
				}
			}
		}
	}			/* end of if (special_check && test_result == TRUE) */
	return test_result;
}

static unsigned char _check_full_screen_jitter(void)
{
	int j;
	unsigned short max_jitter = 0;
	unsigned char test_result = 1;
	unsigned short *shake_value;
#ifdef SIX_SIGMA_JITTER
	double six_sigma = 0;
#endif
	shake_value = (u16 *) (&global_large_buf[0]);

	for (j = 0; j < sys.sc_sensor_num * sys.sc_driver_num; j++) {
		if (need_check[j] == _NEED_NOT_CHECK) {
			continue;
		}
		shake_value[j] = channel_max_value[j] - channel_min_value[j];

		if (shake_value[j] > max_jitter) {
			max_jitter = shake_value[j];
		}
	}

#ifdef SIX_SIGMA_JITTER
	six_sigma = 6 * _get_six_sigma_value();
	/* if 6sigama>jitter_limit or max_jitter>jitter_limit+10，jitter is not legal */
	if ((six_sigma > permit_jitter_limit) || (max_jitter > permit_jitter_limit + 10))
#endif
	{
		for (j = 0; j < sys.sc_sensor_num * sys.sc_driver_num; j++) {
			if (shake_value[j] >= permit_jitter_limit) {
				channel_status[j] |= _BEYOND_JITTER_LIMIT;
				test_error_code |= _BEYOND_JITTER_LIMIT;
				test_result = 0;
				DEBUG("point %d beyond jitter limit", j);
			}
		}
	}
	return test_result;
}

static unsigned char _check_uniformity(void)
{
	u16 i = 0;
	u16 min_val = 0, max_val = 0;
	long uniformity = 0;
	unsigned char test_result = 1;

	min_val = current_data_temp[0];
	max_val = current_data_temp[0];
	for (i = 1; i < sys.sc_sensor_num * sys.sc_driver_num; i++) {
		if (need_check[i] == _NEED_NOT_CHECK) {
			continue;
		}
		if (current_data_temp[i] > max_val) {
			max_val = current_data_temp[i];
		}
		if (current_data_temp[i] < min_val) {
			min_val = current_data_temp[i];
		}
	}

	if (0 == max_val) {
		uniformity = 0;
	} else {
		uniformity = (min_val * FLOAT_AMPLIFIER) / max_val;
	}
	DEBUG("min_val: %d, max_val: %d, tp uniformity(x1000): %lx", min_val, max_val, uniformity);
	if (uniformity < uniformity_limit) {
		beyond_uniformity_limit_num++;
		//channel_status[i] |= _BEYOND_UNIFORMITY_LIMIT;
		test_error_code |= _BEYOND_UNIFORMITY_LIMIT;
		test_result = 0;
	}
	return test_result;
}

static s32 _check_modele_type(int type)
{
	int ic_id = read_sensorid();

	if (ic_id != read_sensorid()) {
		ic_id = read_sensorid();
		if (ic_id != read_sensorid()) {
			WARNING("Read many ID inconsistent");
			return -1;
		}
	}

	if ((ic_id | ini_module_type) < 0) {
		return ic_id < ini_module_type ? ic_id : ini_module_type;
	}

	if (ic_id != ini_module_type) {
		test_error_code |= _MODULE_TYPE_ERR;
	}

	return 0;
}

static s32 _check_device_version(int type)
{
	s32 ret = 0;

	if (type & _VER_EQU_CHECK) {
		DEBUG("ini version:%s\n", ini_version1);
		ret = check_version(ini_version1);
		if (ret != 0) {
			test_error_code |= _VERSION_ERR;
		}
	} else if (type & _VER_GREATER_CHECK) {
		DEBUG("ini version:%s\n", ini_version1);
		ret = check_version(ini_version1);
		if (ret == 1 || ret < 0) {
			test_error_code |= _VERSION_ERR;
		}
	} else if (type & _VER_BETWEEN_CHECK) {
		signed int ret1 = 0;
		signed int ret2 = 0;

		DEBUG("ini version1:%s\n", ini_version1);
		DEBUG("ini version2:%s\n", ini_version2);
		ret1 = check_version(ini_version1);
		ret2 = check_version(ini_version2);
		if (ret1 == 1 || ret2 == 2 || ret1 < 0 || ret2 < 0) {
			test_error_code |= _VERSION_ERR;
		}
	}

	return 0;
}

static unsigned char _rawdata_test_result_analysis(int check_types)
{
	int i;
	int accord_temp = 0;
	int temp;
	int error_code_temp = test_error_code;
	int err = 0;
	int test_end = 0;

	// screen max value check
	error_code_temp &= ~_BEYOND_MAX_LIMIT;
	if (((check_types & _MAX_CHECK) != 0) && ((test_error_code & _BEYOND_MAX_LIMIT) != 0)) {
		for (i = 0; i < sys.sc_sensor_num * sys.sc_driver_num; i++) {
			if ((channel_status[i] & _BEYOND_MAX_LIMIT) == _BEYOND_MAX_LIMIT) {
				if (beyond_max_limit_num[i] >= (samping_set_number * 9 / 10)) {
					error_code_temp |= _BEYOND_MAX_LIMIT;
					test_end |= _BEYOND_MAX_LIMIT;
				} else if (beyond_max_limit_num[i] > samping_set_number / 10) {
					error_code_temp |= _BEYOND_MAX_LIMIT;
					err |= _BEYOND_MAX_LIMIT;
				} else {
					channel_status[i] &= ~_BEYOND_MAX_LIMIT;
				}
			}
		}
	}
	/* touch key max value check */
	error_code_temp &= ~_KEY_BEYOND_MAX_LIMIT;
	if (((check_types & _KEY_MAX_CHECK) != 0) && ((test_error_code & _KEY_BEYOND_MAX_LIMIT) != 0)) {
		for (i = sys.sc_sensor_num * sys.sc_driver_num; i < sys.sc_sensor_num * sys.sc_driver_num + sys.key_number; i++) {
			if ((channel_status[i] & _KEY_BEYOND_MAX_LIMIT) == _KEY_BEYOND_MAX_LIMIT) {
				if (beyond_max_limit_num[i] >= (samping_set_number * 9 / 10)) {
					error_code_temp |= _KEY_BEYOND_MAX_LIMIT;
					test_end |= _KEY_BEYOND_MAX_LIMIT;
				} else if (beyond_max_limit_num[i] > samping_set_number / 10) {
					error_code_temp |= _KEY_BEYOND_MAX_LIMIT;
					err |= _KEY_BEYOND_MAX_LIMIT;
				} else {
					channel_status[i] &= ~_KEY_BEYOND_MAX_LIMIT;
				}
			}
		}
	}
	/* screen min value check */
	error_code_temp &= ~_BEYOND_MIN_LIMIT;
	if (((check_types & _MIN_CHECK) != 0) && ((test_error_code & _BEYOND_MIN_LIMIT) != 0)) {
		for (i = 0; i < sys.sc_sensor_num * sys.sc_driver_num; i++) {
			if ((channel_status[i] & _BEYOND_MIN_LIMIT) == _BEYOND_MIN_LIMIT) {
				if (beyond_min_limit_num[i] >= (samping_set_number * 9 / 10)) {
					error_code_temp |= _BEYOND_MIN_LIMIT;
					test_end |= _BEYOND_MIN_LIMIT;
				} else if (beyond_min_limit_num[i] > samping_set_number / 10) {
					error_code_temp |= _BEYOND_MIN_LIMIT;
					err |= _BEYOND_MIN_LIMIT;
				} else {
					channel_status[i] &= ~_BEYOND_MIN_LIMIT;
				}
			}
		}
	}
	/* touch key min value check */
	error_code_temp &= ~_KEY_BEYOND_MIN_LIMIT;
	if (((check_types & _KEY_MIN_CHECK) != 0) && ((test_error_code & _KEY_BEYOND_MIN_LIMIT) != 0)) {
		for (i = sys.sc_sensor_num * sys.sc_driver_num; i < sys.sc_sensor_num * sys.sc_driver_num + sys.key_number; i++) {
			if ((channel_status[i] & _KEY_BEYOND_MIN_LIMIT) == _KEY_BEYOND_MIN_LIMIT) {
				if (beyond_min_limit_num[i] >= (samping_set_number * 9 / 10)) {
					error_code_temp |= _KEY_BEYOND_MIN_LIMIT;
					test_end |= _KEY_BEYOND_MIN_LIMIT;
				} else if (beyond_min_limit_num[i] > samping_set_number / 10) {
					error_code_temp |= _KEY_BEYOND_MIN_LIMIT;
					err |= _KEY_BEYOND_MIN_LIMIT;
				} else {
					channel_status[i] &= ~_KEY_BEYOND_MIN_LIMIT;
				}
			}
		}
	}
	/* screen uniformity check */
	error_code_temp &= ~_BEYOND_UNIFORMITY_LIMIT;
	if (((check_types & _UNIFORMITY_CHECK) != 0) && ((test_error_code & _BEYOND_UNIFORMITY_LIMIT) != 0)) {
		DEBUG("beyond_uniformity_limit_num:%d", beyond_uniformity_limit_num);
		if (beyond_uniformity_limit_num >= (samping_set_number * 9 / 10)) {
			error_code_temp |= _BEYOND_UNIFORMITY_LIMIT;
			test_end |= _BEYOND_UNIFORMITY_LIMIT;
		} else if (beyond_uniformity_limit_num > samping_set_number / 10) {
			error_code_temp |= _BEYOND_UNIFORMITY_LIMIT;
			err |= _BEYOND_UNIFORMITY_LIMIT;
			DEBUG("beyond_uniformity_limit_num:%d", beyond_uniformity_limit_num);
		}
	}
	/* adjacent data accord check */
	error_code_temp &= ~_BEYOND_ACCORD_LIMIT;
	if (((check_types & _ACCORD_CHECK) != 0) && ((test_error_code & _BEYOND_ACCORD_LIMIT) != 0)) {
		for (i = 0; i < sys.sc_sensor_num * sys.sc_driver_num; i++) {
			if ((channel_status[i] & _BEYOND_ACCORD_LIMIT) == _BEYOND_ACCORD_LIMIT) {
				if (beyond_accord_limit_num[i] >= (samping_set_number * 9 / 10)) {
					error_code_temp |= _BEYOND_ACCORD_LIMIT;
					test_end |= _BEYOND_ACCORD_LIMIT;
					accord_temp++;
				} else if (beyond_accord_limit_num[i] > samping_set_number / 10) {
					error_code_temp |= _BEYOND_ACCORD_LIMIT;
					err |= _BEYOND_ACCORD_LIMIT;
					accord_temp++;
				} else {
					channel_status[i] &= ~_BEYOND_ACCORD_LIMIT;
				}
			}
		}
	}
	/* screen max accord check */
	error_code_temp &= ~_BEYOND_OFFSET_LIMIT;
	if (((check_types & _OFFSET_CHECK) != 0) && ((test_error_code & _BEYOND_OFFSET_LIMIT) != 0)) {
		for (i = 0; i < sys.sc_sensor_num * sys.sc_driver_num; i++) {
			if ((channel_status[i] & _BEYOND_OFFSET_LIMIT) == _BEYOND_OFFSET_LIMIT) {
				if (beyond_offset_limit_num[i] >= (samping_set_number * 9 / 10)) {
					error_code_temp |= _BEYOND_OFFSET_LIMIT;
					test_end |= _BEYOND_OFFSET_LIMIT;
				} else if (beyond_offset_limit_num[i] > samping_set_number / 10) {
					error_code_temp |= _BEYOND_OFFSET_LIMIT;
					err |= _BEYOND_OFFSET_LIMIT;
				} else {
					channel_status[i] &= ~_BEYOND_OFFSET_LIMIT;
				}
			}
		}
	}

	if (1) {		/* (sys.AccordOrOffsetNG == FALSE) */
		if (((check_types & _ACCORD_CHECK) != 0) && ((check_types & _OFFSET_CHECK) != 0)) {
			for (i = 0; i < sys.sc_sensor_num * sys.sc_driver_num; i++) {
				if (((channel_status[i] & _BEYOND_OFFSET_LIMIT) != _BEYOND_OFFSET_LIMIT)
				    && ((channel_status[i] & _BEYOND_ACCORD_LIMIT) == _BEYOND_ACCORD_LIMIT)) {
					channel_status[i] &= ~_BEYOND_ACCORD_LIMIT;
					accord_temp--;
				}
			}
			if (accord_temp == 0) {
				error_code_temp &= ~_BEYOND_ACCORD_LIMIT;
				test_end &= ~_BEYOND_ACCORD_LIMIT;
				err &= ~_BEYOND_ACCORD_LIMIT;
			}
		}
	}

	error_code_temp |= (test_error_code & _BEYOND_JITTER_LIMIT);

	test_error_code = error_code_temp;
	DEBUG("test_end:0x%0x err:0x%0x", test_end, err);
	if (test_end != _CHANNEL_PASS) {
		return 1;
	}

	if (err != _CHANNEL_PASS) {
		if ((check_types & _FAST_TEST_MODE) != 0) {
			if (samping_set_number < samping_num) {
				temp = samping_set_number;
				samping_set_number += (samping_num / 4);
				for (i = 0; i < sys.sc_sensor_num * sys.sc_driver_num; i++) {
					channel_average[i] = channel_average[i] * temp / samping_set_number;
					channel_square_average[i] += channel_square_average[i] * temp / samping_set_number;
				}
				return 0;
			}
		}
	}

	return 1;
}

#if GTP_SAVE_TEST_DATA
static s32 _save_testing_data(char *save_test_data_dir, int test_types)
{
	FILE *fp = NULL;
	s32 ret;
	s32 tmp = 0;
	u8 *data = NULL;
	s32 i = 0, j;
	s32 bytes = 0;
	int max, min;
	int average;

	DEBUG("_save_testing_data");
	data = (u8 *) malloc(MAX_BUFFER_SIZE);
	if (NULL == data) {
		WARNING("memory error!");
		return MEMORY_ERR;
	}

	fp = fopen((const char *)save_test_data_dir, "a+");
	if (NULL == fp) {
		WARNING("open %s failed!", save_test_data_dir);
		free(data);
		return FILE_OPEN_CREATE_ERR;
	}

	if (current_data_index == 0) {
		bytes = (s32) sprintf((char *)data, "Config:\n");
		for (i = 0; i < sys.config_length; i++) {
			bytes += (s32) sprintf((char *)&data[bytes], "0x%02X,", module_cfg[i]);
		}
		bytes += (s32) sprintf((char *)&data[bytes], "\n\n");
		ret = fwrite(data, bytes, 1, fp);
		bytes = 0;
		if (ret < 0) {
			WARNING("write to file fail.");
			goto exit_save_testing_data;
		}

		if ((test_types & _MAX_CHECK) != 0) {
			bytes = (s32) sprintf((char *)data, "Channel maximum:\n");
			for (i = 0; i < sys.sc_sensor_num; i++) {
				for (j = 0; j < sys.sc_driver_num; j++) {
					bytes += (s32) sprintf((char *)&data[bytes], "%d,", max_limit_value[i + j * sys.sc_sensor_num]);
				}
				bytes += (s32) sprintf((char *)&data[bytes], "\n");
				ret = fwrite(data, bytes, 1, fp);
				bytes = 0;
				if (ret < 0) {
					WARNING("write to file fail.");
					goto exit_save_testing_data;
				}
			}

			j = 0;
			bytes = 0;
			for (i = 0; i < sys.key_number; i++) {
				if (_node_in_key_chn(i + sys.sc_sensor_num * sys.sc_driver_num, module_cfg, sys.key_offest) < 0) {
					continue;
				}
				bytes += (s32) sprintf((char *)&data[bytes], "%d,", max_key_limit_value[j++]);
				/* DEBUG("max[%d]%d",i,max_key_limit_value[i]); */
			}
			bytes += (s32) sprintf((char *)&data[bytes], "\n");
			ret = fwrite(data, bytes, 1, fp);
			bytes = 0;
			if (ret < 0) {
				WARNING("write to file fail.");
				goto exit_save_testing_data;
			}
		}

		if ((test_types & _MIN_CHECK) != 0) {
			bytes = (s32) sprintf((char *)data, "Channel minimum:\n");
			for (i = 0; i < sys.sc_sensor_num; i++) {
				for (j = 0; j < sys.sc_driver_num; j++) {
					bytes += (s32) sprintf((char *)&data[bytes], "%d,", min_limit_value[i + j * sys.sc_sensor_num]);
				}
				bytes += (s32) sprintf((char *)&data[bytes], "\n");
				ret = fwrite(data, bytes, 1, fp);
				bytes = 0;
				if (ret < 0) {
					WARNING("write to file fail.");
					goto exit_save_testing_data;
				}
			}

			j = 0;
			bytes = 0;
			for (i = 0; i < sys.key_number; i++) {
				if (_node_in_key_chn(i + sys.sc_sensor_num * sys.sc_driver_num, module_cfg, sys.key_offest) < 0) {
					continue;
				}
				bytes += (s32) sprintf((char *)&data[bytes], "%d,", min_key_limit_value[j++]);
			}
			bytes += (s32) sprintf((char *)&data[bytes], "\n");
			ret = fwrite(data, bytes, 1, fp);
			if (ret < 0) {
				WARNING("write to file fail.");
				goto exit_save_testing_data;
			}
		}

		if ((test_types & _ACCORD_CHECK) != 0) {
			bytes = (s32) sprintf((char *)data, "Channel average:(%d)\n", FLOAT_AMPLIFIER);
			for (i = 0; i < sys.sc_sensor_num; i++) {
				for (j = 0; j < sys.sc_driver_num; j++) {
					bytes += (s32) sprintf((char *)&data[bytes], "%ld,", accord_limit[i + j * sys.sc_sensor_num]);
				}
				bytes += (s32) sprintf((char *)&data[bytes], "\n");
				ret = fwrite(data, bytes, 1, fp);
				bytes = 0;
				if (ret < 0) {
					WARNING("write to file fail.");
					goto exit_save_testing_data;
				}
			}

			bytes = (s32) sprintf((char *)data, "\n");
			ret = fwrite(data, bytes, 1, fp);
			if (ret < 0) {
				WARNING("write to file fail.");
				goto exit_save_testing_data;
			}
		}

		bytes = (s32) sprintf((char *)data, " Rawdata\n");
		ret = fwrite(data, bytes, 1, fp);
		if (ret < 0) {
			WARNING("write to file fail.");
			goto exit_save_testing_data;
		}
	}

	bytes = (s32) sprintf((char *)data, "No.%d\n", current_data_index);
	ret = fwrite(data, bytes, 1, fp);
	if (ret < 0) {
		WARNING("write to file fail.");
		goto exit_save_testing_data;
	}

	average = max = min = current_data_temp[0];
	for (i = 0; i < sys.sc_sensor_num; i++) {
		bytes = 0;
		for (j = 0; j < sys.sc_driver_num; j++) {
			tmp = current_data_temp[i + j * sys.sc_sensor_num];
			bytes += (s32) sprintf((char *)&data[bytes], "%d,", tmp);
			if (tmp > max) {
				max = tmp;
			}
			if (tmp < min) {
				min = tmp;
			}
			average = (average + tmp) / 2;
		}
		bytes += (s32) sprintf((char *)&data[bytes], "\n");
		ret = fwrite(data, bytes, 1, fp);
		if (ret < 0) {
			WARNING("write to file fail.");
			goto exit_save_testing_data;
		}
	}

	if (sys.key_number > 0) {
		bytes = (s32) sprintf((char *)data, "Key Rawdata:\n");
		for (i = 0; i < sys.key_number; i++) {
			if (_node_in_key_chn(i + sys.sc_sensor_num * sys.sc_driver_num, module_cfg, sys.key_offest) < 0) {
				continue;
			}
			bytes += (s32) sprintf((char *)&data[bytes], "%d,", current_data_temp[i + sys.sc_sensor_num * sys.sc_driver_num]);
		}
		bytes += (s32) sprintf((char *)&data[bytes], "\n");
		ret = fwrite(data, bytes, 1, fp);
		if (ret < 0) {
			WARNING("write to file fail.");
			goto exit_save_testing_data;
		}
	}

	bytes = (s32) sprintf((char *)data, "  Maximum:%d  Minimum:%d  Average:%d\n\n", max, min, average);
	ret = fwrite(data, bytes, 1, fp);
	if (ret < 0) {
		WARNING("write to file fail.");
		goto exit_save_testing_data;
	}

	if ((test_types & _ACCORD_CHECK) != 0) {
		bytes = (s32) sprintf((char *)data, "Channel_Accord :(%d)\n", FLOAT_AMPLIFIER);
		ret = fwrite(data, bytes, 1, fp);
		if (ret < 0) {
			WARNING("write to file fail.");
			goto exit_save_testing_data;
		}
		for (i = 0; i < sys.sc_sensor_num; i++) {
			bytes = 0;
			for (j = 0; j < sys.sc_driver_num; j++) {
				bytes += (s32) sprintf((char *)&data[bytes], "%ld,", channel_max_accord[i + j * sys.sc_sensor_num]);
			}
			bytes += (s32) sprintf((char *)&data[bytes], "\n");
			ret = fwrite(data, bytes, 1, fp);
			if (ret < 0) {
				WARNING("write to file fail.");
				goto exit_save_testing_data;
			}
		}
		bytes = (s32) sprintf((char *)data, "\n");
		ret = fwrite(data, bytes, 1, fp);
		if (ret < 0) {
			WARNING("write to file fail.");
			goto exit_save_testing_data;
		}
	}
exit_save_testing_data:
	/* DEBUG("step4"); */
	free(data);
	/* DEBUG("step3"); */
	fclose(fp);
	/* DEBUG("step2"); */
	return ret;
}

static s32 _save_test_result_data(char *save_test_data_dir, int test_types, u8 * shortresult)
{
	FILE *fp = NULL;
	s32 ret, index;
	u8 *data = NULL;
	s32 bytes = 0;
	data = (u8 *) malloc(MAX_BUFFER_SIZE);
	if (NULL == data) {
		WARNING("memory error!");
		return MEMORY_ERR;
	}

	fp = fopen((const char *)save_test_data_dir, "a+");
	if (NULL == fp) {
		WARNING("open %s failed!", save_test_data_dir);
		free(data);
		return FILE_OPEN_CREATE_ERR;
	}

	bytes = (s32) sprintf((char *)data, "Test Result:");
	if (test_error_code == _CHANNEL_PASS) {
		bytes += (s32) sprintf((char *)&data[bytes], "Pass\n\n");
	} else {
		bytes += (s32) sprintf((char *)&data[bytes], "Fail\n\n");
	}
	bytes += (s32) sprintf((char *)&data[bytes], "Test items:\n");
	if ((test_types & _MAX_CHECK) != 0) {
		bytes += (s32) sprintf((char *)&data[bytes], "Max Rawdata:  ");
		if (test_error_code & _BEYOND_MAX_LIMIT) {
			bytes += (s32) sprintf((char *)&data[bytes], "NG !\n");
		} else {
			bytes += (s32) sprintf((char *)&data[bytes], "pass\n");
		}
	}

	if ((test_types & _MIN_CHECK) != 0) {
		bytes += (s32) sprintf((char *)&data[bytes], "Min Rawdata:  ");
		if (test_error_code & _BEYOND_MIN_LIMIT) {
			bytes += (s32) sprintf((char *)&data[bytes], "NG !\n");
		} else {
			bytes += (s32) sprintf((char *)&data[bytes], "pass\n");
		}
	}

	if ((test_types & _ACCORD_CHECK) != 0) {
		bytes += (s32) sprintf((char *)&data[bytes], "Area Accord:  ");

		if (test_error_code & _BEYOND_ACCORD_LIMIT) {

			bytes += (s32) sprintf((char *)&data[bytes], "NG !\n");

		}

		else {

			bytes += (s32) sprintf((char *)&data[bytes], "pass\n");

		}
	}

	if ((test_types & _OFFSET_CHECK) != 0) {

		bytes += (s32) sprintf((char *)&data[bytes], "Max Offest:  ");

		if (test_error_code & _BEYOND_OFFSET_LIMIT) {

			bytes += (s32) sprintf((char *)&data[bytes], "NG !\n");

		}

		else {

			bytes += (s32) sprintf((char *)&data[bytes], "pass\n");

		}
	}

	if ((test_types & _JITTER_CHECK) != 0) {

		bytes += (s32) sprintf((char *)&data[bytes], "Max Jitier:  ");

		if (test_error_code & _BEYOND_JITTER_LIMIT) {

			bytes += (s32) sprintf((char *)&data[bytes], "NG !\n");

		}

		else {

			bytes += (s32) sprintf((char *)&data[bytes], "pass\n");

		}
	}

	if (test_types & _UNIFORMITY_CHECK) {

		bytes += (s32) sprintf((char *)&data[bytes], "Uniformity:  ");

		if (test_error_code & _BEYOND_UNIFORMITY_LIMIT) {

			bytes += (s32) sprintf((char *)&data[bytes], "NG !\n");

		}

		else {

			bytes += (s32) sprintf((char *)&data[bytes], "pass\n");

		}
	}

	if ((test_types & _KEY_MAX_CHECK) != 0) {

		bytes += (s32) sprintf((char *)&data[bytes], "Key Max Rawdata:  ");

		if (test_error_code & _KEY_BEYOND_MAX_LIMIT) {

			bytes += (s32) sprintf((char *)&data[bytes], "NG !\n");

		}

		else {

			bytes += (s32) sprintf((char *)&data[bytes], "pass\n");

		}
	}

	if ((test_types & _KEY_MIN_CHECK) != 0) {

		bytes += (s32) sprintf((char *)&data[bytes], "Key Min Rawdata:  ");

		if (test_error_code & _KEY_BEYOND_MIN_LIMIT) {

			bytes += (s32) sprintf((char *)&data[bytes], "NG !\n");

		}

		else {

			bytes += (s32) sprintf((char *)&data[bytes], "pass\n");

		}
	}

	if (test_types & (_VER_EQU_CHECK | _VER_GREATER_CHECK | _VER_BETWEEN_CHECK)) {

		bytes += (s32) sprintf((char *)&data[bytes], "Device Version:  ");

		if (test_error_code & _VERSION_ERR) {

			bytes += (s32) sprintf((char *)&data[bytes], "NG !\n");

		}

		else {

			bytes += (s32) sprintf((char *)&data[bytes], "pass\n");

		}
	}

	if (test_types & _MODULE_TYPE_CHECK) {

		bytes += (s32) sprintf((char *)&data[bytes], "Module Type:  ");

		if (test_error_code & _MODULE_TYPE_ERR) {

			bytes += (s32) sprintf((char *)&data[bytes], "NG !\n");

		}

		else {

			bytes += (s32) sprintf((char *)&data[bytes], "pass\n");

		}
	}

	ret = fwrite(data, bytes, 1, fp);

	if (ret < 0) {

		WARNING("write to file fail.");

		free(data);

		fclose(fp);

		return ret;

	}

	if ((test_types & _MODULE_SHORT_CHECK) != 0) {

		bytes = (s32) sprintf((char *)data, "Module short test:  ");

		if (test_error_code & _GT_SHORT) {

			bytes += (s32) sprintf((char *)&data[bytes], "NG !\n\n\nError items:\nShort:\n");

			if (shortresult[0] > _GT9_UPLOAD_SHORT_TOTAL) {

				WARNING("short total over limit, data error!");

				shortresult[0] = 0;

			}

			for (index = 0; index < shortresult[0]; index++) {

				/* DEBUG("bytes=%d shortresult[0]=%d",bytes,shortresult[0]); */
				if (shortresult[1 + index * 4] & 0x80) {

					bytes += (s32) sprintf((char *)&data[bytes], "Drv%d - ", shortresult[1 + index * 4] & 0x7F);

				}

				else {

					if (shortresult[1 + index * 4] == (_DRV_TOTAL_NUM + 1)) {

						bytes += (s32) sprintf((char *)&data[bytes], "GND\\VDD%d - ", shortresult[1 + index * 4] & 0x7F);

					}

					else {

						bytes += (s32) sprintf((char *)&data[bytes], "Sen%d - ", shortresult[1 + index * 4] & 0x7F);

					}
				}
				if (shortresult[2 + index * 4] & 0x80) {

					bytes += (s32) sprintf((char *)&data[bytes], "Drv%d 之间短路", shortresult[2 + index * 4] & 0x7F);

				}

				else {

					if (shortresult[2 + index * 4] == (_DRV_TOTAL_NUM + 1)) {

						bytes += (s32) sprintf((char *)&data[bytes], "GND\\VDD 之间短路");

					}

					else {

						bytes += (s32) sprintf((char *)&data[bytes], "Sen%d 之间短路", shortresult[2 + index * 4] & 0x7F);

					}
				}
				bytes += (s32) sprintf((char *)&data[bytes], "(R=%d Kohm)\n", (((shortresult[3 + index * 4] << 8) + shortresult[4 + index * 4]) & 0xffff) / 10);

				DEBUG("%d&%d:", shortresult[1 + index * 4], shortresult[2 + index * 4]);

				DEBUG("%dK", (((shortresult[3 + index * 4] << 8) + shortresult[4 + index * 4]) & 0xffff) / 10);

			}
		}

		else {

			bytes += (s32) sprintf((char *)&data[bytes], "pass\n");

		}
		ret = fwrite(data, bytes, 1, fp);

		if (ret < 0) {

			WARNING("write to file fail.");

			free(data);

			fclose(fp);

			return ret;

		}

	}

	free(data);

	fclose(fp);

	return 1;

}

#endif

static void _unzip_nc(unsigned char *s_buf, unsigned char *key_nc_buf, unsigned short Length, unsigned short KeyNcLength)
{

	unsigned short i, point;

	unsigned char m, data;

	int b_size = _DRV_TOTAL_NUM * _SEN_TOTAL_NUM;

	u8 *tmp;
        tmp = (u8*)(&global_large_buf[0]);

	DEBUG("sensor:%d, driver:%d\n", _SEN_TOTAL_NUM, _DRV_TOTAL_NUM);

	memset(need_check, 0, b_size);

	memset(tmp, 0, b_size);

	point = 0;

	for (i = 0; i < Length; i++) {

		data = *s_buf;

		for (m = 0; m < 8; m++) {

			if (point >= b_size) {

				goto KEY_NC_UNZIP;

			}

			tmp[point] &= 0xfe;

			if ((data & 0x80) == 0x80) {

				tmp[point] |= 0x01;

			}

			data <<= 1;

			point++;

			// DEBUG("POINT:%d\n", point);
			/*
			   if (point >= sys.SensorNum * sys.DriverNum)
			   {
			   break;
			   }
			 */
		}

		s_buf++;

	}

	/* memcpy(need_check,tmp,sys.sensor_num*sys.sc_driver_num); */
KEY_NC_UNZIP:
	DEBUG("sys.sc_driver_num:%d", sys.sc_driver_num);

	for (i = 0, point = 0; i < sys.sc_driver_num; i++) {

		for (m = 0; m < sys.sc_sensor_num; m++) {

			need_check[point++] = tmp[i + m * sys.sc_driver_num];

		}

	}

	DEBUG("Load key nc\n");

	memset(channel_key_need_check, 0, MAX_KEY_RAWDATA);

	point = 0;

	for (i = 0; i < KeyNcLength; i++) {

		data = *key_nc_buf;

		for (m = 0; m < 8; m++) {

			if (point >= MAX_KEY_RAWDATA) {

				return;

			}

			channel_key_need_check[point] &= 0xfe;

			if ((data & 0x80) == 0x80) {

				channel_key_need_check[point] |= 0x01;

			}

			data <<= 1;

			point++;

		}

		key_nc_buf++;

	}

}

#if GTP_TEST_PARAMS_FROM_INI
static s32 _init_special_node(char *inipath)
{

	FILE *fp = NULL;

	s8 *buf = NULL;

	s8 *tmp = NULL;

	size_t bytes = 0;

	s32 i = 0, space_count = 0;

	u16 tmpNode = 0, max, min;

	long accord;

	int b_size = _SEN_TOTAL_NUM * _DRV_TOTAL_NUM;

	if (NULL == inipath) {
		return PARAMETERS_ILLEGL;
	}

	buf = (s8 *) malloc(b_size * 4 * 6);

	if (NULL == buf) {

		return MEMORY_ERR;

	}

	fp = fopen((const char *)inipath, "r");

	if (fp == NULL) {

		free(buf);

		buf = NULL;

		DEBUG("open %s fail!", inipath);

		return INI_FILE_OPEN_ERR;

	}

	/* while(!feof(fp)) */
	while (1) {

		i = 0;

		space_count = 0;

		do {

			bytes = fread(&buf[i], 1, 1, fp);

			if (i >= b_size * 4 * 6 || bytes < 0) {

				fclose(fp);

				free(buf);

				return INI_FILE_READ_ERR;

			}
			/* DEBUG("%c", buf[i]); */

			if (buf[i] == ' ') {

				continue;

			}

		} while (buf[i] != '\r' && buf[i++] != '\n');

		buf[i] = '\0';

		getrid_space(buf, i);

		strtok((char *)buf, "=");

		if (0 == strcmp((const char *)buf, (const char *)"SepcialTestNode")) {

			i = 0;

			DEBUG("Begin get node data.");

			do {

				tmp = (s8 *) strtok((char *)NULL, ",");

				if (tmp == NULL) {

					break;

				}

				tmpNode = goodix_atoi((char const *)tmp);

				/* DEBUG("tmpNode:%d", tmpNode); */

				tmp = (s8 *) strtok((char *)NULL, ",");

				if (tmp == NULL) {

					fclose(fp);

					free(buf);

					return INI_FILE_ILLEGAL;

				}

				max = goodix_atoi((char const *)tmp);

				/* DEBUG("max:%d", max); */

				tmp = (s8 *) strtok((char *)NULL, ",");

				if (tmp == NULL) {

					fclose(fp);

					free(buf);

					return INI_FILE_ILLEGAL;

				}

				min = goodix_atoi((char const *)tmp);

				/* DEBUG("min:%d", min); */

				tmp = (s8 *) strtok((char *)NULL, ",");

				if (tmp == NULL) {

					fclose(fp);

					free(buf);

					return INI_FILE_ILLEGAL;

				}

				accord = atof((char const *)tmp);

				/* DEBUG("accord:%d", accord); */

				if (tmpNode < sys.sc_driver_num * sys.sc_sensor_num) {

					tmpNode = tmpNode / sys.sc_driver_num + (tmpNode % sys.sc_driver_num) * sys.sc_sensor_num;

					max_limit_value[tmpNode] = max;

					min_limit_value[tmpNode] = min;

					accord_limit[tmpNode] = accord;

				}

				else {

					tmpNode -= sys.sc_driver_num * sys.sc_sensor_num;

					max_key_limit_value[tmpNode] = max;

					min_key_limit_value[tmpNode] = min;

				}

			} while (i++ <= b_size);

			/* DEBUG("get node data end."); */
			fclose(fp);

			free(buf);

			return 1;

		}

	}

	fclose(fp);

	free(buf);

	return INI_FILE_ILLEGAL;

}
#else
static s32 _init_special_node_array(void)
{

	s32 i = 0;

	u16 tmpNode = 0;
	const u16 special_node_tmp[] = SEPCIALTESTNODE;

	for (i = 0; i < sizeof(special_node_tmp)/sizeof(u16); i += 4) {
		tmpNode = special_node_tmp[i];
		if (tmpNode < sys.sc_driver_num * sys.sc_sensor_num) {
			tmpNode = tmpNode / sys.sc_driver_num + (tmpNode % sys.sc_driver_num) * sys.sc_sensor_num;

			max_limit_value[tmpNode] = special_node_tmp[i + 1];

			min_limit_value[tmpNode] = special_node_tmp[i + 2];

			accord_limit[tmpNode] = special_node_tmp[i + 3];

		} else {
			DEBUG("tmpNode:%d,sc_driver_num:%d,sc_sensor_num:%d, i:%d",tmpNode,sys.sc_driver_num,sys.sc_sensor_num,i);//\u6dfb\u52a0log
			tmpNode -= sys.sc_driver_num * sys.sc_sensor_num;

			max_key_limit_value[tmpNode] = special_node_tmp[i + 1];

			min_key_limit_value[tmpNode] = special_node_tmp[i + 2];
		}
	}

	return i / 4;
}
#endif
static s32 _check_rawdata_proc(int check_types, u16 * data, int len, char *save_path)
{

	if (data == NULL || len < sys.driver_num * sys.sensor_num) {

		return PARAMETERS_ILLEGL;

	}

	memcpy(current_data_temp, data, sys.driver_num * sys.sensor_num * 2);

	_get_channel_max_value();

	_get_channel_min_value();

	_get_channel_average_value();

	if ((check_types & _MAX_CHECK) != 0) {

		_check_channel_max_value();

		DEBUG("After max check\n");

		DEBUG_DATA(channel_status, sys.driver_num * sys.sensor_num);

	}

	if ((check_types & _MIN_CHECK) != 0) {

		_check_channel_min_value();

		DEBUG("After min check\n");

		DEBUG_DATA(channel_status, sys.driver_num * sys.sensor_num);

	}

	if ((check_types & _KEY_MAX_CHECK) != 0) {

		_check_key_max_value();

		DEBUG("After key max check\n");

		DEBUG_DATA(channel_status, sys.driver_num * sys.sensor_num);

	}

	if ((check_types & _KEY_MIN_CHECK) != 0) {

		_check_key_min_value();

		DEBUG("After key min check\n");

		DEBUG_DATA(channel_status, sys.driver_num * sys.sensor_num);

	}

	if ((check_types & _ACCORD_CHECK) != 0) {

		_check_area_accord();

		DEBUG("After area accord check\n");

		DEBUG_DATA(channel_status, sys.driver_num * sys.sensor_num);

	}

	if ((check_types & _OFFSET_CHECK) != 0) {

		_check_full_screen_offest(check_types & _SPECIAL_CHECK);

		DEBUG("After offset check:%ld", offset_limit);

		DEBUG_DATA(channel_status, sys.driver_num * sys.sensor_num);

	}

	if ((check_types & _UNIFORMITY_CHECK) != 0) {

		_check_uniformity();

		DEBUG("After uniformity check:%ld\n", uniformity_limit);

	}
#if GTP_SAVE_TEST_DATA
//  if ((check_types & _TEST_RESULT_SAVE) != 0)
	{
		_save_testing_data(save_path, check_types);
	}

#endif /* 
        */

	DEBUG("The %d group rawdata test end!\n", current_data_index);

	current_data_index++;

	if (current_data_index < samping_set_number) {

		return 0;

	}

	if ((check_types & _JITTER_CHECK) != 0) {

		_check_full_screen_jitter();

		DEBUG("After FullScreenJitterCheck\n");

	}

	if (_rawdata_test_result_analysis(check_types) == 0) {

		DEBUG("After TestResultAnalyse\n");

		return 0;

	}

	DEBUG("rawdata test end!\n");

	return 1;

}

static s32 _check_other_options(int type)
{

	int ret = 0;

	if (type & (_VER_EQU_CHECK | _VER_GREATER_CHECK | _VER_BETWEEN_CHECK)) {

		ret = _check_device_version(type);

		if (ret < 0) {

			return ret;

		}

	}

	if (type & _MODULE_TYPE_CHECK) {

		ret = _check_modele_type(type);

		if (ret < 0) {

			return ret;

		}

		DEBUG("module test");

	}

	return ret;

}

s32 _load_dsp_code_check(void)
{

	u8 i, count, packages;

	u8 *ram;

	u16 start_addr, tmp_addr;

	s32 len, dsp_len = sizeof(dsp_short_9p);

	s32 ret = -1;

	ram = (u8 *) (&global_large_buf[0]);

	start_addr = 0xC000;

	len = PACKAGE_SIZE;

	tmp_addr = start_addr;

	count = 0;

	dsp_len = sizeof(dsp_short_9p);

	packages = dsp_len / PACKAGE_SIZE + 1;

	for (i = 0; i < packages; i++) {

		if (len > dsp_len) {

			len = dsp_len;

		}

		i2c_write_data(tmp_addr, (u8 *) & dsp_short_9p[tmp_addr - start_addr], len);

		i2c_read_data(tmp_addr, ram, len);

		ret = memcmp(&dsp_short_9p[tmp_addr - start_addr], ram, len);

		if (ret) {

			if (count++ > 5) {

				WARNING("equal error.\n");

				break;

			}

			continue;

		}

		tmp_addr += len;

		dsp_len -= len;

		if (dsp_len <= 0) {

			break;

		}

	}

	if (count < 5) {

		DEBUG("Burn DSP code successfully!\n");

		return 1;

	}

	return -1;

}

int _hold_ss51_dsp(void)
{

	int ret = -1;

	int retry = 0;

	unsigned char rd_buf[3];

	/* reset cpu */
	enter_update_mode();

	while (retry++ < 200) {

		/* Hold ss51 & dsp */
		rd_buf[0] = 0x0C;

		ret = i2c_write_data(_rRW_MISCTL__SWRST_B0_, rd_buf, 1);

		if (ret <= 0) {

			DEBUG("Hold ss51 & dsp I2C error,retry:%d", retry);

			continue;

		}
		/* Confirm hold */
		ret = i2c_read_data(_rRW_MISCTL__SWRST_B0_, rd_buf, 1);

		if (ret <= 0) {

			DEBUG("Hold ss51 & dsp I2C error,retry:%d", retry);

			continue;

		}

		if (0x0C == rd_buf[0]) {

			DEBUG("Hold ss51 & dsp confirm SUCCESS");

			break;

		}

		DEBUG("Hold ss51 & dsp confirm 0x4180 failed,value:%d", rd_buf[0]);

	}

	if (retry >= 200) {

		WARNING("Enter update Hold ss51 failed.");

		return -1;

	}

	return 1;

}

s32 _check_short_circuit(int test_types, u8 * short_result)
{

	u8 data[10], config[sys.config_length];

	s32 i, ret;

	if (!(test_types & _MODULE_SHORT_CHECK)) {

		DEBUG("Didn't need to check.[gt900],test type 0x%x", test_types);

		return _NEED_NOT_CHECK;

	}

	driver_status[0] = 0;

	read_config(config, sizeof(config));

	DEBUG("GT1x short test start.");

	if (disable_irq_esd() < 0) {

		WARNING("disable irq and esd fail.");

		return -1;

	}

	usleep(20 * 1000);

	/* select addr & hold ss51_dsp */
	ret = _hold_ss51_dsp();

	if (ret <= 0) {

		DEBUG("hold ss51 & dsp failed.");

		ret = ENTER_UPDATE_MODE_ERR;

		goto gt900_test_exit;

	}

	enter_update_mode_noreset();

	DEBUG("Loading..\n");

	if (_load_dsp_code_check() < 0) {

		ret = SHORT_TEST_ERROR;

		goto gt900_test_exit;

	}

	dsp_fw_startup(1);

	usleep(30 * 1000);

	for (i = 0; i < 100; i++) {

		i2c_read_data(_rRW_MISCTL__SHORT_BOOT_FLAG, data, 1);

		if (data[0] == 0xaa) {

			break;

		}

		DEBUG("buf[0]:0x%x", data[0]);

		usleep(10 * 1000);

	}

	if (i >= 20) {

		WARNING("Didn't get 0xaa at 0x%X\n", _rRW_MISCTL__SHORT_BOOT_FLAG);

		ret = SHORT_TEST_ERROR;

		goto gt900_test_exit;

	}

	data[0] = 0x00;

	i2c_write_data(_bRW_MISCTL__TMR0_EN, data, 1);

	write_test_params(module_cfg);

	/* clr 5095, runing dsp */
	data[0] = 0x00;

	i2c_write_data(_rRW_MISCTL__SHORT_BOOT_FLAG, data, 1);

	/* check whether the test is completed */
	i = 0;

	while (1) {

		i2c_read_data(0x8800, data, 1);

		if (data[0] == 0x88) {

			break;

		}

		usleep(50 * 1000);

		i++;

		if (i > 150) {

			WARNING("Didn't get 0x88 at 0x8800\n");

			ret = SHORT_TEST_ERROR;

			goto gt900_test_exit;

		}

	}

	i = ((_DRV_TOTAL_NUM + _SEN_TOTAL_NUM) * 2 + 20) / 4;
	i += (_DRV_TOTAL_NUM + _SEN_TOTAL_NUM) * 2 / 4;

	i += (_GT9_UPLOAD_SHORT_TOTAL + 1) * 4;

	if (i > 1343) {

		return -1;

	}

	driver_status[0] = 0;

	/* DEBUG("AVDD:%0.2f",sys_avdd);
	   DEBUG("gt900_short_threshold:%d",gt900_short_threshold);
	   DEBUG("gt900_resistor_warn_threshold:%d",gt900_resistor_warn_threshold);
	   DEBUG("gt900_drv_drv_resistor_threshold:%d",gt900_drv_drv_resistor_threshold);
	   DEBUG("gt900_drv_sen_resistor_threshold:%d",gt900_drv_sen_resistor_threshold);
	   DEBUG("gt900_sen_sen_resistor_threshold:%d",gt900_sen_sen_resistor_threshold);
	   DEBUG("gt900_drv_gnd_resistor_threshold:%d",gt900_drv_gnd_resistor_threshold);
	   DEBUG("gt900_sen_gnd_resistor_threshold:%d",gt900_sen_gnd_resistor_threshold); */

	ret = short_test_analysis(short_result, _GT9_UPLOAD_SHORT_TOTAL);

	if (ret > 0) {

		test_to_show(&short_result[1], short_result[0]);

		test_error_code |= ret;

		ret = short_result[0];

	}

gt900_test_exit:
	reset_guitar();

	usleep(2 * 1000);

	enable_irq_esd();

	memset(data, 0x00, 8);

	i2c_write_data(0x8800, data, 8);

	DEBUG("result upload_cnt:%d", short_result[0]);

	DEBUG("short text end,ret 0x%x", ret);

	return ret;

}

s32 _alloc_test_memory(int b_size)
{

	int offest = 0;

	global_large_buf = (u8 *) malloc(sizeof(u8) * 60 * b_size);

	if (global_large_buf == NULL) {

		return MEMORY_ERR;

	}

	offest = 2 * 1024;

	need_check = &global_large_buf[offest];

	offest += b_size * sizeof(char);

	channel_key_need_check = &global_large_buf[offest];

	offest += b_size * sizeof(char);

	channel_status = (u16 *) (&global_large_buf[offest]);

	offest += b_size * sizeof(short);

	channel_max_value = (u16 *) (&global_large_buf[offest]);

	offest += b_size * sizeof(short);

	channel_min_value = (u16 *) (&global_large_buf[offest]);

	offest += b_size * sizeof(short);

	channel_max_accord = (long *)(&global_large_buf[offest]);

	offest += b_size * sizeof(long);

	channel_average = (int *)(&global_large_buf[offest]);

	offest += b_size * sizeof(int);

	channel_square_average = (int *)(&global_large_buf[offest]);

	offest += b_size * sizeof(int);

	driver_status = &global_large_buf[offest];

	offest += b_size * sizeof(char);

	beyond_max_limit_num = &global_large_buf[offest];

	offest += b_size * sizeof(char);

	beyond_min_limit_num = &global_large_buf[offest];

	offest += b_size * sizeof(char);

	beyond_accord_limit_num = &global_large_buf[offest];

	offest += b_size * sizeof(char);

	beyond_offset_limit_num = &global_large_buf[offest];

	offest += b_size * sizeof(char);

	beyond_jitter_limit_num = &global_large_buf[offest];

	offest += b_size * sizeof(char);

	max_limit_value = (s32 *) (&global_large_buf[offest]);

	offest += b_size * sizeof(int);

	min_limit_value = (s32 *) (&global_large_buf[offest]);

	offest += b_size * sizeof(int);

	accord_limit = (long *)(&global_large_buf[offest]);

	offest += b_size * sizeof(long);

	ini_version1 = &global_large_buf[offest];

	offest += 50;

	ini_version2 = &global_large_buf[offest];

	offest += 50;

	original_cfg = &global_large_buf[offest];

	offest += 350;

	module_cfg = &global_large_buf[offest];

	offest += 350;

	max_key_limit_value = (s32 *) (&global_large_buf[offest]);

	offest += MAX_KEY_RAWDATA * sizeof(int);

	min_key_limit_value = (s32 *) (&global_large_buf[offest]);

	offest += MAX_KEY_RAWDATA * sizeof(int);

	current_data_temp = (u16 *) (&global_large_buf[offest]);

	offest += b_size * sizeof(short);

	return 1;

}

void _exit_test(void)
{

	disable_hopping(original_cfg, sys.config_length, 0);

	if (global_large_buf != NULL) {

		free(global_large_buf);

		global_large_buf = NULL;

	}

}

#if GTP_TEST_PARAMS_FROM_INI
static s32 _get_test_parameters(char *inipath)
{

	int test_types;

	if (inipath == NULL) {

		WARNING("ini file path is null.");

		return PARAMETERS_ILLEGL;

	}

	test_types = ini_read_hex(inipath, (const s8 *)"test_types");

	if (test_types < 0) {

		WARNING("get the test types 0x%x is error.", test_types);

		return test_types;

	}

	DEBUG("test type:%x\n", test_types);

	if (test_types & _MAX_CHECK) {

		max_limit_value_tmp = ini_read_int(inipath, (const s8 *)"max_limit_value");

		DEBUG("max_limit_value:%d\n", max_limit_value_tmp);

	}

	if (test_types & _MIN_CHECK) {

		min_limit_value_tmp = ini_read_int(inipath, (const s8 *)"min_limit_value");

		DEBUG("min_limit_value:%d\n", min_limit_value_tmp);

	}

	if (test_types & _ACCORD_CHECK) {

		accord_limit_tmp = ini_read_float(inipath, (const s8 *)"accord_limit");

		DEBUG("accord_limit:%ld", accord_limit_tmp);

	}

	if (test_types & _OFFSET_CHECK) {

		offset_limit = ini_read_float(inipath, (const s8 *)"offset_limit");

		DEBUG("offset_limit:%ld", offset_limit);

	}

	if (test_types & _JITTER_CHECK) {

		permit_jitter_limit = ini_read_int(inipath, (const s8 *)"permit_jitter_limit");

		DEBUG("permit_jitter_limit:%d\n", permit_jitter_limit);

	}

	if (test_types & _SPECIAL_CHECK) {

		special_limit = ini_read_float(inipath, (const s8 *)"special_limit");

		DEBUG("special_limit:%ld", special_limit);

	}

	if (test_types & _KEY_MAX_CHECK) {

		max_key_limit_value_tmp = ini_read_int(inipath, (const s8 *)"max_key_limit_value");

		DEBUG("max_key_limit_value:%d\n", max_key_limit_value_tmp);

	}

	if (test_types & _KEY_MIN_CHECK) {

		min_key_limit_value_tmp = ini_read_int(inipath, (const s8 *)"min_key_limit_value");

		DEBUG("min_key_limit_value:%d\n", min_key_limit_value_tmp);

	}

	if (test_types & _MODULE_TYPE_CHECK) {

		ini_module_type = ini_read_int(inipath, (const s8 *)"module_type");

		DEBUG("Sensor ID:%d\n", ini_module_type);

	}

	if (test_types & _VER_EQU_CHECK) {

		ini_read(inipath, (const s8 *)"version_equ", (s8 *) ini_version1);

		DEBUG("version_equ:%s", ini_version1);

	}

	else if (test_types & _VER_GREATER_CHECK) {

		ini_read(inipath, (const s8 *)"version_greater", (s8 *) ini_version1);

		DEBUG("version_greater:%s", ini_version1);

	}

	else if (test_types & _VER_BETWEEN_CHECK) {

		ini_read(inipath, (const s8 *)"version_between1", (s8 *) ini_version1);

		DEBUG("version_between1:%s", ini_version1);

		ini_read(inipath, (const s8 *)"version_between2", (s8 *) ini_version2);

		DEBUG("version_between2:%s", ini_version2);

	}

	if (test_types & _MODULE_SHORT_CHECK) {

		long lret;

		int ret = ini_read_int(inipath, (const s8 *)"gt900_short_threshold");

		if (ret > 0) {

			gt900_short_threshold = ret;

			DEBUG("gt900_short_threshold:%d", gt900_short_threshold);

		}

		ret = ini_read_int(inipath, (const s8 *)"gt900_drv_drv_resistor_threshold");

		if (ret > 0) {

			gt900_drv_drv_resistor_threshold = ret;

			DEBUG("gt900_drv_drv_resistor_threshold:%d", gt900_drv_drv_resistor_threshold);

		}

		ret = ini_read_int(inipath, (const s8 *)"gt900_drv_sen_resistor_threshold");

		if (ret > 0) {

			gt900_drv_sen_resistor_threshold = ret;

			DEBUG("gt900_drv_sen_resistor_threshold:%d", gt900_drv_sen_resistor_threshold);

		}

		ret = ini_read_int(inipath, (const s8 *)"gt900_sen_sen_resistor_threshold");

		if (ret > 0) {

			gt900_sen_sen_resistor_threshold = ret;

			DEBUG("gt900_sen_sen_resistor_threshold:%d", gt900_sen_sen_resistor_threshold);

		}

		ret = ini_read_int(inipath, (const s8 *)"gt900_resistor_warn_threshold");

		if (ret > 0) {

			gt900_resistor_warn_threshold = ret;

			DEBUG("gt900_resistor_warn_threshold:%d", gt900_resistor_warn_threshold);

		}

		ret = ini_read_int(inipath, (const s8 *)"gt900_drv_gnd_resistor_threshold");

		if (ret > 0) {

			gt900_drv_gnd_resistor_threshold = ret;

			DEBUG("gt900_drv_gnd_resistor_threshold:%d", gt900_drv_gnd_resistor_threshold);

		}

		ret = ini_read_int(inipath, (const s8 *)"gt900_sen_gnd_resistor_threshold");

		if (ret > 0) {

			gt900_sen_gnd_resistor_threshold = ret;

			DEBUG("gt900_sen_gnd_resistor_threshold:%d", gt900_sen_gnd_resistor_threshold);

		}

		ret = ini_read_int(inipath, (const s8 *)"gt900_short_test_times");

		if (ret > 0) {

			gt900_short_test_times = ret;

			DEBUG("gt900_short_test_times:%d", gt900_short_test_times);

		}

		lret = ini_read_float(inipath, (const s8 *)"tri_pattern_r_ratio");

		if (ret > 0) {

			tri_pattern_r_ratio = lret;

			DEBUG("tri_pattern_r_ratio:%ld", tri_pattern_r_ratio);

		}

		ret = ini_read_float(inipath, (const s8 *)"AVDD");

		if (ret > 0) {

			sys_avdd = ret * 10 / FLOAT_AMPLIFIER;

			DEBUG("sys_avdd:%d", sys_avdd);

		}

	}

	uniformity_limit = ini_read_float(inipath, (const s8 *)"rawdata_uniformity");

	if (uniformity_limit > 0) {

		test_types |= _UNIFORMITY_CHECK;

		DEBUG("uniformity_limit:%ld", uniformity_limit);

	}

	DEBUG("_get_test_parameters success!");

	return test_types;

}
#else
static s32 _get_test_parameters_array(void)
{

	int test_types;

	test_types = TEST_TYPES;

	if (test_types < 0) {

		WARNING("get the test types 0x%x is error.", test_types);

		return test_types;

	}

	DEBUG("test type:%x\n", test_types);

	if (test_types & _MAX_CHECK) {

		max_limit_value_tmp = MAX_LIMIT_VALUE;

		DEBUG("max_limit_value:%d\n", max_limit_value_tmp);

	}

	if (test_types & _MIN_CHECK) {

		min_limit_value_tmp = MIN_LIMIT_VALUE;

		DEBUG("min_limit_value:%d\n", min_limit_value_tmp);

	}

	if (test_types & _ACCORD_CHECK) {

		accord_limit_tmp = ACCORD_LIMIT;

		DEBUG("accord_limit:%ld", accord_limit_tmp);

	}

	if (test_types & _OFFSET_CHECK) {

		offset_limit = OFFEST_LIMIT;

		DEBUG("offset_limit:%ld", offset_limit);

	}

	if (test_types & _JITTER_CHECK) {

		permit_jitter_limit = PERMIT_JITTER_LIMIT;

		DEBUG("permit_jitter_limit:%d\n", permit_jitter_limit);

	}

	if (test_types & _SPECIAL_CHECK) {

		special_limit = SPECIAL_LIMIT;

		DEBUG("special_limit:%ld", special_limit);

	}

	if (test_types & _KEY_MAX_CHECK) {

		max_key_limit_value_tmp = MAX_KEY_LIMIT_VALUE;

		DEBUG("max_key_limit_value:%d\n", max_key_limit_value_tmp);

	}

	if (test_types & _KEY_MIN_CHECK) {

		min_key_limit_value_tmp = MIN_KEY_LIMIT_VALUE;

		DEBUG("min_key_limit_value:%d\n", min_key_limit_value_tmp);

	}

	if (test_types & _MODULE_TYPE_CHECK) {

		ini_module_type = MODULE_TYPE;

		DEBUG("Sensor ID:%d\n", ini_module_type);

	}

	if (test_types & _VER_EQU_CHECK) {
		memcpy((char *)ini_version1, (const char *)VERSION_EQU, sizeof(VERSION_EQU));

		DEBUG("version_equ:%s", ini_version1);

	}

	else if (test_types & _VER_GREATER_CHECK) {

		memcpy(ini_version1, VERSION_GREATER, sizeof(VERSION_GREATER));
		DEBUG("version_greater:%s", ini_version1);

	}

	else if (test_types & _VER_BETWEEN_CHECK) {

		memcpy(ini_version1, VERSION_BETWEEN1, sizeof(VERSION_BETWEEN1));
		DEBUG("version_between1:%s", ini_version1);

		memcpy(ini_version2, VERSION_BETWEEN2, sizeof(VERSION_BETWEEN2));
		DEBUG("version_between2:%s", ini_version2);

	}

	if (test_types & _MODULE_SHORT_CHECK) {

		gt900_short_threshold = GT900_SHORT_THRESHOLD;

		DEBUG("gt900_short_threshold:%d", gt900_short_threshold);

		gt900_drv_drv_resistor_threshold = GT900_DRV_DRV_RESISTOR_THRESHOLD;

		DEBUG("gt900_drv_drv_resistor_threshold:%d", gt900_drv_drv_resistor_threshold);

		gt900_drv_sen_resistor_threshold = GT900_DRV_SEN_RESISTOR_THRESHOLD;

		DEBUG("gt900_drv_sen_resistor_threshold:%d", gt900_drv_sen_resistor_threshold);

		gt900_sen_sen_resistor_threshold = GT900_SEN_SEN_RESISTOR_THRESHOLD;

		DEBUG("gt900_sen_sen_resistor_threshold:%d", gt900_sen_sen_resistor_threshold);

		gt900_resistor_warn_threshold = GT900_RESISTOR_WARN_THRESHOLD;

		DEBUG("gt900_resistor_warn_threshold:%d", gt900_resistor_warn_threshold);

		gt900_drv_gnd_resistor_threshold = GT900_DRV_GND_RESISTOR_THRESHOLD;

		DEBUG("gt900_drv_gnd_resistor_threshold:%d", gt900_drv_gnd_resistor_threshold);

		gt900_sen_gnd_resistor_threshold = GT900_SEN_GND_RESISTOR_THRESHOLD;

		DEBUG("gt900_sen_gnd_resistor_threshold:%d", gt900_sen_gnd_resistor_threshold);

		gt900_short_test_times = GT900_SHORT_TEST_TIMES;

		DEBUG("gt900_short_test_times:%d", gt900_short_test_times);

		tri_pattern_r_ratio = TRI_PATTERN_R_RATIO;

		DEBUG("tri_pattern_r_ratio:%ld", tri_pattern_r_ratio);

		sys_avdd = AVDD * 10 / FLOAT_AMPLIFIER;

		DEBUG("sys_avdd:%d", sys_avdd);

	}

	uniformity_limit = RAWDATA_UNIFORMITY;

	if (uniformity_limit > 0) {

		test_types |= _UNIFORMITY_CHECK;

		DEBUG("uniformity_limit:%ld", uniformity_limit);

	}

	DEBUG("_get_test_parameters success!");

	return test_types;

}
#endif

s32 _init_test_paramters(char *inipath)
{

	int b_size = _DRV_TOTAL_NUM * _SEN_TOTAL_NUM + 10;

	int i;

	int check_types;

	int ret = -1;

	u8 *s_nc_buf;

	u8 *key_nc_buf;

#if !GTP_TEST_PARAMS_FROM_INI
	const u8 nc_tmp[] = NC;
	const u8 key_nc_tmp[] = KEY_NC;
	const u8 module_cfg_tmp[] = MODULE_CFG;
#endif

	DEBUG("%s", __func__);

	if (_alloc_test_memory(b_size) < 0) {

		return MEMORY_ERR;

	}

	DEBUG("begin _get_test_parameters");
#if GTP_TEST_PARAMS_FROM_INI
	check_types = _get_test_parameters(inipath);
#else
	check_types = _get_test_parameters_array();
#endif
	if (check_types < 0) {

		return check_types;

	}

	current_data_index = 0;

	test_error_code = _CHANNEL_PASS;

	if ((check_types & _FAST_TEST_MODE) != 0) {

		samping_set_number = (samping_num / 4) + (samping_num % 4);

	}

	else {

		samping_set_number = samping_num;

	}

	beyond_uniformity_limit_num = 0;

	for (i = 0; i < b_size; i++) {

		channel_status[i] = _CHANNEL_PASS;

		channel_max_value[i] = 0;

		channel_min_value[i] = 0xFFFF;

		channel_max_accord[i] = 0;

		channel_average[i] = 0;

		channel_square_average[i] = 0;

		beyond_max_limit_num[i] = 0;

		beyond_min_limit_num[i] = 0;

		beyond_accord_limit_num[i] = 0;

		beyond_offset_limit_num[i] = 0;

		max_limit_value[i] = max_limit_value_tmp;

		min_limit_value[i] = min_limit_value_tmp;

		accord_limit[i] = accord_limit_tmp;

	}

	for (i = 0; i < MAX_KEY_RAWDATA; i++) {

		max_key_limit_value[i] = max_key_limit_value_tmp;

		min_key_limit_value[i] = min_key_limit_value_tmp;

	}

	read_config(original_cfg, sys.config_length);

#if GTP_TEST_PARAMS_FROM_INI
	ret = ini_read_text(inipath, (const s8 *)"module_cfg", module_cfg);
#else
	memcpy(module_cfg, module_cfg_tmp, sizeof(module_cfg_tmp));
	ret = sizeof(module_cfg_tmp);
#endif

	DEBUG("module_cfg len %d ,config len %d", ret, sys.config_length);

	if (ret < 0 || ret != sys.config_length) {

		DEBUG("switch the original cfg.");

		memcpy(module_cfg, original_cfg, sys.config_length);

		disable_hopping(module_cfg, sys.config_length, 1);

	} else {

		DEBUG("switch the module cfg.");

		disable_hopping(module_cfg, sys.config_length, 0);

	}
	s_nc_buf = (unsigned char *)(&global_large_buf[0]);
	key_nc_buf = (unsigned char *)(&global_large_buf[2 * (b_size / 8) + 10]);

	memset(s_nc_buf, 0, 2 * (b_size / 8) + 10);

	memset(key_nc_buf, 0, _SEN_TOTAL_NUM / 8 + 10);

	DEBUG("here\n");

#if GTP_TEST_PARAMS_FROM_INI
	ini_read_text(inipath, (const s8 *)"NC", s_nc_buf);
	ret = ini_read_text(inipath, (const s8 *)"KEY_NC", key_nc_buf);
#else
	memcpy(s_nc_buf, nc_tmp, sizeof(nc_tmp));
	memcpy(key_nc_buf, key_nc_tmp, sizeof(key_nc_tmp));
	ret = sizeof(key_nc_tmp);
#endif

	DEBUG("WITH KEY?:0x%x\n", ret);

	if (ret <= 0) {
		_unzip_nc(s_nc_buf, key_nc_buf, b_size / 8 + 8, 0);
	} else {
		_unzip_nc(s_nc_buf, key_nc_buf, b_size / 8 + 8, _SEN_TOTAL_NUM);
	}

	DEBUG("need check array:\n");

	DEBUG_ARRAY(need_check, sys.sc_driver_num * sys.sc_sensor_num);

	DEBUG("key need check array:\n");

	/* DEBUG_ARRAY(channel_key_need_check, b_size); */

	DEBUG("key_nc_buf:\n");

	/* DEBUG_ARRAY(key_nc_buf, strlen((const char*)key_nc_buf)); */
#if GTP_TEST_PARAMS_FROM_INI
	ret = _init_special_node(inipath);
	if (ret < 0) {

		WARNING("set special node fail, ret 0x%x", ret);

	}
#else
	_init_special_node_array();
#endif

	return check_types;

}

s32 _open_short_test(unsigned char *short_result_data)
{

	int ret = -1, test_types;

	char *ini_path, times = 0, timeouts = 0;

	u16 *rawdata;

	u8 *short_result;

	char *save_path;

	u8 *largebuf;

	largebuf = (unsigned char *)malloc(2 * 1024);

	ini_path = (char *)(&largebuf[0]);

	short_result = (u8 *) (&largebuf[250]);

	save_path = (char *)(&largebuf[350]);

	rawdata = (u16 *) (&largebuf[600]);

	sys.reg_rawdata_base = 0xB798;

	sys.key_offest = 83;

	sys.config_length = 239;

	read_config(largebuf, sys.config_length);

	parse_config(largebuf);

	DEBUG("sen*drv:%d*%d", sys.sensor_num, sys.driver_num);

#if GTP_TEST_PARAMS_FROM_INI
	if (auto_find_ini(ini_find_dir1, ini_format, ini_path) < 0) {

		if (auto_find_ini(ini_find_dir2, ini_format, ini_path) < 0) {

			WARNING("Not find the ini file.");

			free(largebuf);

			return INI_FILE_ILLEGAL;

		}

	}

	DEBUG("find ini path:%s", ini_path);
#endif

	test_types = _init_test_paramters(ini_path);

	if (test_types < 0) {

		WARNING("get test params failed.");

		free(largebuf);

		return test_types;

	}

	FORMAT_PATH(save_path, save_result_dir);

	DEBUG("save path is %s", save_path);
	if (test_types & (_MAX_CHECK | _MIN_CHECK | _ACCORD_CHECK | _OFFSET_CHECK | _JITTER_CHECK | _KEY_MAX_CHECK | _KEY_MIN_CHECK | _UNIFORMITY_CHECK)) {

		while (times < 64) {

			ret = read_raw_data(rawdata, sys.sensor_num * sys.driver_num);

			if (ret < 0) {

				DEBUG("read rawdata timeout %d.", timeouts);

				if (++timeouts > 5) {

					WARNING("read rawdata timeout.");

					break;

				}

				be_normal();

				continue;

			}

			ret = _check_rawdata_proc(test_types, rawdata, sys.sensor_num * sys.driver_num, save_path);

			if (ret < 0) {

				WARNING("raw data test proc error.");

				break;

			}

			else if (ret == 1) {

				DEBUG("rawdata check finish.");

				break;

			}

			times++;

		}

		be_normal();

		if (ret < 0) {

			WARNING("rawdata check fail.");

			goto TEST_END;

		}
	}

	ret = _check_other_options(test_types);

	if (ret < 0) {

		WARNING("DeviceVersion or ModuleType test failed.");

		goto TEST_END;

	}

	memset(short_result, 0, 100);

	ret = _check_short_circuit(test_types, short_result);

	if (ret < 0) {

		WARNING("Short Test Fail.");

		goto TEST_END;

	}

	ret = test_error_code;

#if GTP_SAVE_TEST_DATA
//  if ((check_types & _TEST_RESULT_SAVE) != 0)
	{

		_save_test_result_data(save_path, test_types, short_result);

	}

#endif
	DEBUG("cnt %d", short_result[0]);

	if (short_result_data != NULL) {

		memcpy(short_result_data, short_result, short_result[0] * 4 + 1);

	}
TEST_END:
	_exit_test();

	DEBUG("test result 0x%X", ret);

	free(largebuf);

	return ret;

}

#ifdef __cplusplus
//}
#endif
