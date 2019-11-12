/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef FW_INFO_H__
#define FW_INFO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/types.h>
#include <stddef.h>
#include <toolchain.h>
#include <sys/util.h>
#include <sys/__assert.h>
#include <string.h>
#if USE_PARTITION_MANAGER
#include <pm_config.h>
#endif

/** @defgroup fw_info Firmware info structure
 * @{
 */

#define MAGIC_LEN_WORDS (CONFIG_FW_INFO_MAGIC_LEN / sizeof(u32_t))



/**
 * This struct is used to request an ABI. The bootloader will populate the
 * `abi` pointer with a pointer to an ABI in another image. An ABI fulfills a
 * request if the ID matches, all flags in the request are set in the ABI, and
 * the version falls between the minimum and maximum (inclusive). If `required`
 * is true the bootloader will refuse to boot an image if it cannot find a
 * requested ABI.
 */
struct __packed fw_info_abi_request {
	/* The id of the ABI. */
	u32_t abi_id;

	/* Flags specifying properties of the ABI. */
	u32_t abi_flags;

	/* The minimum version accepted. */
	u32_t abi_min_version;

	/* The maximum version accepted. */
	u32_t abi_max_version;

	/* The ABI is required. */
	u32_t required;

	/* Where to place a pointer to the ABI. */
	const struct fw_info_abi **abi;
};


/**
 * This is a data structure that is placed at a specific offset inside a
 * firmware image so it can be consistently read by external parties. The
 * specific offset makes it easy to find, and the magic value at the start
 * guarantees that it contains data of a specific format.
 */
struct __packed fw_info {
	/* Magic value to verify that the struct has the correct format. */
	u32_t magic[MAGIC_LEN_WORDS];

	/* Size of the firmware image code. */
	u32_t firmware_size;

	/* Monotonically increasing version counter.*/
	u32_t firmware_version;

	/* The address of the start (vector table) of the firmware. */
	u32_t firmware_address;

	/* Value that can be modified to invalidate the firmware. Has the value
	 * CONFIG_FW_INFO_VALID_VAL when valid.
	 */
	u32_t valid;

	/* This firmware's ABIs. */
	u32_t abi_out_len;
	const struct fw_info_abi *const *abi_out;

	/* Where to place the getter for the ABI provided to this firmware. */
	u32_t abi_in_len;
	const struct fw_info_abi_request *abi_in;

	/* Reserved values (set to 0) */
	u32_t reserved00[4];
};

/** @cond
 *  Remove from doc building.
 */
