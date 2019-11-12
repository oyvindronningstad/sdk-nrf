/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "fw_info.h"
#include <linker/sections.h>
#include <sys/util.h>
#include <errno.h>
#include <string.h>
#include <nrfx_nvmc.h>


extern const u32_t _image_rom_start;
extern const u32_t _flash_used;
extern const struct fw_info _firmware_info_start[];
extern const struct fw_info_abi * const _ext_abis_start[];
extern const u32_t _ext_abis_size[];
extern const struct fw_info_abi_request _ext_abis_req_start[];
extern const u32_t _ext_abis_req_size[];
extern const u32_t _ext_abis_req_elem_size[];
extern const u32_t _fw_info_images_start[];
extern const u32_t _fw_info_images_size[];


__fw_info struct fw_info m_firmware_info =
{
	.magic = {FIRMWARE_INFO_MAGIC},
	.firmware_size = (u32_t)&_flash_used,
	.firmware_version = CONFIG_FW_INFO_FIRMWARE_VERSION,
	.firmware_address = (u32_t)&_image_rom_start,
	.valid = CONFIG_FW_INFO_VALID_VAL,
	.abi_out_len = (u32_t)_ext_abis_size,
	.abi_out = _ext_abis_start,
	.abi_in_len = (u32_t)_ext_abis_req_size,
	.abi_in = _ext_abis_req_start,
	.reserved00 = {0, 0, 0, 0},
};


Z_GENERIC_SECTION(.fw_info_images) __attribute__((used))
const u32_t self_image = ((u32_t)&_image_rom_start - VECTOR_OFFSET);


const struct fw_info_abi *fw_info_abi_find(u32_t id, u32_t flags,
					u32_t min_version, u32_t max_version)
{
	for (u32_t i = 0; i < (u32_t)_fw_info_images_size; i++) {
		const struct fw_info *fw_info;

		fw_info = fw_info_find(_fw_info_images_start[i]);
		if (!fw_info || (fw_info->valid != CONFIG_FW_INFO_VALID_VAL)) {
			continue;
		}
		for (u32_t j = 0; j < fw_info->abi_out_len; j++) {
			const struct fw_info_abi *abi = fw_info->abi_out[j];

			if ((abi->abi_id == id)
			&&  (abi->abi_version >= min_version)
			&&  (abi->abi_version <  max_version)
			&& ((abi->abi_flags & flags) == flags)) {
				/* Found valid abi. */
				return abi;
			}
		}
	}
	return NULL;
}


bool fw_info_abi_provide(const struct fw_info *fw_info, bool test_only)
{
	__ASSERT((u32_t)_ext_abis_req_elem_size
			== sizeof(_ext_abis_req_start[0]),
		"Element size not correct. See abis.ld.");

	for (u32_t i = 0; i < fw_info->abi_in_len; i++) {
		const struct fw_info_abi_request *abi_req = &fw_info->abi_in[i];
		const struct fw_info_abi *abi;

		abi = fw_info_abi_find(abi_req->abi_id, abi_req->abi_flags,
			abi_req->abi_min_version, abi_req->abi_max_version);
		if (!test_only) {
			/* Provide abi, or NULL. */
			*abi_req->abi = abi;
		}
		if (!abi && abi_req->required) {
			/* Requirement not met */
			return false;
		}
	}
	return true;
}


#define INVALID_VAL 0xFFFF0000

#ifdef CONFIG_NRFX_NVMC
void fw_info_invalidate(const struct fw_info *fw_info)
{
	/* Check if value has been written. */
	if (fw_info->valid == CONFIG_FW_INFO_VALID_VAL) {
		nrfx_nvmc_word_write((u32_t)&(fw_info->valid), INVALID_VAL);
	}
}
#endif
