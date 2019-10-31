
. = ALIGN(4);
_fw_info_images_start = .;
KEEP(*(.fw_info_images))
_fw_info_images_size = ABSOLUTE((. - _fw_info_images_start) / 4);

. = ALIGN(4);
_ext_abis_start = .;
KEEP(*(.ext_abis))
_ext_abis_elem_size = ABSOLUTE(24);
_ext_abis_size = ABSOLUTE((. - _ext_abis_start) / 4);

. = ALIGN(4);
_ext_abis_req_start = .;
KEEP(*(.ext_abis_req))
_ext_abis_req_elem_size = ABSOLUTE(24);
_ext_abis_req_size = ABSOLUTE((. - _ext_abis_req_start) / _ext_abis_req_elem_size);
