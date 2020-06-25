/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <ztest.h>

void test_ff_key(void)
{
	zassert_true(false, "Should not have booted.");
}

void test_main(void)
{
	ztest_test_suite(test_bl_crypto_ff_key,
			 ztest_unit_test(test_ff_key)
	);
	ztest_run_test_suite(test_bl_crypto_ff_key);
}
