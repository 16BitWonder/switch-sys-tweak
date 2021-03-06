/*
 * Copyright (c) 2018 p-sam
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <switch.h>
#include "ns_srvget_mitm_service.hpp"
#include "file_utils.hpp"
#include "ini.h"

static int _ProcessControlDataIniHandler(void* user, const char* section, const char* name, const char* value) {
	NsAppControlData* data = (NsAppControlData*)user;

	if (strcasecmp(section, "override_nacp") == 0) {
		if (strcasecmp(name, "name") == 0) {
			for(unsigned int i = 0; i < sizeof(data->nacp.lang_entries) / sizeof(data->nacp.lang_entries[0]); i++) {
				strncpy(data->nacp.lang_entries[i].name, value, sizeof(data->nacp.lang_entries[i].name)-1);
			}
		} else if (strcasecmp(name, "author") == 0) {
			for(unsigned int i = 0; i < sizeof(data->nacp.lang_entries) / sizeof(data->nacp.lang_entries[0]); i++) {
				strncpy(data->nacp.lang_entries[i].author, value, sizeof(data->nacp.lang_entries[i].author)-1);
			}
		} else if (strcasecmp(name, "display_version") == 0) {
			strncpy(data->nacp.display_version, value, sizeof(data->nacp.display_version)-1);
		} else if (strcasecmp(name, "startup_user_account") == 0) {
			data->nacp.startup_user_account = (*value == 't' || *value == '1');
		}
	}

	return 1;
}

static void _ProcessControlData(u64 tid, NsAppControlData* data, u64* size) {
	char path[50] = {0};
	snprintf(path, sizeof(path)-1, "/atmosphere/contents/%016lx/config.ini", tid);
	ini_parse(path, _ProcessControlDataIniHandler, data);
	snprintf(path, sizeof(path)-1, "/atmosphere/contents/%016lx/icon.jpg", tid);
	
	FILE* f = fopen(path, "rb");
	if(f != NULL) {
		fread(&data->icon[0], sizeof(data->icon), 1, f);
		*size = sizeof(data->nacp) + ftell(f);
		fclose(f);
	}

	FileUtils::LogLine("_ProcessControlData(%016lx) // [%ld|%s] %s", tid, *size, f ? "loaded" : "failed", path);
}

bool NsAm2MitmService::ShouldMitm(const ams::sm::MitmProcessInfo& client_info) {
	bool should_mitm = (client_info.program_id == ams::ncm::SystemAppletId::Qlaunch);
	FILE_LOG_IPC(NSAM2_MITM_SERVICE_NAME, client_info, "() // %s", should_mitm ? "true" : "false");
	return should_mitm;
}

bool NsRoMitmService::ShouldMitm(const ams::sm::MitmProcessInfo& client_info) {
	bool should_mitm = (client_info.program_id == ams::ncm::SystemProgramId::Ppc);
	FILE_LOG_IPC(NSRO_MITM_SERVICE_NAME, client_info, "() // %s", should_mitm ? "true" : "false");
	return should_mitm;
}

ams::Result NsServiceGetterMitmService::GetROAppControlDataInterface(ams::sf::Out<ams::sf::SharedPointer<NsROAppControlDataInterface>> out) {
	Service s;
	Result rc = serviceDispatch(this->forward_service.get(), (u32)NsSrvGetterCmdId::GetROAppControlDataInterface,
		.out_num_objects = 1,
		.out_objects = &s,
	);

	if(R_SUCCEEDED(rc)) {
		const ams::sf::cmif::DomainObjectId target_object_id{serviceGetObjectId(&s)};
		out.SetValue(ams::sf::CreateSharedObjectEmplaced<NsROAppControlDataInterface, NsROAppControlDataService>(this->client_info, std::make_unique<Service>(s)), target_object_id);
	}

	FILE_LOG_IPC_CLASS("() // %x", rc);
	return rc;
}

ams::Result NsROAppControlDataService::GetAppControlData(u8 flag, u64 tid, const ams::sf::OutBuffer &buffer, ams::sf::Out<u64> out_size) {
	const struct {
		u8 flag;
		u64 tid;
	} in = {flag, tid};

	Result rc = serviceDispatchInOut(this->srv.get(), NsROAppControlDataInterfaceCmdId::GetAppControlData, in, *out_size.GetPointer(),
		.buffer_attrs = {SfBufferAttr_HipcMapAlias | SfBufferAttr_Out},
		.buffers = {{buffer.GetPointer(), buffer.GetSize()}},
	);

	FILE_LOG_IPC_CLASS("(%u, 0x%016lx, buf[0x%lx]) // %x[0x%lx]",  flag, tid, buffer.GetSize(), rc, out_size.GetValue());

	if(R_SUCCEEDED(rc) && buffer.GetSize() >= sizeof(NsAppControlData) && FileUtils::WaitInitialized()) {
		_ProcessControlData(tid, (NsAppControlData*)buffer.GetPointer(), out_size.GetPointer());
	}
	return rc;
}

ams::Result NsROAppControlDataService::GetAppDesiredLanguage(u32 bitmask, ams::sf::Out<u8> out_langentry) {
	Result rc = serviceDispatchInOut(this->srv.get(), NsROAppControlDataInterfaceCmdId::GetAppDesiredLanguage, bitmask, *out_langentry.GetPointer());
	FILE_LOG_IPC_CLASS("(0x%08x) // %x[%u]", bitmask, rc, out_langentry.GetValue());
	return rc;
}

ams::Result NsROAppControlDataService::ConvertAppLanguageToLanguageCode(u8 langentry, ams::sf::Out<u64> out_langcode) {
	Result rc = serviceDispatchInOut(this->srv.get(), NsROAppControlDataInterfaceCmdId::ConvertAppLanguageToLanguageCode, langentry, *out_langcode.GetPointer());
	FILE_LOG_IPC_CLASS("(0x%02x) // %x[%u]", langentry, rc, out_langcode.GetValue());
	return rc;
}

ams::Result NsROAppControlDataService::ConvertLanguageCodeToAppLanguage(u64 langcode, ams::sf::Out<u8> out_langentry) {
	Result rc = serviceDispatchInOut(this->srv.get(), NsROAppControlDataInterfaceCmdId::ConvertLanguageCodeToAppLanguage, langcode, *out_langentry.GetPointer());
	FILE_LOG_IPC_CLASS("(0x%016lx); // %x[0x%02x]", langcode, rc, out_langentry.GetValue());
	return rc;
}
