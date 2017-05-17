//---------------------------------------------------------------------------

#ifndef untFlashReaderH
#define untFlashReaderH

extern "C"{
#include "../enc28j60_iap_app/inc/D1608Pack.h"
}
//---------------------------------------------------------------------------

void LoadPresetById(int preset_id, ConfigMap& tmp_config_map, unsigned char* flash_dump_data);
void LoadGlobalConfig(GlobalConfig& global_config, unsigned char* flash_dump_data);

#endif
