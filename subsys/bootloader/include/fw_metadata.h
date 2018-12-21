/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef FW_METADATA_H__
#define FW_METADATA_H__

/*
 * The package will consist of (firmware | (padding) | validation_info),
 * where the firmware contains the firmware_info at a predefined location. The
 * padding is present if the validation_info needs alignment. The
 * validation_info is not directly referenced from the firmware_info since the
 * validation_info doesn't actually have to be placed after the firmware.
 *
 * Putting the firmware info inside the firmware instead of in front of it
 * removes the need to consider the padding before the vector table of the
 * firmware. It will also likely make it easier to add all the info at compile
 * time.
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <toolchain.h>
#include <assert.h>
#include <string.h>
#include <debug.h>

#define MAGIC_LEN_WORDS (CONFIG_SB_MAGIC_LEN / sizeof(u32_t))

struct fw_abi_info;

/**@brief Function that returns an ABI.
 *
 * @param[in]    id      Which ABI to get.
 * @param[in]    index   If there are multiple ABIs available with the same ID,
 *                       retrieve the different ones with this.
 * @param[out]   abi     Pointer to the abi with the given id and index.
 *
 * @retval 0        Success.
 * @retval -ENOENT  id not found.
 * @retval -EBADF   index too large.
 * @retval -EFAULT  abi was NULL.
 */
typedef int (*fw_abi_getter)(u32_t id, u32_t index,
				const struct fw_abi_info **abi);

struct __packed fw_firmware_info {
	/* Magic value to verify that the struct has the correct type. */
	u32_t magic[MAGIC_LEN_WORDS];

	/* Size without validation_info pointer and padding. */
	u32_t firmware_size;

	/* Monotonically increasing version counter.*/
	u32_t firmware_version;

	/* The address of the start (vector table) of the firmware. */
	u32_t firmware_address;

	/* Where to place the getter for the ABI provided to this firmware. */
	fw_abi_getter *abi_in;

	/* This firmware's ABI getter. */
	const fw_abi_getter abi_out;
};


