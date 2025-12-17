include(AddLLVM)

set(PASS_LIST
    AffineRecurrence
    DerivedInductionVar
    SimpleLICM
)

foreach(PASS ${PASS_LIST})
    # Tell LLVM build system it's okay that this directory has multiple targets
    add_llvm_pass_plugin(
        ${PASS}
        ${PASS}.cpp
        PARTIAL_SOURCES_INTENDED
    )
endforeach()
