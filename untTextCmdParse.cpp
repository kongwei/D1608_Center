//---------------------------------------------------------------------------


#pragma hdrstop
#include <vcl.h>

#include "untTextCmdParse.h"

#include <mem.h>
#include <string.h>
extern "C"{
#include "../enc28j60_iap_app/inc/D1608Pack.h"
}
//---------------------------------------------------------------------------
static int TextToInt(char * str, int base)
{
    double ret = atof(str) * base;

    // double 可能会超过int边界
    if (ret > INT_MAX)
    {
        return INT_MAX;
    }
    else if (ret < INT_MIN)
    {
        return INT_MIN;
    }
    else
    {
        return ret;
    }
}
static short TextToShort(char * str, int base)
{
    double ret = atof(str) * base;

    // double 可能会超过int边界
    if (ret > SHRT_MAX)
    {
        return SHRT_MAX;
    }
    else if (ret < SHRT_MIN)
    {
        return SHRT_MIN;
    }
    else
    {
        return ret;
    }
}
//---------------------------------------------------------------------------

#pragma package(smart_init)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum Opr {o_set, o_add, o_minus};
typedef struct
{
    char var_name[20];
    int range_count;
    int range[16];
}Var;
typedef struct
{
    char value[80];
    enum Opr opr;
    int var_count;
    Var vars[10];
}CommandContent;

typedef struct
{
    int count;
    char * sub_strings[10];
}SubStrings;

static SubStrings DivString(char * string, char div_char)
{
    SubStrings result = {0};
    char * p = string;
    char * p_substring = string;
    while (*p != '\0')
    {
        if (*p == div_char)
        {
            result.sub_strings[result.count] = p_substring;
            result.count++;
            *p = '\0';
            p_substring = p+1;
        }

        p++;
    }
    result.sub_strings[result.count] = p_substring;
    result.count++;

    return result;
}

int parse(const char * command, CommandContent * result)
{
    char * tmp_p;
    SubStrings vars;
    int var_i;
	char command_tmp[120] = "";
    int command_length = strlen(command);
	
	memcpy(command_tmp, command, strlen(command));

	// 必须以 | 结尾，并且删除这个结尾
	if (command_tmp[command_length-1] != ']')
	{
		return 0;
	}
	command_tmp[command_length-1] = '\0';

    // 不能有空格
    //if (strstr(command, " ") != NULL)
    //{
    //    return 0;
    //}

    // 解析 Opr 和 Value
    if ((tmp_p=strstr(command_tmp, "+=")) != NULL)
    {
        result->opr = o_add;
        strncpy(result->value, tmp_p+2, sizeof(result->value));
        *tmp_p = '\0';
    }
    else if ((tmp_p=strstr(command_tmp, "-=")) != NULL)
    {
        result->opr = o_minus;
        strncpy(result->value, tmp_p+2, sizeof(result->value));
        *tmp_p = '\0';
    }
    else if ((tmp_p=strstr(command_tmp, "=")) != NULL)
    {
        result->opr = o_set;
        strncpy(result->value, tmp_p+1, sizeof(result->value));
        *tmp_p = '\0';
    }
    else
        return 0;

    // 解析 变量, 按照 '.' 分割
    vars = DivString(command_tmp, '.');
    for (var_i=0; var_i<vars.count; var_i++)
    {
        char * var = vars.sub_strings[var_i];
        if ((tmp_p=strstr(var, "<")) != NULL)
        {
            SubStrings range_list;
            int list_i;
            
            // 最后一个也必须是 ">"
            int variable_length = strlen(var);
            if (var[variable_length-1] != '>')
                return 0;

            *tmp_p = '\0';
            var[variable_length-1] = '\0';

            // 解析 range, 先按照 ',' 分割, 再按照 '-' 范围
            range_list = DivString(tmp_p+1, ',');
            for (list_i=0; list_i<range_list.count; list_i++)
            {
                Var * result_var = &result->vars[result->var_count];
                
                char * list = range_list.sub_strings[list_i];
                SubStrings ranges = DivString(list, '-');

                if (ranges.count > 2)
                    return 0;

                if (ranges.count == 1)
                {                                                      
                    result_var->range[result_var->range_count] = atoi(ranges.sub_strings[0]);
                    result_var->range_count++;
                }
                if (ranges.count == 2)
                {
                    int i;
                    int begin = atoi(ranges.sub_strings[0]);
                    int end = atoi(ranges.sub_strings[1]);
                    if (begin < 1 || end > 16)
                        return 0;

                    for (i=begin; i<=end; i++)
                    {
                        result_var->range[result_var->range_count] = i;
                        result_var->range_count++;
                    }
                }
            }
        }

        strcpy(result->vars[result->var_count].var_name, var);
        result->var_count++;
    }

    return 1;
}

int StringToBool(char * str)
{
	if (!strcmp(str, "on"))
		return 1;
	if (!strcmp(str, "off"))
		return 0;
	return -1;
}
int IsValidBoolString(char * str)
{
	return StringToBool(str) >= 0;
}

