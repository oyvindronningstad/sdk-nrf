#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

set(tfm_byproducts
  s_veneers.a
  )

set(TFM_DIR ${NRF_DIR}/../trusted-firmware-m)

if (CONFIG_TFM_CRYPTO)
  set(tfm_crypto_arg -DTFM_PARTITION_CRYPTO=ON)
endif()

set (platform nrf9160dk_nrf9160)

include(ExternalProject)
ExternalProject_Add(tfm_subimage
  SOURCE_DIR ${TFM_DIR}
  BINARY_DIR ${CMAKE_BINARY_DIR}/tfm
  CMAKE_ARGS -DTARGET_PLATFORM=${platform} ${tfm_crypto_arg} -DCOMPILER=GNUARM -GNinja
  BUILD_BYPRODUCTS ${tfm_byproducts}
  BUILD_ALWAYS True
  INSTALL_COMMAND ""
  )
