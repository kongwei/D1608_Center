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

#define PRESET_SIZE (86*1024)


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

void GetUsingPage(Using_Page * using_page, unsigned char* flash_dump_data)
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
		if (preset_id>=1 && preset_id<=8 && preset_index<4)
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

static unsigned char GetCheckSum(const void * address, int size)
{
	unsigned char * p_check = (unsigned char*)address;
	unsigned char sum = 0;
	int i;
	for (i=0;i<size;i++)
	{
		sum += p_check[i];
	}
	return sum;
}
// 全FF的校验和
static unsigned char GetPadCheckSum(int size)
{
	return 0 * size;
}
static void UpdatePageHeaderCheckSum(Page_Header * header, const void* address, int size, const void* dict_address, int dict_size)
{
	// 按照2048进行校验。所有数据相加得0
	unsigned char header_checksum = GetCheckSum(header, sizeof(Page_Header));
	unsigned char data_checksum = GetCheckSum(address, size);
	unsigned char dict_checksum = GetCheckSum(dict_address, dict_size);
	unsigned char pad_checksum = GetPadCheckSum(PAGE_SIZE-sizeof(Page_Header)-size-dict_size);
	
	header->check -= header_checksum+data_checksum+dict_checksum+pad_checksum;
}
void SavePresetById(int preset_id, int index, void * buf, unsigned char* flash_dump_data)
{
    int address = ((preset_id-1)*8 + index)*2048;

    memset(flash_dump_data+address, 0, 2048);

    void * src = NULL;
    int size = 0;
    Page_Header header;

    const void * dict_ptr = NULL;
    int dict_size = 0;

    switch (index)
    {
    case 0:
        src = buf;
        size = sizeof(config_map.input_dsp[0]) * 8;
        dict_ptr = &input_dsp_ola_list;
        dict_size = sizeof(input_dsp_ola_list);
        break;
    case 1:
        src = buf;
        size = sizeof(config_map.input_dsp[0]) * 8;
        dict_ptr = &input_dsp_ola_list;
        dict_size = sizeof(input_dsp_ola_list);
        break;
    case 2:
        src = buf;
        size = sizeof(config_map.output_dsp[0]) * 8;
        break;
    case 3:
        src = buf;
        size = sizeof(config_map.output_dsp[0]) * 8;
        break;
    }
    header.count = 1;
    header.id = preset_id*10+index;
    header.check = 0;
    UpdatePageHeaderCheckSum(&header, src, size, dict_ptr, dict_size);

    memcpy(flash_dump_data+address, (uint32_t*)&header, sizeof(header));
    if (dict_size > 0)
    {
        memcpy(flash_dump_data+address+sizeof(header), dict_ptr, dict_size);
    }
    memcpy(flash_dump_data+address+sizeof(header)+dict_size, src, size);
}