//-------------------------------
#define EQ_TYPE_Peaking                      1
#define EQ_TYPE_BandPass                     2
#define EQ_TYPE_HighShelving                 3   
#define EQ_TYPE_LowShelving                  4   
#define EQ_TYPE_Notch                        5   
#define EQ_TYPE_HighBessel_12dB              1202
#define EQ_TYPE_LowBessel_12dB               1212
#define EQ_TYPE_HighBessel_18dB              1802
#define EQ_TYPE_LowBessel_18dB               1812
#define EQ_TYPE_HighBessel_24dB              2402
#define EQ_TYPE_LowBessel_24dB               2412
//#define EQ_TYPE_HighPassButterworth_48dB     4802
//#define EQ_TYPE_LowPassButterworth_48dB      4812
#define EQ_TYPE_HighLinkwitz_12dB            1203
#define EQ_TYPE_LowLinkwitz_12dB             1213
#define EQ_TYPE_HighLinkwitz_24dB            2403
#define EQ_TYPE_LowLinkwitz_24dB             2413
#define EQ_TYPE_HighLinkwitz_48dB            4803
#define EQ_TYPE_LowLinkwitz_48dB             4813
#define EQ_TYPE_HighBansen                   604 
#define EQ_TYPE_LowBansen                    614 
#define EQ_TYPE_HighPassButterworth_12dB     1201
#define EQ_TYPE_HighPassButterworth_18dB     1801
#define EQ_TYPE_HighPassButterworth_24dB     2401
#define EQ_TYPE_HighPassButterworth_48dB     4801
#define EQ_TYPE_LowPassButterworth_12dB      1211
#define EQ_TYPE_LowPassButterworth_18dB      1811
#define EQ_TYPE_LowPassButterworth_24dB      2411
#define EQ_TYPE_LowPassButterworth_48dB      4811
#define EQ_TYPE_Pink                         7   
typedef struct
{
	short type_id;
	char type_name[30];
}TypeNameMap;
const TypeNameMap type_map[30] =
{
    {EQ_TYPE_Peaking                      ,    "PARAMETRIC"},              
    {EQ_TYPE_BandPass                     ,    "BAND_PASS"},               
    {EQ_TYPE_HighShelving                 ,    "HIGH_SHELF"},              
    {EQ_TYPE_LowShelving                  ,    "LOW_SHELF"},               
    {EQ_TYPE_Notch                        ,    "NOTCH"},                   
    {EQ_TYPE_HighBessel_12dB              ,    "12dB_BESSEL_HIGH"},        
    {EQ_TYPE_LowBessel_12dB               ,    "12dB_BESSEL_LOW"},         
    {EQ_TYPE_HighBessel_18dB              ,    "18dB_BESSEL_HIGH"},        
    {EQ_TYPE_LowBessel_18dB               ,    "18dB_BESSEL_LOW"},         
    {EQ_TYPE_HighBessel_24dB              ,    "24dB_BESSEL_HIGH"},        
    {EQ_TYPE_LowBessel_24dB               ,    "24dB_BESSEL_LOW"},         
    {EQ_TYPE_HighPassButterworth_48dB     ,    "48dB_BESSEL_HIGH"},        
    {EQ_TYPE_LowPassButterworth_48dB      ,    "48dB_BESSEL_LOW"},         
    {EQ_TYPE_HighLinkwitz_12dB            ,    "12dB_LINKWITZ-RILEY_HIGH"},
    {EQ_TYPE_LowLinkwitz_12dB             ,    "12dB_LINKWITZ-RILEY_LOW"}, 
    {EQ_TYPE_HighLinkwitz_24dB            ,    "24dB_LINKWITZ-RILEY_HIGH"},
    {EQ_TYPE_LowLinkwitz_24dB             ,    "24dB_LINKWITZ-RILEY_LOW"}, 
    {EQ_TYPE_HighLinkwitz_48dB            ,    "48dB_LINKWITZ-RILEY_HIGH"},
    {EQ_TYPE_LowLinkwitz_48dB             ,    "48dB_LINKWITZ-RILEY_LOW"}, 
    {EQ_TYPE_HighBansen                   ,    "6dB_BANSEN_HIGH"},         
    {EQ_TYPE_LowBansen                    ,    "6dB_BANSEN_LOW"},          
    {EQ_TYPE_HighPassButterworth_12dB     ,    "12dB_BUTTERWORTH_HIGH"},   
    {EQ_TYPE_HighPassButterworth_18dB     ,    "18dB_BUTTERWORTH_HIGH"},   
    {EQ_TYPE_HighPassButterworth_24dB     ,    "24dB_BUTTERWORTH_HIGH"},   
    {EQ_TYPE_HighPassButterworth_48dB     ,    "48dB_BUTTERWORTH_HIGH"},   
    {EQ_TYPE_LowPassButterworth_12dB      ,    "12dB_BUTTERWORTH_LOW"},    
    {EQ_TYPE_LowPassButterworth_18dB      ,    "18dB_BUTTERWORTH_LOW"},    
    {EQ_TYPE_LowPassButterworth_24dB      ,    "24dB_BUTTERWORTH_LOW"},    
    {EQ_TYPE_LowPassButterworth_48dB      ,    "48dB_BUTTERWORTH_LOW"},    
    {EQ_TYPE_Pink                         ,    "PINK"},           
};                                                                         
static short StringToTYPE(char * str)
{
	int i;
	for (i=0;i<30;i++)
	{
		if (!strcmp(str, type_map[i].type_name))
		{
			return type_map[i].type_id;
		}
	}
	return 0;
}
static int IsValidInteger(char * str, int base)
{
	char * end_ptr;
	strtol(str, &end_ptr, base);
	if (*end_ptr == '\0')
		return 1;

	return 0;
}
static int IsValidVolume(char * str)
{
	if (!strcmp(str, "Off"))
	{
		return 1;
	}
	else
	{
		char * end_ptr;
		strtod(str, &end_ptr);
		if (!strcmp(end_ptr, "dB"))
			return 1;
	}
	return 0;
}
static short StringToVolume(char * str, short min)
{
	if (!strcmp(str, "Off"))
	{
		return min;
	}
	else
	{
		return TextToShort(str, 10);;
	}
}
static int IsValidFreq(char * str, short min, short max)
{
	
	char * end_ptr;
	strtod(str, &end_ptr);
	if (!strcmp(end_ptr, "Hz"))
	{
		float freq = atof(str);
		if (freq >= min && freq <= max)
		{
			return 1;
		}
	}

	return 0;
}
static unsigned int StringToFreq(char * str)
{
	return TextToInt(str, 10);
}
static int IsValidTimeMs(char * str)
{
	char * end_ptr;
	strtod(str, &end_ptr);
	if (!strcmp(end_ptr, "ms"))
		return 1;
	return 0;
}
static int StringToTimeMs(char * str, int scale)
{
	return TextToInt(str, scale);
}
static int IsValidRatio(char * str)
{
	if (!strcmp(str, "∞"))
		return 1;
	else
	{
		char * end_ptr;
		strtod(str, &end_ptr);
		if (*end_ptr == '\0')
			return 1;
	}
	return 0;
}
static int StringToRatio(char * str)
{
	if (!strcmp(str, "∞"))
	{
		return 0;
	}
	else
	{
		double ratio_tmp = atof(str);
		if (ratio_tmp < 1)
			ratio_tmp = 1;

        ratio_tmp = 100.0 / ratio_tmp;
		return ratio_tmp;
	}
}
static int StringToMenuFunction(char * str)
{
	if (!strcmp(str, "next_preset"))
		return 1;
	if (!strcmp(str, "volume_up"))
		return 2;
	if (!strcmp(str, "volume_down"))
		return 3;
	
	return -1;
}

