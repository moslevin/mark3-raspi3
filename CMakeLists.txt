set(folder_prefix ./arch/${mark3_arch}/${mark3_variant}/${mark3_toolchain})

set(arm_extra_cxx
    ${folder_prefix}/kernelswi.cpp
    ${folder_prefix}/kerneltimer.cpp
    ${folder_prefix}/threadport.cpp
    ${folder_prefix}/port_asm.s
    )

set(arm_extra_headers
    ${folder_prefix}/portcfg.h
    ${folder_prefix}/threadport.h
    )

set_source_files_properties(${folder_prefix}/port_asm.s PROPERTIES COMPILE_FLAGS "-x assembler-with-cpp")

set_property(GLOBAL PROPERTY global_mark3_extra_cxx "${arm_extra_cxx}")
set_property(GLOBAL PROPERTY global_mark3_extra_headers "${arm_extra_headers}")
