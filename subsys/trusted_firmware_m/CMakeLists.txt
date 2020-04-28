#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

set(tfm_byproducts
  tfm/s_veneers.a
  )

set(TFM_DIR ${NRF_DIR}/../trusted-firmware-m)

if (CONFIG_TFM_CRYPTO)
  set(tfm_crypto_arg -DTFM_PARTITION_CRYPTO=ON)
  set(TFM_PARTITION_CRYPTO ON)
else()
  set(TFM_PARTITION_CRYPTO OFF)
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

set(SECURE_FW OFF)
set(BL2 OFF)
set(NS_APP ON)
set(APP_LIB_ONLY ON)
set(TARGET_PLATFORM ${platform})
set(COMPILER GNUARM)
set(S_VENEER_PATH ${CMAKE_CURRENT_BINARY_DIR}/tfm)
add_subdirectory(${NRF_DIR}/../trusted-firmware-m ${CMAKE_CURRENT_BINARY_DIR}/tfm)
