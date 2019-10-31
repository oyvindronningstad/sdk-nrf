/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <bl_crypto.h>
#include "bl_crypto_internal.h"
#include <fw_info.h>
#include <kernel.h>

#ifdef CONFIG_BL_ROT_VERIFY_ABI_USE
__ext_abi_req(BL_ROT_VERIFY, 1, struct bl_rot_verify_abi, bl_rot_verify);

int bl_root_of_trust_verify(const u8_t *public_key, const u8_t *public_key_hash,
			 const u8_t *signature, const u8_t *firmware,
			 const u32_t firmware_len)
{
	return bl_rot_verify->abi.bl_root_of_trust_verify(public_key,
			public_key_hash, signature, firmware, firmware_len);
}
#endif


#ifdef CONFIG_BL_SHA256_ABI_USE
__ext_abi_req(BL_SHA256, 1, struct bl_sha256_abi, bl_sha256);

int bl_sha256_init(bl_sha256_ctx_t *ctx)
{
	if (sizeof(*ctx) < bl_sha256->abi.bl_sha256_ctx_size) {
		return -EFAULT;
	}
	return bl_sha256->abi.bl_sha256_init(ctx);
}

int bl_sha256_update(bl_sha256_ctx_t *ctx, const u8_t *data, u32_t data_len)
{
	return bl_sha256->abi.bl_sha256_update(ctx, data, data_len);
}

int bl_sha256_finalize(bl_sha256_ctx_t *ctx, u8_t *output)
{
	return bl_sha256->abi.bl_sha256_finalize(ctx, output);
}

int bl_sha256_verify(const u8_t *data, u32_t data_len, const u8_t *expected)
{
	return bl_sha256->abi.bl_sha256_verify(data, data_len, expected);
}
#endif


#ifdef CONFIG_BL_SECP256R1_ABI_USE
__ext_abi_req(BL_SECP256R1, 1, struct bl_secp256r1_abi, bl_secp256r1);

int bl_secp256r1_validate(const u8_t *hash, u32_t hash_len,
			const u8_t *public_key, const u8_t *signature)
{
	return bl_secp256r1->abi.bl_secp256r1_validate(hash, hash_len,
							public_key, signature);
}
#endif
