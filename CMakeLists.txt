cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

project(veludda C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the SDK
pico_sdk_init()

add_executable(veludda
	bmpdata.c
	graphlib.c
	musicdata.c
	rotatevideo.c
	veludda_chars.c
	veludda.c
	ff.C
	pico-sdmm.c
	usbkeyboard.c
	hid_virtual_keytable.c
)

# set compile definitions
add_compile_definitions(PICO_CLOCK_ADJUST_PERI_CLOCK_WITH_SYS_CLOCK=1)

target_include_directories(veludda PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
)

# Pull in basic dependencies
target_link_libraries(veludda pico_stdlib hardware_spi hardware_pwm hardware_dma tinyusb_host tinyusb_board pico_multicore)

# create map/bin/hex file etc.
pico_add_extra_outputs(veludda)
