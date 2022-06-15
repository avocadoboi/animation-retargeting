# Toolchain file for GCC and Clang.

add_compile_options(
	-Wall 
	-Wpedantic 
	-Wextra 
	-Wduplicated-branches 
	-Wduplicated-cond 
	-Wcast-qual 
	-Wcast-align
	-Wconversion

	-Wno-parentheses
	-Wno-missing-field-initializers
)

add_compile_options(
	-fmax-errors=5
)

if (DEFINED ENV{VCPKG_ROOT})
	include($ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake)
endif ()