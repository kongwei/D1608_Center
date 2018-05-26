//---------------------------------------------------------------------------


#pragma hdrstop
#include <vcl.h>

#include "untTextCmdParse.h"

#include "TextCmdParse.h"
#include <mem.h>
#include <string.h>
#include "D1608Pack.h"

#pragma package(smart_init)

#if defined (WIN32)
    #define strcasecmp stricmp
    #define strncasecmp strnicmp
#endif


int DelayProcessTextCommand(String command_head, std::vector<UINT> & cmd_queue)
{
    std::vector<UINT> ret = ProcessTextCommand(command_head);

    if (ret.size() == 0)
    {
        return -1;
    }

    for (UINT cmd_index=0; cmd_index<ret.size(); cmd_index++)
    {
        cmd_queue.push_back(ret[cmd_index]);
    }

    return 0;
}

std::vector<UINT> ProcessTextCommand(String command_head)
{
    std::vector<UINT> ret;
    int cmd_id;

	char * command = TextCmdPtr(command_head.c_str());	// 需要转换，跳过head
	//int command_length = strlen(command);
	//int command_head_length = strlen(command_head) - command_length;
	
	CommandContent result = {0};

	if (parse(command, &result)==0)
	{
        if (result.var_count == 2 && !strcasecmp(result.vars[0].var_name, "config"))
		{
            char mac[10] = "";
            UINT ip;

            if (!strcasecmp(result.vars[1].var_name, "avaliable_preset") && result.vars[1].range_count==1 && IsValidBoolString(result.value))
            {
                int preset_id = result.vars[1].range[0];
                if (preset_id<=8 && preset_id>=1)
                {
                    global_config.avaliable_preset[preset_id-1] = StringToBool(result.value);
                }
            }
            else if (!strcasecmp(result.vars[1].var_name, "menu_key_function") && StringToMenuFunction(result.value)!=-1)
            {
                global_config.menu_key_function = StringToMenuFunction(result.value);
            }
            else if (!strcasecmp(result.vars[1].var_name, "up_key_function") && StringToMenuFunction(result.value)!=-1)
            {
                global_config.up_key_function = StringToMenuFunction(result.value);
            }
            else if (!strcasecmp(result.vars[1].var_name, "down_key_function") && StringToMenuFunction(result.value)!=-1)
            {
                global_config.down_key_function = StringToMenuFunction(result.value);
            }
            else if (!strcasecmp(result.vars[1].var_name, "ip") && (ip=StringToIp(result.value))!=0xFFFFFFFF)
            {
                global_config.static_ip_address = ip;
            }
            else if (!strcasecmp(result.vars[1].var_name, "mac_address") && StringToMac(result.value, mac))
            {
                memcpy(global_config.mac_address, mac, 6);
            }
            else if (!strcasecmp(result.vars[1].var_name, "preset_name") && result.vars[1].range_count==1)
            {
                int preset_id = result.vars[1].range[0];
                if (preset_id<=8 && preset_id>=1)
                {
                    strncpy(global_config.preset_name[preset_id-1], result.value, 16);
                }
            }
            else if (!strcasecmp(result.vars[1].var_name, "name"))
            {
                strncpy(global_config.d1616_name, result.value, 16);
            }
            else if (!strcasecmp(result.vars[1].var_name, "avaliable_preset") && result.vars[1].range_count==1 && IsValidBoolString(result.value))
            {
                int preset_id = result.vars[1].range[0];
                if (preset_id<=8 && preset_id>=1)
                {
                    global_config.avaliable_preset[preset_id-1] = StringToBool(result.value);
                }
            }
            /*else if (!strcasecmp(result.vars[1].var_name, "unlock"))
            {
                strncpy(global_config.unlock_string, result.value, 20);
                CheckUnlock();
            }*/
            else if (!strcasecmp(result.vars[1].var_name, "unlock_ext") && !strcasecmp(result.value, "on"))
            {
                global_config.running_timer_limit = 0;
                global_config.reboot_count_limit = 0;
                memset(global_config.locked_string, 0, sizeof(global_config.locked_string));
                memset(global_config.locked_string, 0, sizeof(global_config.password_of_key));
                memset(global_config.locked_string, 0, sizeof(global_config.password));
            }
            /*else if (!strcasecmp(result.vars[1].var_name, "adjust_running_time") && !strcasecmp(result.value, "on"))
            {
                SetLeaveTheFactory();
                WriteLog(EVENT_RESET_RUNNING_TIME, 0);
                // 不需要刷新
            }*/
            else if (!strcasecmp(result.vars[1].var_name, "use_global_name") && IsValidBoolString(result.value))
            {
                global_config.is_global_name = StringToBool(result.value);
            }
            else if (!strcasecmp(result.vars[1].var_name, "auto_saved") && IsValidBoolString(result.value))
            {
                global_config.auto_saved = StringToBool(result.value);
            }
            else if (!strcasecmp(result.vars[1].var_name, "lock_key") && IsValidBoolString(result.value))
            {
                global_config.lock_updownmenu = StringToBool(result.value);
            }
            else if (!strcasecmp(result.vars[1].var_name, "filename"))
            {
                strncpy(global_config.import_filename, result.value, 10);
            }
            else if (!strcasecmp(result.vars[1].var_name, "test") && IsValidBoolString(result.value))
            {
                global_config.led_test = StringToBool(result.value);
            }
            else if (!strcasecmp(result.vars[1].var_name, "usart1_ack") && IsValidBoolString(result.value))
            {
                global_config.usart1_receive_other_ack = StringToBool(result.value);
            }
            else if (!strcasecmp(result.vars[1].var_name, "usart3_ack") && IsValidBoolString(result.value))
            {
                global_config.usart3_receive_other_ack = StringToBool(result.value);
            }

            // 用 -1 表示立即刷新
            ret.insert(ret.begin(), -1);
		}
		// 统一处理，只有单值的feedback
		else if (result.var_count == 2 && !strcasecmp(result.vars[0].var_name, "master"))
		{
			if (!strcasecmp(result.vars[1].var_name, "volume") && IsValidVolume(result.value))
			{
                config_map.input_dsp[0].master_level_a = StringToVolume(result.value, -720);
				cmd_id = offsetof(ConfigMap, input_dsp[0].master_level_a);
                ret.insert(ret.begin(), cmd_id);
			}
			else if (!strcasecmp(result.vars[1].var_name, "mute") && IsValidBoolString(result.value))
			{
                config_map.input_dsp[0].master_mute_switch = StringToBool(result.value);
				cmd_id = offsetof(ConfigMap, input_dsp[0].master_mute_switch);
                ret.insert(ret.begin(), cmd_id);
			}
		}
		else if (result.var_count == 3
			&& !strcasecmp(result.vars[0].var_name, "input")
    		&& IsValidPeqName(result.vars[1].var_name, result.vars[1].range_count)
			&& !strcasecmp(result.vars[2].var_name, "bypass")
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
			&& !strcasecmp(result.vars[0].var_name, "input")
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
                    FilterConfigMap value = config_map.input_dsp[input_channel].filter[band];
					if (StringToPeq(&value, &peq_params, result.vars[1].var_name))
					{
                        config_map.input_dsp[input_channel].filter[band] = value;
						cmd_id = offsetof(ConfigMap, input_dsp[input_channel].filter[band]);
                        ret.insert(ret.begin(), cmd_id);
					}
				}
			}
		}
		// for部分统一处理，统一feedback，有多值反馈
		else if (result.var_count == 2 && !strcasecmp(result.vars[0].var_name, "input"))
		{
			// channel 号
			int i;
			for (i=0;i<result.vars[0].range_count;i++)
			{
				int input_channel = result.vars[0].range[i] - 1;
				if (!strcasecmp(result.vars[1].var_name, "eq") && IsValidBoolString(result.value))
				{
                    config_map.input_dsp[input_channel].eq_switch = StringToBool(result.value);
					cmd_id = offsetof(ConfigMap, input_dsp[input_channel].eq_switch);
                    ret.insert(ret.begin(), cmd_id);
				}
				else if (!strcasecmp(result.vars[1].var_name, "invert") && IsValidBoolString(result.value))
				{
                    config_map.input_dsp[input_channel].invert_switch = StringToBool(result.value);
					cmd_id = offsetof(ConfigMap, input_dsp[input_channel].invert_switch);
                    ret.insert(ret.begin(), cmd_id);
				}
				else if (!strcasecmp(result.vars[1].var_name, "noise") && IsValidBoolString(result.value))
				{
                    config_map.input_dsp[input_channel].noise_switch = StringToBool(result.value);
					cmd_id = offsetof(ConfigMap, input_dsp[input_channel].noise_switch);
                    ret.insert(ret.begin(), cmd_id);
				}
				else if (!strcasecmp(result.vars[1].var_name, "mute") && IsValidBoolString(result.value))
				{
                    config_map.input_dsp[input_channel].mute_switch = StringToBool(result.value);
					cmd_id = offsetof(ConfigMap, input_dsp[input_channel].mute_switch);
                    ret.insert(ret.begin(), cmd_id);
				}
				else if (!strcasecmp(result.vars[1].var_name, "phantom") && IsValidBoolString(result.value))
				{
                    config_map.input_dsp[input_channel].phantom_switch = StringToBool(result.value);
					cmd_id = offsetof(ConfigMap, input_dsp[input_channel].phantom_switch);
                    ret.insert(ret.begin(), cmd_id);
				}
				else if (!strcasecmp(result.vars[1].var_name, "volume") && IsValidVolume(result.value))
				{
                    config_map.input_dsp[input_channel].level_a = StringToVolume(result.value, -720);
                    cmd_id = offsetof(ConfigMap, input_dsp[input_channel].level_a);
                    ret.insert(ret.begin(), cmd_id);
				}
				else if (!strcasecmp(result.vars[1].var_name, "inside_volume") && IsValidVolume(result.value))
				{
                    config_map.input_dsp[input_channel].level_b = StringToVolume(result.value, -720);
					cmd_id = offsetof(ConfigMap, input_dsp[input_channel].level_b);
                    ret.insert(ret.begin(), cmd_id);
				}
				else if (!strcasecmp(result.vars[1].var_name, "gain") && StringToInputGain(result.value)!=0)
				{
                    config_map.input_dsp[input_channel].gain = StringToInputGain(result.value);
					cmd_id = offsetof(ConfigMap, input_dsp[input_channel].gain);
                    ret.insert(ret.begin(), cmd_id);
				}
				else if (!strcasecmp(result.vars[1].var_name, "delay") && IsValidTime(result.value, NULL))
				{
                    config_map.input_dsp[input_channel].delay = StringToTimeMs(result.value, 1000);	// us
					cmd_id = offsetof(ConfigMap, input_dsp[input_channel].delay);
                    ret.insert(ret.begin(), cmd_id);
				}
				else if (!strcasecmp(result.vars[1].var_name, "name"))
				{
                    strncpy(config_map.input_dsp[input_channel].dsp_name, result.value, 6);
					cmd_id = offsetof(ConfigMap, input_dsp[input_channel].dsp_name);
                    ret.insert(ret.begin(), cmd_id);
				}
			}// for
		}
		else if (result.var_count == 3
			&& !strcasecmp(result.vars[0].var_name, "output")
    		&& IsValidPeqName(result.vars[1].var_name, result.vars[1].range_count)
			&& !strcasecmp(result.vars[2].var_name, "bypass")
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
			&& !strcasecmp(result.vars[0].var_name, "output")
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
                    FilterConfigMap value = config_map.output_dsp[output_channel].filter[band];
					if (StringToPeq(&value, &peq_params, result.vars[1].var_name))
					{
                        config_map.output_dsp[output_channel].filter[band] = value;
						cmd_id = offsetof(ConfigMap, output_dsp[output_channel].filter[band]);
                        ret.insert(ret.begin(), cmd_id);
					}
				}
			}
		}
		else if (result.var_count == 2 && !strcasecmp(result.vars[0].var_name, "output"))	// for部分统一处理，统一feedback，有多值反馈
		{
			// channel 号
			int i;
			for (i=0;i<result.vars[0].range_count;i++)
			{
				int output_channel = result.vars[0].range[i] - 1;
				if (!strcasecmp(result.vars[1].var_name, "eq") && IsValidBoolString(result.value))
				{
                    config_map.output_dsp[output_channel].eq_switch = StringToBool(result.value);
					cmd_id = offsetof(ConfigMap, output_dsp[output_channel].eq_switch);
                    ret.insert(ret.begin(), cmd_id);
				}
				else if (!strcasecmp(result.vars[1].var_name, "invert") && IsValidBoolString(result.value))
				{
                    config_map.output_dsp[output_channel].invert_switch = StringToBool(result.value);
					cmd_id = offsetof(ConfigMap, output_dsp[output_channel].invert_switch);
                    ret.insert(ret.begin(), cmd_id);
				}
				else if (!strcasecmp(result.vars[1].var_name, "mute") && IsValidBoolString(result.value))
				{
                    config_map.output_dsp[output_channel].mute_switch = StringToBool(result.value);
					cmd_id = offsetof(ConfigMap, output_dsp[output_channel].mute_switch);
                    ret.insert(ret.begin(), cmd_id);
				}
				else if (!strcasecmp(result.vars[1].var_name, "volume") && IsValidVolume(result.value))
				{
                    config_map.output_dsp[output_channel].level_a = StringToVolume(result.value, -720);
					cmd_id = offsetof(ConfigMap, output_dsp[output_channel].level_a);
                    ret.insert(ret.begin(), cmd_id);
				}
				else if (!strcasecmp(result.vars[1].var_name, "inside_volume") && IsValidVolume(result.value))
				{
                    config_map.output_dsp[output_channel].level_b = StringToVolume(result.value, -720);
					cmd_id = offsetof(ConfigMap, output_dsp[output_channel].level_b);
                    ret.insert(ret.begin(), cmd_id);
				}
				else if (!strcasecmp(result.vars[1].var_name, "gain") && StringToOutputGain(result.value)!=0)
				{
                    config_map.output_dsp[output_channel].gain = StringToOutputGain(result.value);
					cmd_id = offsetof(ConfigMap, output_dsp[output_channel].gain);
                    ret.insert(ret.begin(), cmd_id);
				}
				else if (!strcasecmp(result.vars[1].var_name, "delay") && IsValidTime(result.value, NULL))
				{
                    config_map.output_dsp[output_channel].delay = StringToTimeMs(result.value, 1000);	// us
					cmd_id = offsetof(ConfigMap, output_dsp[output_channel].delay);
                    ret.insert(ret.begin(), cmd_id);
				}
				else if (!strcasecmp(result.vars[1].var_name, "name"))
				{
                    strncpy(config_map.output_dsp[output_channel].dsp_name, result.value, 6);
					cmd_id = offsetof(ConfigMap, output_dsp[output_channel].dsp_name);
                    ret.insert(ret.begin(), cmd_id);
				}
				else if (!strcasecmp(result.vars[1].var_name, "comp") && IsValidBoolString(result.value))
				{
                    config_map.output_dsp[output_channel].comp_switch = StringToBool(result.value);
					cmd_id = offsetof(ConfigMap, output_dsp[output_channel].comp_switch);
                    ret.insert(ret.begin(), cmd_id);
				}
				else if (!strcasecmp(result.vars[1].var_name, "auto_comp") && IsValidBoolString(result.value))
				{
                    config_map.output_dsp[output_channel].auto_time = StringToBool(result.value);
					cmd_id = offsetof(ConfigMap, output_dsp[output_channel].auto_time);
                    ret.insert(ret.begin(), cmd_id);
				}
			} // for
		}
		else if (result.var_count == 3
			&& !strcasecmp(result.vars[0].var_name, "output")
			&& !strcasecmp(result.vars[1].var_name, "comp"))	// 只有压缩参数，单独处理，单一feedback
		{
			// channel 号
			int i;
			for (i=0;i<result.vars[0].range_count;i++)
			{
				int output_channel = result.vars[0].range[i] - 1;
				if (!strcasecmp(result.vars[2].var_name, "ratio") && IsValidRatio(result.value))
				{
                    config_map.output_dsp[output_channel].ratio = StringToRatio(result.value);
					cmd_id = offsetof(ConfigMap, output_dsp[output_channel].ratio);
                    ret.insert(ret.begin(), cmd_id);
				}
				else if (!strcasecmp(result.vars[2].var_name, "threshold") && IsValidVolume(result.value))
				{
                    config_map.output_dsp[output_channel].threshold = StringToVolume(result.value, 0);
					cmd_id = offsetof(ConfigMap, output_dsp[output_channel].threshold);
                    ret.insert(ret.begin(), cmd_id);
				}
				else if (!strcasecmp(result.vars[2].var_name, "attack_time") && IsValidTime(result.value, NULL))
				{
                    config_map.output_dsp[output_channel].attack_time = StringToTimeMs(result.value, 10);	// ms
					cmd_id = offsetof(ConfigMap, output_dsp[output_channel].attack_time);
                    ret.insert(ret.begin(), cmd_id);
				}
				else if (!strcasecmp(result.vars[2].var_name, "release_time") && IsValidTime(result.value, NULL))
				{
                    config_map.output_dsp[output_channel].release_time = StringToTimeMs(result.value, 10);	// ms
					cmd_id = offsetof(ConfigMap, output_dsp[output_channel].release_time);
                    ret.insert(ret.begin(), cmd_id);
				}
				else if (!strcasecmp(result.vars[2].var_name, "gain") && IsValidVolume(result.value))
				{
                    config_map.output_dsp[output_channel].comp_gain = StringToVolume(result.value, 0);	// 0 - 24 dB
					cmd_id = offsetof(ConfigMap, output_dsp[output_channel].comp_gain);
                    ret.insert(ret.begin(), cmd_id);
				}
			}
		}
		else if (result.var_count == 3
			&& !strcasecmp(result.vars[0].var_name, "output")
			&& !strcasecmp(result.vars[1].var_name, "route_input"))	// 只有mix参数，单一处理，单个output批量feedback
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
					
					if ((!strcasecmp(result.vars[2].var_name, "mute"))  && IsValidBoolString(result.value))
					{
                        config_map.output_dsp[output_channel].mix_mute[input_channel] = StringToBool(result.value);
						cmd_id = offsetof(ConfigMap, output_dsp[output_channel].mix_mute[input_channel]);
                        ret.insert(ret.begin(), cmd_id);
					}
					else if (!strcasecmp(result.vars[2].var_name, "volume") && IsValidVolume(result.value))
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

