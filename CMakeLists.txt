cmake_minimum_required(VERSION 3.13)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(my_project CXX C ASM)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

add_subdirectory(nec_transmit_library)
add_subdirectory(nec_receive_library)

add_executable(main
    main.c
)

# pico_set_binary_type(main no_flash)

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(main
    pico_stdlib
    pico_unique_id
    hardware_spi
    hardware_pio
    nec_transmit_library
    nec_receive_library
)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(main)
