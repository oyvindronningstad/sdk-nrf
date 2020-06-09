/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/** @file dfu_target_stream.h
 *
 * @defgroup dfu_target_stream MCUBoot DFU Target
 * @{
 * @brief DFU Target for upgrades performed by MCUBoot
 */

#ifndef DFU_TARGET_STREAM_H__
#define DFU_TARGET_STREAM_H__

#include <stddef.h>
#include <storage/stream_flash.h>

#ifdef __cplusplus
extern "C" {
#endif

struct stream_flash_ctx *dfu_target_stream_get_stream(void);

/**
 * @brief Initialize dfu target, perform steps necessary to write flash stream.
 *
 * @param[in] file_size Size of the current file being downloaded.
 *
 * @retval Non-negative value if successful, negative errno otherwise.
 */
int dfu_target_stream_init(const char *id, const struct device *fdev,
			   uint8_t *buf, size_t len, size_t offset, size_t size,
			   stream_flash_callback_t cb);

/**
 * @brief Get offset of stream
 *
 * @param[out] offset Returns the offset of the firmware upgrade.
 *
 * @return Non-negative value if success, otherwise negative value if unable
 *         to get the offset
 */
int dfu_target_stream_offset_get(size_t *offset);

/**
 * @brief Write firmware data.
 *
 * @param[in] buf Pointer to data that should be written.
 * @param[in] len Length of data to write.
 *
 * @return Non-negtive value on success, negative errno otherwise.
 */
int dfu_target_stream_write(const void *const buf, size_t len);

/**
 * @brief Deinitialize resources and finalize stream flash write if successful.

 * @param[in] successful Indicate whether the firmware was successfully recived.
 *
 * @return Non-negative value on success, negative errno otherwise.
 */
int dfu_target_stream_done(bool successful);

#endif /* DFU_TARGET_STREAM_H__ */

/**@} */