void LoadPresetById(int preset_id, ConfigMap& tmp_config_map, unsigned char* flash_dump_data)
{
	Using_Page using_page;
	GetUsingPage(&using_page, flash_dump_data);

	InitConfigMap(tmp_config_map);
	// input
	if (using_page.preset_address[preset_id-1][0] != 0)
    {
        const OlaList * input_dict = (OlaList*)(using_page.preset_address[preset_id-1][0]+sizeof(Page_Header));
        char * input_data_base = (char*)input_dict +
                sizeof(input_dict->length) +
                sizeof(input_dict->struct_length) +
                input_dict->length * sizeof(OlaInfo);
        for (int i=0;i<8;i++)
        {
            ReadIODspMem((char*)(tmp_config_map.input_dsp + i),
                input_data_base + i*input_dict->struct_length,
                input_dict,
                (OlaList*)&input_dsp_ola_list);
        }
    }
	if (using_page.preset_address[preset_id-1][1] != 0)
    {
        const OlaList * input_dict = (OlaList*)(using_page.preset_address[preset_id-1][1]+sizeof(Page_Header));
        char * input_data_base = (char*)input_dict +
                sizeof(input_dict->length) +
                sizeof(input_dict->struct_length) +
                input_dict->length * sizeof(OlaInfo);
        for (int i=0;i<8;i++)
        {
            ReadIODspMem((char*)(tmp_config_map.input_dsp + i + 8),
                input_data_base + i*input_dict->struct_length,
                input_dict,
                (OlaList*)&input_dsp_ola_list);
        }
    }
	// output
	if (using_page.preset_address[preset_id-1][2] != 0)
    {
        const OlaList * output_dict = (OlaList*)(using_page.preset_address[preset_id-1][2]+sizeof(Page_Header));
        char * output_data_base = (char*)output_dict +
                sizeof(output_dict->length) +
                sizeof(output_dict->struct_length) +
                output_dict->length * sizeof(OlaInfo);
        for (int i=0;i<8;i++)
        {
            ReadIODspMem((char*)(tmp_config_map.output_dsp + i),
                output_data_base + i*output_dict->struct_length,
                output_dict,
                (OlaList*)&output_dsp_ola_list);
        }
    }
	if (using_page.preset_address[preset_id-1][3] != 0)
    {
        const OlaList * output_dict = (OlaList*)(using_page.preset_address[preset_id-1][3]+sizeof(Page_Header));
        char * output_data_base = (char*)output_dict +
                sizeof(output_dict->length) +
                sizeof(output_dict->struct_length) +
                output_dict->length * sizeof(OlaInfo);
        for (int i=0;i<8;i++)
        {
            ReadIODspMem((char*)(tmp_config_map.output_dsp + i + 8),
                output_data_base + i*output_dict->struct_length,
                output_dict,
                (OlaList*)&output_dsp_ola_list);
        }
    }
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


const InputOlaList input_dsp_ola_list =
{
	15, 
	sizeof(InputConfigMap), 
	{
		{offsetof(InputConfigMap, eq_switch), sizeof(unsigned char), 1},
		{offsetof(InputConfigMap, comp_switch), sizeof(unsigned char), 1},
		{offsetof(InputConfigMap, auto_switch), sizeof(unsigned char), 1},
		{offsetof(InputConfigMap, invert_switch), sizeof(unsigned char), 1},
		{offsetof(InputConfigMap, noise_switch), sizeof(unsigned char), 1},
		{offsetof(InputConfigMap, mute_switch), sizeof(unsigned char), 1},
		{offsetof(InputConfigMap, phantom_switch), sizeof(unsigned char), 1},
		{offsetof(InputConfigMap, level_a), sizeof(unsigned short), 1},
		{offsetof(InputConfigMap, level_b), sizeof(unsigned short), 1},
		{offsetof(InputConfigMap, gain), sizeof(unsigned char), 1},
		{offsetof(InputConfigMap, delay), sizeof(unsigned int), 1},
		{offsetof(InputConfigMap, filter), TypeFilterConfigMap, 11},
		{offsetof(InputConfigMap, dsp_name), sizeof(char), 7},
		{offsetof(InputConfigMap, master_mute_switch), sizeof(unsigned char), 1},
		{offsetof(InputConfigMap, master_level_a), sizeof(unsigned short), 1},
	},
};

void ReadIODspMem(char * dst, char * src, const OlaList * dst_ola_list, const OlaList * src_ola_list)
{
	int i;
	int max_length = src_ola_list->length;
	
	if (src_ola_list->length < dst_ola_list->length)
	{
		max_length = dst_ola_list->length;
	}
	
	for (i=0;i<max_length;i++)
	{
		const OlaInfo src_ola_info = src_ola_list->ola_info[i];
		const OlaInfo dst_ola_info = dst_ola_list->ola_info[i];

        if (src_ola_info.offset == 0xFFFF || dst_ola_info.offset == 0xFFFF)
            continue;

		// 需要类型一致
		if (src_ola_info.type_length == dst_ola_info.type_length && src_ola_info.array_length == dst_ola_info.array_length)
		{
			if (src_ola_info.type_length == TypeFilterConfigMap)
			{
				memcpy(dst+dst_ola_info.offset, src+src_ola_info.offset, sizeof(FilterConfigMap)*dst_ola_info.array_length);
			}
			else
			{
				memcpy(dst+dst_ola_info.offset, src+src_ola_info.offset, dst_ola_info.type_length*dst_ola_info.array_length);
			}
		}
	}
};

const OutputOlaList output_dsp_ola_list =
{
	18,
    sizeof(OutputConfigMap),
	{
		{offsetof(OutputConfigMap, eq_switch), sizeof(unsigned char), 1},
		{offsetof(OutputConfigMap, comp_switch), sizeof(unsigned char), 1},
		{offsetof(OutputConfigMap, invert_switch), sizeof(unsigned char), 1},
		{offsetof(OutputConfigMap, mute_switch), sizeof(unsigned char), 1},
		{offsetof(OutputConfigMap, level_a), sizeof(unsigned short), 1},
		{offsetof(OutputConfigMap, level_b), sizeof(unsigned short), 1},
		{offsetof(OutputConfigMap, gain), sizeof(unsigned char), 1},
		{offsetof(OutputConfigMap, delay), sizeof(unsigned int), 1},
		{offsetof(OutputConfigMap, filter), TypeFilterConfigMap, 11},
		{offsetof(OutputConfigMap, dsp_name), sizeof(char), 7},

		{offsetof(OutputConfigMap, ratio), sizeof(int), 1},
		{offsetof(OutputConfigMap, threshold), sizeof(int), 1},
		{offsetof(OutputConfigMap, attack_time), sizeof(int), 1},
		{offsetof(OutputConfigMap, release_time), sizeof(int), 1},
		{offsetof(OutputConfigMap, comp_gain), sizeof(int), 1},
		{offsetof(OutputConfigMap, auto_time), sizeof(char), 1},

		{offsetof(OutputConfigMap, mix), sizeof(short), INPUT_DSP_NUM+1},
		{offsetof(OutputConfigMap, mix_mute), sizeof(unsigned char), INPUT_DSP_NUM+1},
	},
};

