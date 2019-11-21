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
extern const u32_t _ext_abis_size[];
extern const u32_t _ext_abis_req_size[];
extern const u32_t _fw_info_images_start[];
extern const u32_t _fw_info_images_size[];
extern const u32_t _fw_info_size[];


__fw_info struct fw_info m_firmware_info =
{
	.magic = {FIRMWARE_INFO_MAGIC},
	.total_size = (u32_t)_fw_info_size,
	.image_size = (u32_t)&_flash_used,
	.image_version = CONFIG_FW_INFO_FIRMWARE_VERSION,
	.image_address = ((u32_t)&_image_rom_start - FW_INFO_VECTOR_OFFSET),
	.boot_address = (u32_t)&_image_rom_start,
	.valid = CONFIG_FW_INFO_VALID_VAL,
	.reserved = {0, 0, 0, 0},
	.abi_out_len = (u32_t)_ext_abis_size,
	.abi_in_len = (u32_t)_ext_abis_req_size,
};


Z_GENERIC_SECTION(.fw_info_images) __attribute__((used))
const u32_t self_image = ((u32_t)&_image_rom_start - FW_INFO_VECTOR_OFFSET);


#define ADVANCE_ABI(abi) ((abi) = ((const struct fw_info_abi *) \
			(((const u8_t *)(abi)) + (abi)->abi_len)))

const struct fw_info_abi *fw_info_abi_find(u32_t id, u32_t flags,
					u32_t min_version, u32_t max_version)
{
	for (u32_t i = 0; i < (u32_t)_fw_info_images_size; i++) {
		const struct fw_info *fw_info;

		fw_info = fw_info_find(_fw_info_images_start[i]);
		if (!fw_info || (fw_info->valid != CONFIG_FW_INFO_VALID_VAL)) {
			continue;
		}
		const struct fw_info_abi *abi = &fw_info->abis[0];

		for (u32_t j = 0; j < fw_info->abi_out_len; j++) {

			if ((abi->abi_id == id)
			&&  (abi->abi_version >= min_version)
			&&  (abi->abi_version <  max_version)
			&& ((abi->abi_flags & flags) == flags)) {
				/* Found valid abi. */
				return abi;
			}
			ADVANCE_ABI(abi);
		}
	}
	return NULL;
}


bool fw_info_abi_provide(const struct fw_info *fw_info, bool test_only)
{
	const struct fw_info_abi *abi = &fw_info->abis[0];

	for (u32_t j = 0; j < fw_info->abi_out_len; j++) {
		ADVANCE_ABI(abi);
	}

	for (u32_t i = 0; i < fw_info->abi_in_len; i++) {
		const struct fw_info_abi_request *abi_req
			= (const struct fw_info_abi_request *)abi;
		const struct fw_info_abi *new_abi;

		new_abi = fw_info_abi_find(abi_req->min_abi.abi_id,
					abi_req->min_abi.abi_flags,
					abi_req->min_abi.abi_version,
					abi_req->abi_max_version);
		if (!test_only) {
			/* Provide abi, or NULL. */
			*abi_req->abi = new_abi;
		}
		if (!new_abi && abi_req->required) {
			/* Requirement not met */
			return false;
		}
		ADVANCE_ABI(abi);
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
