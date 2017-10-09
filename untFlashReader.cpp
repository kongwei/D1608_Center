//---------------------------------------------------------------------------


#pragma hdrstop
#include <Classes.hpp>
#include "untFlashReader.h"
#include "Coefficient.h"
#include <string.h>
//---------------------------------------------------------------------------

#pragma package(smart_init)

#define PAGE_SIZE  (0x800)

#define CONFIG_ID 500

#define PRESET_SIZE (96*1024)

typedef struct
{
	unsigned char* preset_address[8][5];
	unsigned char* config_address;
}Using_Page;

typedef struct
{
	int count;
	unsigned int id;
	char check;
	char padt[7];
}Page_Header;

static int IsPageChecked(unsigned char* address)
{
	// 按照2048进行校验。所有数据相加得0
	unsigned char * p_check = (unsigned char*)address;
	unsigned char sum = 0;
	int i;
	for (i=0;i<PAGE_SIZE;i++)
	{
		sum += p_check[i];
	}
	return sum == 0;
}

static void GetUsingPage(Using_Page * using_page, unsigned char* flash_dump_data)
{
	int i;
	memset(using_page, 0, sizeof(Using_Page));
	for (i=0; i<PRESET_SIZE;i+=2048)
	{
		int preset_id, preset_index;
		unsigned char* address = i + flash_dump_data;
		Page_Header * p_head;
		p_head = (Page_Header*)address;
		preset_id = p_head->id / 10;
		preset_index = p_head->id % 10;
		if (preset_id>=1 && preset_id<=8 && preset_index<5)
		{
			// 检查校验和
			if (IsPageChecked(address))
			{
				if (using_page->preset_address[preset_id-1][preset_index] == 0)
					using_page->preset_address[preset_id-1][preset_index] = address;
				else
				{
					Page_Header * p_head_last = (Page_Header*)using_page->preset_address[preset_id-1][preset_index];
					if (p_head_last->count < p_head->count)
						using_page->preset_address[preset_id-1][preset_index] = address;
				}
			}
		}
		else if (p_head->id == CONFIG_ID)
		{
			Page_Header * p_head_last = (Page_Header*)using_page->config_address;
			if (p_head_last == 0x00000000)
				using_page->config_address = address;
			else if (p_head_last->count < p_head->count)
				using_page->config_address = address;
		}
	}
}

void LoadPresetById(int preset_id, ConfigMap& tmp_config_map, unsigned char* flash_dump_data)
{
	Using_Page using_page;
	GetUsingPage(&using_page, flash_dump_data);

	InitConfigMap(tmp_config_map);
	// input
	if (using_page.preset_address[preset_id-1][0] != 0)
		memcpy(&tmp_config_map.input_dsp[0],
			(char*)using_page.preset_address[preset_id-1][0]+sizeof(Page_Header),
			sizeof(config_map.input_dsp[0])*8);
	if (using_page.preset_address[preset_id-1][1] != 0)
		memcpy(&tmp_config_map.input_dsp[8],
			(char*)using_page.preset_address[preset_id-1][1]+sizeof(Page_Header),
			sizeof(config_map.input_dsp[0])*8);
	// output
	if (using_page.preset_address[preset_id-1][2] != 0)
		memcpy(&tmp_config_map.output_dsp[0],
			(char*)using_page.preset_address[preset_id-1][2]+sizeof(Page_Header),
			sizeof(config_map.output_dsp[0])*8);
	if (using_page.preset_address[preset_id-1][3] != 0)
		memcpy(&tmp_config_map.output_dsp[8],
			(char*)using_page.preset_address[preset_id-1][3]+sizeof(Page_Header),
			sizeof(config_map.output_dsp[0])*8);
	// mix & other
	if (using_page.preset_address[preset_id-1][4] != 0)
		memcpy(&tmp_config_map.master_mix,
			(char*)using_page.preset_address[preset_id-1][4]+sizeof(Page_Header),
			sizeof(config_map.master_mix));
}

static void ResetGlobalConfig(GlobalConfig& global_config)
{
	global_config.is_global_name = 0;
    memset(global_config.input_dsp_name, 0, sizeof(global_config.input_dsp_name));
    memset(global_config.output_dsp_name, 0, sizeof(global_config.output_dsp_name));
    memset(global_config.preset_name, 0, sizeof(global_config.preset_name));
    memset(global_config.d1616_name, 0, sizeof(global_config.d1616_name));
	global_config.version = sizeof(global_config);
    global_config.active_preset_id = 1;
	global_config.static_ip_address = 0;

	// 默认preset 1、preset 2可用
	global_config.avaliable_preset[0] = 1;
	global_config.avaliable_preset[1] = 0;
	global_config.avaliable_preset[2] = 0;
	global_config.avaliable_preset[3] = 0;
	global_config.avaliable_preset[4] = 0;
	global_config.avaliable_preset[5] = 0;
	global_config.avaliable_preset[6] = 0;
	global_config.avaliable_preset[7] = 0;

	global_config.auto_saved = 0;
}

void LoadGlobalConfig(GlobalConfig& global_config, unsigned char* flash_dump_data)
{
	Using_Page using_page;
	GetUsingPage(&using_page, flash_dump_data);
	
	if (using_page.config_address != 0)
	{
		memcpy(&global_config,
			(char*)using_page.config_address+sizeof(Page_Header),
			sizeof(global_config));

		// TODO: 校验global_config数据是否正确
	}
	else
	{
		ResetGlobalConfig(global_config);
	}
}

//---------------------------------------------
Event * current_event_ptr = NULL;
static Event * ReRangeEventPoint(Event * p)
{
	if (p >= (Event *)(LOG_START_PAGE+LOG_SIZE))
	{
		p = (Event *)(LOG_START_PAGE);
	}
	return p;
}
static void MoveNextEventPtr()
{
	// move to next page
	current_event_ptr = ReRangeEventPoint(current_event_ptr+1);
}
// 计算 Log 位置
Event * GetLogPtr(Event log_data[EVENT_POOL_SIZE])
{
	uint64_t last_timer;
	short last_power_on_count;
	Event * event_ptr;
	short last_save_ok_count = 0;

    current_event_ptr = log_data;

	// 遍历
	for (event_ptr = log_data;
		event_ptr<log_data+EVENT_POOL_SIZE;
		event_ptr++)
	{
		if (event_ptr->timer!=0xFFFFFFFF && event_ptr->event_id == EVENT_POWER_OFF)
		{
			if (last_power_on_count < event_ptr->event_data)
			{
				last_power_on_count = event_ptr->event_data;
				last_timer = event_ptr->timer;
				current_event_ptr = event_ptr;
			}
		}
		else if (event_ptr->timer!=0xFFFFFFFF && event_ptr->event_id == EVENT_POWER_SAVE_OK)
		{
			if (last_save_ok_count < event_ptr->event_data)
			{
				last_save_ok_count = event_ptr->event_data;
			}
		}
	}
	
	last_timer *= 100;
	
	if (current_event_ptr == (Event *)LOG_START_PAGE)
	{
		// 从后向前找到一个有数据的日志
		for (event_ptr = (Event *)(LOG_START_PAGE+LOG_SIZE-sizeof(Event));
			event_ptr>=(Event *)LOG_START_PAGE;
			event_ptr--)
		{
			if (event_ptr->timer!=0xFFFFFFFF)
			{
				current_event_ptr = ReRangeEventPoint(event_ptr + 1);
				break;
			}
		}
	}
	else
	{
		while(current_event_ptr->timer!=0xFFFFFFFF)
			MoveNextEventPtr();
	}

    return current_event_ptr;
}