#define OFFSET_CHECK(type, member, value) \
		BUILD_ASSERT_MSG(offsetof(type, member) == value, \
				#member " has wrong offset")

/* Static asserts to ensure compatibility */
OFFSET_CHECK(struct fw_firmware_info, magic, 0);
OFFSET_CHECK(struct fw_firmware_info, firmware_size, CONFIG_SB_MAGIC_LEN);
OFFSET_CHECK(struct fw_firmware_info, firmware_version,
	(CONFIG_SB_MAGIC_LEN + 4));
OFFSET_CHECK(struct fw_firmware_info, firmware_address,
	(CONFIG_SB_MAGIC_LEN + 8));

/* For declaring this firmware's firmware info. */
#define __fw_info Z_GENERIC_SECTION(.firmware_info) __attribute__((used)) const

struct __packed fw_validation_info {
	/* Magic value to verify that the struct has the correct type. */
	u32_t magic[MAGIC_LEN_WORDS];

	/* The address of the start (vector table) of the firmware. */
	u32_t firmware_address;

	/* The hash of the firmware.*/
	u8_t  firmware_hash[CONFIG_SB_HASH_LEN];

	/* Public key to be used for signature verification. This must be
	 * checked against a trusted hash.
	 */
	u8_t  public_key[CONFIG_SB_PUBLIC_KEY_LEN];

	/* Signature over the firmware as represented by the firmware_address
	 * and firmware_size in the firmware_info.
	 */
	u8_t  signature[CONFIG_SB_SIGNATURE_LEN];
};


/* Static asserts to ensure compatibility */
OFFSET_CHECK(struct fw_validation_info, magic, 0);
OFFSET_CHECK(struct fw_validation_info, firmware_address, CONFIG_SB_MAGIC_LEN);
OFFSET_CHECK(struct fw_validation_info, firmware_hash,
	(CONFIG_SB_MAGIC_LEN + 4));
OFFSET_CHECK(struct fw_validation_info, public_key,
	(CONFIG_SB_MAGIC_LEN + 4 + CONFIG_SB_HASH_LEN));
OFFSET_CHECK(struct fw_validation_info, signature,
	(CONFIG_SB_MAGIC_LEN + 4 + CONFIG_SB_HASH_LEN
	+ CONFIG_SB_SIGNATURE_LEN));

/* Can be used to make the firmware discoverable in other locations, e.g. when
 * searching backwards. This struct would typically be constructed locally, so
 * it needs no version.
 */
struct __packed fw_validation_pointer {
	u32_t magic[MAGIC_LEN_WORDS];
	const struct fw_validation_info *validation_info;
};

/* Static asserts to ensure compatibility */
OFFSET_CHECK(struct fw_validation_pointer, magic, 0);
OFFSET_CHECK(struct fw_validation_pointer, validation_info,
	CONFIG_SB_MAGIC_LEN);


/* This struct is meant to serve as a header before a list of function pointers
 * (or something else) that constitute the actual ABI. How to use the ABI, such
 * as the signatures of all the functions in the list must be unambiguous for an
 * ID/version combination.
 */
struct __packed fw_abi_info {
	/* Magic value to verify that the struct has the correct type. */
	u32_t magic[MAGIC_LEN_WORDS];

	/* The id of the ABI. */
	u32_t abi_id;

	/* Flags specifying properties of the ABI. */
	u32_t abi_flags;

	/* The version of this ABI. */
	u32_t abi_version;

	/* The length of this header plus everything after this header. Must be
	 * word-aligned.
	 */
	u32_t abi_len;
};


#define OFFSET_CHECK_EXT_ABI(type, member, value) \
	BUILD_ASSERT_MSG(offsetof(type, header.member) == value, \
		"ext_abi " #type " has wrong offset for header." #member)

#define __ext_abi(type, name) \
	OFFSET_CHECK_EXT_ABI(type, magic, 0); \
	OFFSET_CHECK_EXT_ABI(type, abi_id, CONFIG_SB_MAGIC_LEN); \
	OFFSET_CHECK_EXT_ABI(type, abi_flags, (CONFIG_SB_MAGIC_LEN + 4)); \
	OFFSET_CHECK_EXT_ABI(type, abi_version, (CONFIG_SB_MAGIC_LEN + 8)); \
	OFFSET_CHECK_EXT_ABI(type, abi_len, (CONFIG_SB_MAGIC_LEN + 12)); \
	BUILD_ASSERT_MSG((sizeof(type) % 4) == 0, \
			"ext_abi " #type " is not word-aligned"); \
	extern const type name; \
	Z_GENERIC_SECTION(.ext_abis) __attribute__((used)) \
	const type * const _CONCAT(name, _ptr) = &name; \
	__attribute__((used)) \
	const type name



#define ABI_INFO_INIT(id, flags, version, total_size) \
	{ \
		.magic = {ABI_INFO_MAGIC}, \
		.abi_id = id, \
		.abi_flags = flags, \
		.abi_version = version, \
		.abi_len = total_size, \
	}

/* Shorthand for declaring function that will be exposed through an ext_abi.
 * This will define a function pointer type as well as declare the function.
 */
#define EXT_ABI_FUNCTION(retval, name, ...) \
	typedef retval (*name ## _t) (__VA_ARGS__); \
	retval name (__VA_ARGS__)


/* All parameters must be word-aligned */
static inline bool memeq_32(const void *expected, const void *actual, u32_t len)
{
	__ASSERT(!((u32_t)expected % 4)
	      && !((u32_t)actual % 4)
	      && !((u32_t)len % 4),
		"A parameter is unaligned.");
	const u32_t *expected_32 = (const u32_t *) expected;
	const u32_t *actual_32   = (const u32_t *) actual;

	for (u32_t i = 0; i < (len / sizeof(u32_t)); i++) {
		if (expected_32[i] != actual_32[i]) {
			return false;
		}
	}
	return true;
}

static inline bool memeq_8(const void *expected, const void *actual, u32_t len)
{
	const u8_t *expected_8 = (const u8_t *) expected;
	const u8_t *actual_8   = (const u8_t *) actual;

	for (u32_t i = 0; i < len; i++) {
		if (expected_8[i] != actual_8[i]) {
			return false;
		}
	}
	return true;
}

static inline bool memeq(const void *expected, const void *actual, u32_t len)
{
	if (((u32_t)expected % 4) || ((u32_t)actual % 4) || ((u32_t)len % 4)) {
		/* Parameters are not word aligned. */
		return memeq_8(expected, actual, len);
	} else {
		return memeq_32(expected, actual, len);
	}
}

/* Get a pointer to the firmware_info structure inside the firmware. */
static inline const struct fw_firmware_info *
firmware_info_get(u32_t firmware_address)
{
	u32_t finfo_addr = firmware_address + CONFIG_SB_FIRMWARE_INFO_OFFSET;
	const struct fw_firmware_info *finfo;
	const u32_t firmware_info_magic[] = {FIRMWARE_INFO_MAGIC};

	finfo = (const struct fw_firmware_info *)(finfo_addr);
	if (memeq(finfo->magic, firmware_info_magic, CONFIG_SB_MAGIC_LEN)) {
		return finfo;
	}
	return NULL;
}

/* Find the validation_info at the end of the firmware. */
static inline const struct fw_validation_info *
validation_info_find(const struct fw_firmware_info *finfo,
			u32_t search_distance)
{
	u32_t vinfo_addr = finfo->firmware_address + finfo->firmware_size;
	const struct fw_validation_info *vinfo;
	const u32_t validation_info_magic[] = {VALIDATION_INFO_MAGIC};

	for (int i = 0; i <= search_distance; i++) {
		vinfo = (const struct fw_validation_info *)(vinfo_addr + i);
		if (memeq(vinfo->magic, validation_info_magic,
					CONFIG_SB_MAGIC_LEN)) {
			return vinfo;
		}
	}
	return NULL;
}


/* Check a fw_abi_info pointer. */
static inline bool abi_info_check(const struct fw_abi_info *abi_info)
{
	const u32_t abi_info_magic[] = {ABI_INFO_MAGIC};
	return(memeq(abi_info->magic, abi_info_magic, CONFIG_SB_MAGIC_LEN));
}


/* Expose ABIs to the firmware at this address. This is meant to be called
 * immediately before booting the aforementioned firmware since it will likely
 * corrupt the memory of the running firmware.
 */
void abi_provide(u32_t address);

/* Get a single ABI.
 *
 * @param[in]    id      Which ABI to get.
 * @param[in]    index   If there are multiple ABIs available with the same ID,
 *                       retrieve the different ones with this.
 *
 * @return The ABI, or NULL, if it wasn't found.
 */
const struct fw_abi_info *abi_get(u32_t id, u32_t index);

/* Find an ABI based on a version range.
 *
 * @param[in]  id           The ID of the ABI to find.
 * @param[in]  flags        The required flags of the ABI to find. The returned
 *                          ABI may have other flags set as well.
 * @param[in]  min_version  The minimum acceptable ABI version.
 * @param[in]  max_version  One more than the maximum acceptable ABI version.
 *
 * @return The ABI, or NULL if none was found.
 */
const struct fw_abi_info *abi_find(u32_t id, u32_t flags, u32_t min_version,
					u32_t max_version);

#endif