// 0 表示DHCP, -1表示失败
static UINT StringToIp(char * str)
{
	if (!strcmp(str, "auto"))
	{
		return 0;
	}
	else
	{
		SubStrings ip_pics = DivString(str, '.');
		if (ip_pics.count == 4)
		{
			if (IsValidInteger(ip_pics.sub_strings[0], 10)
				&& IsValidInteger(ip_pics.sub_strings[1], 10)
				&& IsValidInteger(ip_pics.sub_strings[2], 10)
				&& IsValidInteger(ip_pics.sub_strings[3], 10))
			{
				unsigned int ip1 = atoi(ip_pics.sub_strings[0]);
				unsigned int ip2 = atoi(ip_pics.sub_strings[1]);
				unsigned int ip3 = atoi(ip_pics.sub_strings[2]);
				unsigned int ip4 = atoi(ip_pics.sub_strings[3]);
				if (ip1<=255 && ip2<=255 && ip3<=255 && ip4<=255)
				{
					int result = (ip4<<24) | (ip3<<16) | (ip2<<8) | ip1;
					return result;
				}
			}
		}
	}
	
	return -1;
}
// 1表示成功转换, 0表示失败
static int StringToMac(char * str, char* mac_addr)
{
	SubStrings ip_pics = DivString(str, ':');
	if (ip_pics.count == 6)
	{
		int i;
		for (i=0;i<6;i++)
		{
			char mac_no_dev[3] = {0};
			// 长度必须是2
			if (strlen(ip_pics.sub_strings[i]) != 2)
				return 0;
			
			mac_no_dev[0] = ip_pics.sub_strings[i][0];
			mac_no_dev[1] = ip_pics.sub_strings[i][1];
			if (!IsValidInteger(mac_no_dev, 16))
				return 0;

			mac_addr[i] = strtol(mac_no_dev, NULL, 16);
		}
		return 1;
	}
	
	return 0;
}
static unsigned char StringToInputGain(char * str)
{
	if (!strcmp(str, "MIC"))
	{
		return INPUT_GAIN_MIC;
	}
	else if (!strcmp(str, "10dBv"))
	{
		return INPUT_GAIN_10dBv;
	}
	else if (!strcmp(str, "22dBu"))
	{
		return INPUT_GAIN_22dBu;
	}
	else if (!strcmp(str, "24dBu"))
	{
		return INPUT_GAIN_24dBu;
	}
	else
	{
		return 0;
	}
}
static unsigned char StringToOutputGain(char * str)
{
	if (!strcmp(str, "10dBv"))
	{
		return OUTPUT_GAIN_10dBv;
	}
	else if (!strcmp(str, "22dBu"))
	{
		return OUTPUT_GAIN_22dBu;
	}
	else if (!strcmp(str, "24dBu"))
	{
		return OUTPUT_GAIN_24dBu;
	}
	else
	{
		return 0;
	}
}
static unsigned int StringToRebootConfirmLevel(char * str)
{
	if (!strcmp(str, "reboot_confirm"))
	{
		return 0;
	}
	else if (!strcmp(str, "clear_preset_confirm"))
	{
		return 1;
	}
	else if (!strcmp(str, "init_confirm"))
	{
		return 2;
	}
	else
	{
		return 0xFFFFFFFF;
	}
}
static unsigned int StringToRebootLevel(char * str)
{
	if (!strcmp(str, "reboot"))
	{
		return 0;
	}
	else if (!strcmp(str, "clear_preset"))
	{
		return 1;
	}
	else if (!strcmp(str, "init"))
	{
		return 2;
	}
	else
	{
		return 0xFFFFFFFF;
	}
}
static int StringToPeq(FilterConfigMap * data_filter, SubStrings * peq_params)
{
	if (peq_params->count != 4)
		return 0;
	
	if (!IsValidVolume(peq_params->sub_strings[2]))
		return 0;
	if (!IsValidFreq(peq_params->sub_strings[0], 20, 20000))// 20Hz ~ 20000Hz
		return 0;
	
	// FREQ
	data_filter->FREQ = StringToFreq(peq_params->sub_strings[0]);// 20Hz ~ 20000Hz
	// Q
    data_filter->Q = TextToShort(peq_params->sub_strings[1], 100);
	// GAIN
	data_filter->GAIN = StringToVolume(peq_params->sub_strings[2], -300);// 最小-30dB
	// TYPE
	data_filter->TYPE = StringToTYPE(peq_params->sub_strings[3]);

	if (data_filter->TYPE == 0)
	{
		return 0;
	}
	
	if (data_filter->TYPE == EQ_TYPE_HighShelving || data_filter->TYPE == EQ_TYPE_LowShelving)
	{
		if (data_filter->GAIN > 120)
			data_filter->GAIN = 120;
	}
	
	return 1;
}
static int IsValidPeqName(char * name, int count)
{
	if (!strcmp(name, "peq"))
		return 1;
	if (!strcmp(name, "LPF") && count == 0)
		return 1;
	if (!strcmp(name, "HPF") && count == 0)
		return 1;
	
	return 0;
}
static int PeqNameToBand(char * name, int index)
{
    // L->1  [1-6]->[2-7] 7->10 H->11
	if (!strcmp(name, "LPF"))
		return 1;
	if (!strcmp(name, "HPF"))
		return 11;
	if (!strcmp(name, "peq"))
	{
		if (index >= 1 && index <= 6)
			return index+1;
		if (index == 7)
			return 10;
	}
	return -1;
}
static char * TextCmdPtr(char * const str)
{
	// 判断头部
	if (!strncmp(str, D1608CMD_FLAG, strlen(D1608CMD_FLAG)))
		return str+strlen(D1608CMD_FLAG);
	if (!strncmp(str, D1608CMD_CONTROL_FLAG, strlen(D1608CMD_CONTROL_FLAG)))
		return str+strlen(D1608CMD_CONTROL_FLAG);
	if (!strncmp(str, D1608CMD_KEEPLIVE_FLAG, strlen(D1608CMD_KEEPLIVE_FLAG)))
		return str+strlen(D1608CMD_KEEPLIVE_FLAG);

	return str;
}
std::vector<UINT> ProcessTextCommand(String command_head)
{
    std::vector<UINT> ret;
    int cmd_id;

	char * command = TextCmdPtr(command_head.c_str());	// 需要转换，跳过head
	//int command_length = strlen(command);
	//int command_head_length = strlen(command_head) - command_length;
	
	CommandContent result = {0};

	if (parse(command, &result))
	{
        if (result.var_count == 2 && !strcmp(result.vars[0].var_name, "config"))
		{
            char mac[10] = "";
            UINT ip;

            if (!strcmp(result.vars[1].var_name, "avaliable_preset") && result.vars[1].range_count==1 && IsValidBoolString(result.value))
            {
                int preset_id = result.vars[1].range[0];
                if (preset_id<=8 && preset_id>=1)
                {
                    global_config.avaliable_preset[preset_id-1] = StringToBool(result.value);
                }
            }
            else if (!strcmp(result.vars[1].var_name, "menu_key_function") && StringToMenuFunction(result.value)!=-1)
            {
                global_config.menu_key_function = StringToMenuFunction(result.value);
            }
            else if (!strcmp(result.vars[1].var_name, "up_key_function") && StringToMenuFunction(result.value)!=-1)
            {
                global_config.up_key_function = StringToMenuFunction(result.value);
            }
            else if (!strcmp(result.vars[1].var_name, "down_key_function") && StringToMenuFunction(result.value)!=-1)
            {
                global_config.down_key_function = StringToMenuFunction(result.value);
            }
            else if (!strcmp(result.vars[1].var_name, "ip") && (ip=StringToIp(result.value))!=0xFFFFFFFF)
            {
                global_config.static_ip_address = ip;
            }
            else if (!strcmp(result.vars[1].var_name, "mac_address") && StringToMac(result.value, mac))
            {
                memcpy(global_config.mac_address, mac, 6);
            }
            else if (!strcmp(result.vars[1].var_name, "preset_name") && result.vars[1].range_count==1)
            {
                int preset_id = result.vars[1].range[0];
                if (preset_id<=8 && preset_id>=1)
                {
                    strncpy(global_config.preset_name[preset_id-1], result.value, 16);
                }
            }
            else if (!strcmp(result.vars[1].var_name, "name"))
            {
                strncpy(global_config.d1616_name, result.value, 16);
            }
            else if (!strcmp(result.vars[1].var_name, "avaliable_preset") && result.vars[1].range_count==1 && IsValidBoolString(result.value))
            {
                int preset_id = result.vars[1].range[0];
                if (preset_id<=8 && preset_id>=1)
                {
                    global_config.avaliable_preset[preset_id-1] = StringToBool(result.value);
                }
            }
            /*else if (!strcmp(result.vars[1].var_name, "unlock"))
            {
                strncpy(global_config.unlock_string, result.value, 20);
                CheckUnlock();
            }*/
            else if (!strcmp(result.vars[1].var_name, "unlock_ext") && !strcmp(result.value, "on"))
            {
                global_config.running_timer_limit = 0;
                global_config.reboot_count_limit = 0;
                memset(global_config.locked_string, 0, sizeof(global_config.locked_string));
                memset(global_config.locked_string, 0, sizeof(global_config.password_of_key));
                memset(global_config.locked_string, 0, sizeof(global_config.password));
            }
            /*else if (!strcmp(result.vars[1].var_name, "adjust_running_time") && !strcmp(result.value, "on"))
            {
                SetLeaveTheFactory();
                WriteLog(EVENT_RESET_RUNNING_TIME, 0);
                // 不需要刷新
            }*/
            else if (!strcmp(result.vars[1].var_name, "use_global_name") && IsValidBoolString(result.value))
            {
                global_config.is_global_name = StringToBool(result.value);
            }
            else if (!strcmp(result.vars[1].var_name, "auto_saved") && IsValidBoolString(result.value))
            {
                global_config.auto_saved = StringToBool(result.value);
            }
            else if (!strcmp(result.vars[1].var_name, "lock_key") && IsValidBoolString(result.value))
            {
                global_config.lock_updownmenu = StringToBool(result.value);
            }
            else if (!strcmp(result.vars[1].var_name, "filename"))
            {
                strncpy(global_config.import_filename, result.value, 10);
            }
            else if (!strcmp(result.vars[1].var_name, "test") && IsValidBoolString(result.value))
            {
                global_config.led_test = StringToBool(result.value);
            }
            else if (!strcmp(result.vars[1].var_name, "usart1_ack") && IsValidBoolString(result.value))
            {
                global_config.usart1_receive_other_ack = StringToBool(result.value);
            }
            else if (!strcmp(result.vars[1].var_name, "usart3_ack") && IsValidBoolString(result.value))
            {
                global_config.usart3_receive_other_ack = StringToBool(result.value);
            }

            // 用 -1 表示立即刷新
            ret.insert(ret.begin(), -1);
		}
		// 统一处理，只有单值的feedback
		else if (result.var_count == 2 && !strcmp(result.vars[0].var_name, "master"))
		{
			if (!strcmp(result.vars[1].var_name, "volume") && IsValidVolume(result.value))
			{
                config_map.input_dsp[0].master_level_a = StringToVolume(result.value, -720);
				cmd_id = offsetof(ConfigMap, input_dsp[0].master_level_a);
                ret.insert(ret.begin(), cmd_id);
			}
			else if (!strcmp(result.vars[1].var_name, "mute") && IsValidBoolString(result.value))
			{
                config_map.input_dsp[0].master_mute_switch = StringToBool(result.value);
				cmd_id = offsetof(ConfigMap, input_dsp[0].master_mute_switch);
                ret.insert(ret.begin(), cmd_id);
			}
		}
		else if (result.var_count == 3
			&& !strcmp(result.vars[0].var_name, "input")
    		&& IsValidPeqName(result.vars[1].var_name, result.vars[1].range_count)
			&& !strcmp(result.vars[2].var_name, "bypass")
            && IsValidBoolString(result.value))
        {
			// channel 号
			int i;
			for (i=0;i<result.vars[0].range_count;i++)
			{
				int input_channel = result.vars[0].range[i] - 1;
				int j;

                int count = (result.vars[1].range_count == 0) ? 1 : result.vars[1].range_count;
				for (j=0;j<count;j++)
				{
    				int band = PeqNameToBand(result.vars[1].var_name, result.vars[1].range[j]) - 1;

                    config_map.input_dsp[input_channel].filter[band].bypass = StringToBool(result.value);
                    cmd_id = offsetof(ConfigMap, input_dsp[input_channel].filter[band]);
                    ret.insert(ret.begin(), cmd_id);
				}
			}
        }
		else if (result.var_count == 2
			&& !strcmp(result.vars[0].var_name, "input")
    		&& IsValidPeqName(result.vars[1].var_name, result.vars[1].range_count))
		{
			// channel 号
			int i;
			for (i=0;i<result.vars[0].range_count;i++)
			{
				int input_channel = result.vars[0].range[i] - 1;
				int j;

                int count = (result.vars[1].range_count == 0) ? 1 : result.vars[1].range_count;
				for (j=0;j<count;j++)
				{
    				int band = PeqNameToBand(result.vars[1].var_name, result.vars[1].range[j]) - 1;

					// 按照下划线分隔
					SubStrings peq_params = DivString(result.value, ',');
                    FilterConfigMap value;
					if (StringToPeq(&value, &peq_params))
					{
                        value.bypass = config_map.input_dsp[input_channel].filter[band].bypass; // 不能改变bypass值
                        config_map.input_dsp[input_channel].filter[band] = value;
						cmd_id = offsetof(ConfigMap, input_dsp[input_channel].filter[band]);
                        ret.insert(ret.begin(), cmd_id);
					}
				}
			}
		}
		// for部分统一处理，统一feedback，有多值反馈
		else if (result.var_count == 2 && !strcmp(result.vars[0].var_name, "input"))
		{
			// channel 号
			int i;
			for (i=0;i<result.vars[0].range_count;i++)
			{
				int input_channel = result.vars[0].range[i] - 1;
				if (!strcmp(result.vars[1].var_name, "eq") && IsValidBoolString(result.value))
				{
                    config_map.input_dsp[input_channel].eq_switch = StringToBool(result.value);
					cmd_id = offsetof(ConfigMap, input_dsp[input_channel].eq_switch);
                    ret.insert(ret.begin(), cmd_id);
				}
				else if (!strcmp(result.vars[1].var_name, "invert") && IsValidBoolString(result.value))
				{
                    config_map.input_dsp[input_channel].invert_switch = StringToBool(result.value);
					cmd_id = offsetof(ConfigMap, input_dsp[input_channel].invert_switch);
                    ret.insert(ret.begin(), cmd_id);
				}
				else if (!strcmp(result.vars[1].var_name, "noise") && IsValidBoolString(result.value))
				{
                    config_map.input_dsp[input_channel].noise_switch = StringToBool(result.value);
					cmd_id = offsetof(ConfigMap, input_dsp[input_channel].noise_switch);
                    ret.insert(ret.begin(), cmd_id);
				}
				else if (!strcmp(result.vars[1].var_name, "mute") && IsValidBoolString(result.value))
				{
                    config_map.input_dsp[input_channel].mute_switch = StringToBool(result.value);
					cmd_id = offsetof(ConfigMap, input_dsp[input_channel].mute_switch);
                    ret.insert(ret.begin(), cmd_id);
				}
				else if (!strcmp(result.vars[1].var_name, "phantom") && IsValidBoolString(result.value))
				{
                    config_map.input_dsp[input_channel].phantom_switch = StringToBool(result.value);
					cmd_id = offsetof(ConfigMap, input_dsp[input_channel].phantom_switch);
                    ret.insert(ret.begin(), cmd_id);
				}
				else if (!strcmp(result.vars[1].var_name, "volume") && IsValidVolume(result.value))
				{
                    config_map.input_dsp[input_channel].level_a = StringToVolume(result.value, -720);
                    cmd_id = offsetof(ConfigMap, input_dsp[input_channel].level_a);
                    ret.insert(ret.begin(), cmd_id);
				}
				else if (!strcmp(result.vars[1].var_name, "inside_volume") && IsValidVolume(result.value))
				{
                    config_map.input_dsp[input_channel].level_b = StringToVolume(result.value, -720);
					cmd_id = offsetof(ConfigMap, input_dsp[input_channel].level_b);
                    ret.insert(ret.begin(), cmd_id);
				}
				else if (!strcmp(result.vars[1].var_name, "gain") && StringToInputGain(result.value)!=0)
				{
                    config_map.input_dsp[input_channel].gain = StringToInputGain(result.value);
					cmd_id = offsetof(ConfigMap, input_dsp[input_channel].gain);
                    ret.insert(ret.begin(), cmd_id);
				}
				else if (!strcmp(result.vars[1].var_name, "delay") && IsValidTimeMs(result.value))
				{
                    config_map.input_dsp[input_channel].delay = StringToTimeMs(result.value, 1000);	// us
					cmd_id = offsetof(ConfigMap, input_dsp[input_channel].delay);
                    ret.insert(ret.begin(), cmd_id);
				}
				else if (!strcmp(result.vars[1].var_name, "name"))
				{
                    strncpy(config_map.input_dsp[input_channel].dsp_name, result.value, 6);
					cmd_id = offsetof(ConfigMap, input_dsp[input_channel].dsp_name);
                    ret.insert(ret.begin(), cmd_id);
				}
			}// for
		}
		else if (result.var_count == 3
			&& !strcmp(result.vars[0].var_name, "output")
    		&& IsValidPeqName(result.vars[1].var_name, result.vars[1].range_count)
			&& !strcmp(result.vars[2].var_name, "bypass")
            && IsValidBoolString(result.value))
        {
			// channel 号
			int i;
			for (i=0;i<result.vars[0].range_count;i++)
			{
				int output_channel = result.vars[0].range[i] - 1;
				int j;

                int count = (result.vars[1].range_count == 0) ? 1 : result.vars[1].range_count;
				for (j=0;j<count;j++)
				{
    				int band = PeqNameToBand(result.vars[1].var_name, result.vars[1].range[j]) - 1;

                    config_map.output_dsp[output_channel].filter[band].bypass = StringToBool(result.value);
                    cmd_id = offsetof(ConfigMap, output_dsp[output_channel].filter[band]);
                    ret.insert(ret.begin(), cmd_id);
				}
			}
        }
		else if (result.var_count == 2
			&& !strcmp(result.vars[0].var_name, "output")
    		&& IsValidPeqName(result.vars[1].var_name, result.vars[1].range_count))	// 只有PEQ命令，单独处理，单个feedback
		{
			// channel 号
			int i;
			for (i=0;i<result.vars[0].range_count;i++)
			{
				int output_channel = result.vars[0].range[i] - 1;
				int j;

                int count = (result.vars[1].range_count == 0) ? 1 : result.vars[1].range_count;
				for (j=0;j<count;j++)
				{
    				int band = PeqNameToBand(result.vars[1].var_name, result.vars[1].range[j]) - 1;

					// 按照下划线分隔
					SubStrings peq_params = DivString(result.value, ',');
                    FilterConfigMap value;
					if (StringToPeq(&value, &peq_params))
					{
                        config_map.output_dsp[output_channel].filter[band] = value;
						cmd_id = offsetof(ConfigMap, output_dsp[output_channel].filter[band]);
                        ret.insert(ret.begin(), cmd_id);
					}
				}
			}
		}
		else if (result.var_count == 2 && !strcmp(result.vars[0].var_name, "output"))	// for部分统一处理，统一feedback，有多值反馈
		{
			// channel 号
			int i;
			for (i=0;i<result.vars[0].range_count;i++)
			{
				int output_channel = result.vars[0].range[i] - 1;
				if (!strcmp(result.vars[1].var_name, "eq") && IsValidBoolString(result.value))
				{
                    config_map.output_dsp[output_channel].eq_switch = StringToBool(result.value);
					cmd_id = offsetof(ConfigMap, output_dsp[output_channel].eq_switch);
                    ret.insert(ret.begin(), cmd_id);
				}
				else if (!strcmp(result.vars[1].var_name, "invert") && IsValidBoolString(result.value))
				{
                    config_map.output_dsp[output_channel].invert_switch = StringToBool(result.value);
					cmd_id = offsetof(ConfigMap, output_dsp[output_channel].invert_switch);
                    ret.insert(ret.begin(), cmd_id);
				}
				else if (!strcmp(result.vars[1].var_name, "mute") && IsValidBoolString(result.value))
				{
                    config_map.output_dsp[output_channel].mute_switch = StringToBool(result.value);
					cmd_id = offsetof(ConfigMap, output_dsp[output_channel].mute_switch);
                    ret.insert(ret.begin(), cmd_id);
				}
				else if (!strcmp(result.vars[1].var_name, "volume") && IsValidVolume(result.value))
				{
                    config_map.output_dsp[output_channel].level_a = StringToVolume(result.value, -720);
					cmd_id = offsetof(ConfigMap, output_dsp[output_channel].level_a);
                    ret.insert(ret.begin(), cmd_id);
				}
				else if (!strcmp(result.vars[1].var_name, "inside_volume") && IsValidVolume(result.value))
				{
                    config_map.output_dsp[output_channel].level_b = StringToVolume(result.value, -720);
					cmd_id = offsetof(ConfigMap, output_dsp[output_channel].level_b);
                    ret.insert(ret.begin(), cmd_id);
				}
				else if (!strcmp(result.vars[1].var_name, "gain") && StringToOutputGain(result.value)!=0)
				{
                    config_map.output_dsp[output_channel].gain = StringToOutputGain(result.value);
					cmd_id = offsetof(ConfigMap, output_dsp[output_channel].gain);
                    ret.insert(ret.begin(), cmd_id);
				}
				else if (!strcmp(result.vars[1].var_name, "delay") && IsValidTimeMs(result.value))
				{
                    config_map.output_dsp[output_channel].delay = StringToTimeMs(result.value, 1000);	// us
					cmd_id = offsetof(ConfigMap, output_dsp[output_channel].delay);
                    ret.insert(ret.begin(), cmd_id);
				}
				else if (!strcmp(result.vars[1].var_name, "name"))
				{
                    strncpy(config_map.output_dsp[output_channel].dsp_name, result.value, 6);
					cmd_id = offsetof(ConfigMap, output_dsp[output_channel].dsp_name);
                    ret.insert(ret.begin(), cmd_id);
				}
				else if (!strcmp(result.vars[1].var_name, "comp") && IsValidBoolString(result.value))
				{
                    config_map.output_dsp[output_channel].comp_switch = StringToBool(result.value);
					cmd_id = offsetof(ConfigMap, output_dsp[output_channel].comp_switch);
                    ret.insert(ret.begin(), cmd_id);
				}
				else if (!strcmp(result.vars[1].var_name, "auto_comp") && IsValidBoolString(result.value))
				{
                    config_map.output_dsp[output_channel].auto_time = StringToBool(result.value);
					cmd_id = offsetof(ConfigMap, output_dsp[output_channel].auto_time);
                    ret.insert(ret.begin(), cmd_id);
				}
			} // for
		}
		else if (result.var_count == 3
			&& !strcmp(result.vars[0].var_name, "output")
			&& !strcmp(result.vars[1].var_name, "comp"))	// 只有压缩参数，单独处理，单一feedback
		{
			// channel 号
			int i;
			for (i=0;i<result.vars[0].range_count;i++)
			{
				int output_channel = result.vars[0].range[i] - 1;
				if (!strcmp(result.vars[2].var_name, "ratio") && IsValidRatio(result.value))
				{
                    config_map.output_dsp[output_channel].ratio = StringToRatio(result.value);
					cmd_id = offsetof(ConfigMap, output_dsp[output_channel].ratio);
                    ret.insert(ret.begin(), cmd_id);
				}
				else if (!strcmp(result.vars[2].var_name, "threshold") && IsValidVolume(result.value))
				{
                    config_map.output_dsp[output_channel].threshold = StringToVolume(result.value, 0);
					cmd_id = offsetof(ConfigMap, output_dsp[output_channel].threshold);
                    ret.insert(ret.begin(), cmd_id);
				}
				else if (!strcmp(result.vars[2].var_name, "attack_time") && IsValidTimeMs(result.value))
				{
                    config_map.output_dsp[output_channel].attack_time = StringToTimeMs(result.value, 10);	// ms
					cmd_id = offsetof(ConfigMap, output_dsp[output_channel].attack_time);
                    ret.insert(ret.begin(), cmd_id);
				}
				else if (!strcmp(result.vars[2].var_name, "release_time") && IsValidTimeMs(result.value))
				{
                    config_map.output_dsp[output_channel].release_time = StringToTimeMs(result.value, 10);	// ms
					cmd_id = offsetof(ConfigMap, output_dsp[output_channel].release_time);
                    ret.insert(ret.begin(), cmd_id);
				}
				else if (!strcmp(result.vars[2].var_name, "gain") && IsValidVolume(result.value))
				{
                    config_map.output_dsp[output_channel].comp_gain = StringToVolume(result.value, 0);	// 0 - 24 dB
					cmd_id = offsetof(ConfigMap, output_dsp[output_channel].comp_gain);
                    ret.insert(ret.begin(), cmd_id);
				}
			}
		}
		else if (result.var_count == 3
			&& !strcmp(result.vars[0].var_name, "output")
			&& !strcmp(result.vars[1].var_name, "route_input"))	// 只有mix参数，单一处理，单个output批量feedback
		{
			// channel 号
			int i;
			for (i=0;i<result.vars[0].range_count;i++)
			{
				int output_channel = result.vars[0].range[i] - 1;
				int j;

				for (j=0;j<result.vars[1].range_count;j++)
				{
					int input_channel = result.vars[1].range[j] - 1;
					
					if ((!strcmp(result.vars[2].var_name, "mute"))  && IsValidBoolString(result.value))
					{
                        config_map.output_dsp[output_channel].mix_mute[input_channel] = StringToBool(result.value);
						cmd_id = offsetof(ConfigMap, output_dsp[output_channel].mix_mute[input_channel]);
                        ret.insert(ret.begin(), cmd_id);
					}
					else if (!strcmp(result.vars[2].var_name, "volume") && IsValidVolume(result.value))
					{
                        config_map.output_dsp[output_channel].mix[input_channel] = StringToVolume(result.value, -720);
						cmd_id = offsetof(ConfigMap, output_dsp[output_channel].mix[input_channel]);
                        ret.insert(ret.begin(), cmd_id);
					}
				}
			}
		}
	}
    return ret;
}