#define OFFSET_CHECK(type, member, value) \
		BUILD_ASSERT_MSG(offsetof(type, member) == value, \
				#member " has wrong offset")

/* Static asserts to ensure compatibility */
OFFSET_CHECK(struct fw_info, magic, 0);
OFFSET_CHECK(struct fw_info, firmware_size, 12);
OFFSET_CHECK(struct fw_info, firmware_version, 16);
OFFSET_CHECK(struct fw_info, firmware_address, 20);
OFFSET_CHECK(struct fw_info, valid, 24);
OFFSET_CHECK(struct fw_info, abi_out_len, 28);
OFFSET_CHECK(struct fw_info, abi_out, 32);
OFFSET_CHECK(struct fw_info, abi_in_len, 36);
OFFSET_CHECK(struct fw_info, abi_in, 40);
OFFSET_CHECK(struct fw_info, reserved00, 44);

OFFSET_CHECK(struct fw_info_abi_request, abi_id, 0);
OFFSET_CHECK(struct fw_info_abi_request, abi_flags, 4);
OFFSET_CHECK(struct fw_info_abi_request, abi_min_version, 8);
OFFSET_CHECK(struct fw_info_abi_request, abi_max_version, 12);
OFFSET_CHECK(struct fw_info_abi_request, required, 16);
OFFSET_CHECK(struct fw_info_abi_request, abi, 20);
/** @endcond
 */

/* For declaring this firmware's firmware info. */
#define __fw_info Z_GENERIC_SECTION(.firmware_info) __attribute__((used)) const

/**
 * This struct is meant to serve as a header before a list of function pointers
 * (or something else) that constitute the actual ABI. How to use the ABI, such
 * as the signatures of all the functions in the list must be unambiguous for an
 * ID/version combination.
 */
struct __packed fw_info_abi {
	/* The length of this header plus everything after this header. Must be
	 * word-aligned.
	 */
	u32_t abi_len;

	/* The id of the ABI. */
	u32_t abi_id;

	/* Flags specifying properties of the ABI. */
	u32_t abi_flags;

	/* The version of this ABI. */
	u32_t abi_version;
};


#define __ext_abi(type, name) \
	BUILD_ASSERT_MSG((sizeof(type) % 4) == 0, \
			"ext_abi " #type " is not word-aligned"); \
	extern const type name; \
	Z_GENERIC_SECTION(.ext_abis) __attribute__((used)) \
	const type * const _CONCAT(name, _ptr) = &name; \
	__attribute__((used)) \
	const type name



#define FW_INFO_ABI_INIT(id, flags, version, total_size) \
	{ \
		.abi_len = total_size, \
		.abi_id = id, \
		.abi_flags = flags, \
		.abi_version = version, \
	}

#define __ext_abi_req(abi_name, req, type, name) \
	__noinit static const type *name; \
	Z_GENERIC_SECTION(.ext_abis_req) \
	__attribute__((used)) \
	const struct fw_info_abi_request _CONCAT(name, _req) = \
	{ \
		.abi_id = abi_name ## _ABI_ID, \
		.abi_flags = CONFIG_ ## abi_name ## _ABI_FLAGS, \
		.abi_min_version = CONFIG_ ## abi_name ## _ABI_VER, \
		.abi_max_version = CONFIG_ ## abi_name ## _ABI_MAX_VER, \
		.required = req, \
		.abi = (void *) &name, \
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

/* Check and provide a pointer to a firmware_info structure.
 *
 * @return pointer if valid, NULL if not.
 */
static inline const struct fw_info *fw_info_check(u32_t fw_info_addr)
{
	const struct fw_info *finfo;
	const u32_t fw_info_magic[] = {FIRMWARE_INFO_MAGIC};

	finfo = (const struct fw_info *)(fw_info_addr);
	if (memeq(finfo->magic, fw_info_magic, CONFIG_FW_INFO_MAGIC_LEN)) {
		return finfo;
	}
	return NULL;
}


/* The supported offsets for the fw_info struct. */
#define FW_INFO_OFFSET0 0x0
#define FW_INFO_OFFSET1 0x200
#define FW_INFO_OFFSET2 0x400
#define FW_INFO_OFFSET3 0x800
#define FW_INFO_OFFSET4 0x1000
#define FW_INFO_OFFSET_COUNT 5

/* Find the difference between the start of the current image and the address
 * from which the firmware info offset is calculated.
 */
#if defined(PM_S0_PAD_SIZE) && (PM_ADDRESS == PM_S0_IMAGE_ADDRESS)
	#define VECTOR_OFFSET PM_S0_PAD_SIZE
#elif defined(PM_S1_PAD_SIZE) && (PM_ADDRESS == PM_S1_IMAGE_ADDRESS)
	#define VECTOR_OFFSET PM_S1_PAD_SIZE
#elif defined(PM_MCUBOOT_PAD_SIZE) && \
		(PM_ADDRESS == PM_MCUBOOT_PRIMARY_APP_ADDRESS)
	#define VECTOR_OFFSET PM_MCUBOOT_PAD_SIZE
#else
	#define VECTOR_OFFSET 0
#endif

#define CURRENT_OFFSET (CONFIG_FW_INFO_OFFSET + VECTOR_OFFSET)

static const u32_t fw_info_allowed_offsets[] = {
					FW_INFO_OFFSET0, FW_INFO_OFFSET1,
					FW_INFO_OFFSET2, FW_INFO_OFFSET3,
					FW_INFO_OFFSET4};

/** @cond
 *  Remove from doc building.
 */
BUILD_ASSERT_MSG(ARRAY_SIZE(fw_info_allowed_offsets) == FW_INFO_OFFSET_COUNT,
		"Mismatch in the number of allowed offsets.");
/** @endcond
 */

#if (FW_INFO_OFFSET_COUNT != 5) || ((CURRENT_OFFSET) != (FW_INFO_OFFSET0) && \
				(CURRENT_OFFSET) != (FW_INFO_OFFSET1) && \
				(CURRENT_OFFSET) != (FW_INFO_OFFSET2) && \
				(CURRENT_OFFSET) != (FW_INFO_OFFSET3) && \
				(CURRENT_OFFSET) != (FW_INFO_OFFSET4))
	#error FW_INFO_OFFSET not set to one of the allowed values.
#endif

/* Search for the firmware_info structure inside the firmware. */
static inline const struct fw_info *fw_info_find(u32_t firmware_address)
{
	const struct fw_info *finfo;

	for (u32_t i = 0; i < FW_INFO_OFFSET_COUNT; i++) {
		finfo = fw_info_check(firmware_address +
						fw_info_allowed_offsets[i]);
		if (finfo) {
			return finfo;
		}
	}
	return NULL;
}


/**Expose ABIs to another firmware
 *
 * Populate the other firmware's @c abi_in with ABIs from other images.
 *
 * @note This is should be called immediately before booting the other firmware
 *       since it will likely corrupt the memory of the running firmware.
 *
 * @param[in]  fw_info    Pointer to the other firmware's information structure.
 * @param[in]  test_only  Don't populate abi_in.
 *
 * @return Whether requirements could be satisified.
 */
bool fw_info_abi_provide(const struct fw_info *fwinfo, bool test_only);

/**Get a single ABI.
 *
 * @param[in]    id      Which ABI to get.
 * @param[in]    index   If there are multiple ABIs available with the same ID,
 *                       retrieve the different ones with this.
 *
 * @return The ABI, or NULL, if it wasn't found.
 */
const struct fw_info_abi *fw_info_abi_get(u32_t id, u32_t index);

/**Find an ABI based on a version range.
 *
 * @param[in]  id           The ID of the ABI to find.
 * @param[in]  flags        The required flags of the ABI to find. The returned
 *                          ABI may have other flags set as well.
 * @param[in]  min_version  The minimum acceptable ABI version.
 * @param[in]  max_version  One more than the maximum acceptable ABI version.
 *
 * @return The ABI, or NULL if none was found.
 */
const struct fw_info_abi *fw_info_abi_find(u32_t id, u32_t flags,
					u32_t min_version, u32_t max_version);


/**Invalidate this image by setting the @c valid value to 0x0.
 *
 * @note This function needs to have CONFIG_NRF_NVMC enabled.
 *
 * @param[in]  fw_info  The info structure to modify.
 *                      This memory will be modified directly in flash.
 */
void fw_info_invalidate(const struct fw_info *fw_info);

  /** @} */

#ifdef __cplusplus
}
#endif

#endif
