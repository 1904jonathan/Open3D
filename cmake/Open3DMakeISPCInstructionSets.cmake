# open3d_make_ispc_instruction_sets(<ispc_isas>)
#
# Sets up ISPC instruction sets based on the following precedence rules
# and stores them into the <ispc_isas> variable.
#   1. User-defined architectures
#   2. All common architectures
function(open3d_make_ispc_instruction_sets ispc_isas)
    unset(${ispc_isas})

    if(CMAKE_ISPC_INSTRUCTION_SETS)
        set(${ispc_isas} ${CMAKE_ISPC_INSTRUCTION_SETS})
        message(STATUS "Building with user-provided instruction sets")
    else()
        # ISAs are defined in the format: [ISA]-i[MASK_SIZE]x[GANG_SIZE].
        #
        # List of all supported targets (generated by ispc --help):
        #   sse2-i32x4, sse2-i32x8, sse4-i8x16, sse4-i16x8,
        #   sse4-i32x4, sse4-i32x8, avx1-i32x4, avx1-i32x8,
        #   avx1-i32x16, avx1-i64x4, avx2-i8x32, avx2-i16x16,
        #   avx2-i32x4, avx2-i32x8, avx2-i32x16, avx2-i64x4,
        #   avx512knl-i32x16, avx512skx-i32x8, avx512skx-i32x16,
        #   avx512skx-i8x64, avx512skx-i16x32, neon-i8x16,
        #   neon-i16x8, neon-i32x4, neon-i32x8, genx-x8, genx-x16
        if(LINUX_AARCH64)
            set(${ispc_isas} neon-i32x4)
        else()
            set(${ispc_isas} sse2-i32x4 sse4-i32x4 avx1-i32x8 avx2-i32x8 avx512knl-i32x16 avx512skx-i32x16)
        endif()
    endif()

    set(${ispc_isas} ${${ispc_isas}} PARENT_SCOPE)

endfunction()
