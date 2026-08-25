include_guard(GLOBAL)

if(MSVC)
set(
    VULKAN_TEMPLATE_DEFINITIONS
    ""
)

set(
    VULKAN_TEMPLATE_FLAGS
    ""
)

set(
    VULKAN_TEMPLATE_WARNINGS
    /W4
    /w14242
    /w14254
    /w14263
    /w14265
    /w14287
    /we4289
    /w14296
    /w14311
    /w14545
    /w14546
    /w14547
    /w14549
    /w14555
    /w14619
    /w14640
    /w14826
    /w14905
    /w14906
    /w14928
    /permissive-
)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
set(
    VULKAN_TEMPLATE_DEFINITIONS
    $<$<CONFIG:Release>:-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3>
)

set(
    VULKAN_TEMPLATE_FLAGS
    -pedantic-errors
    -pipe
    -fno-common

    # -fno-rtti
    # -fsafe-buffer-usage-suggestions
    # -fno-exceptions

    #ssf
    -fstrict-flex-arrays=3
    # -fstack-clash-protection
    # -fstack-protector-strong
    # -fcf-protection=full

    # -Wl,-z,nodlopen -Wl,-z,noexecstack \
    # -Wl,-z,relro -Wl,-z,now \
    # -Wl,--as-needed -Wl,--no-copy-dt-needed-entries
    # -D_GLIBCXX_ASSERTIONS \
    # -D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_FAST \

    #redhat
    # -mshstk
    # -fsplit-stack
    # -fstack-limit-register
    # -fstack-limit-symbol
    # -fno-stack-array
)

set(
    VULKAN_TEMPLATE_WARNINGS
    -Wall
    -Wextra
    -Waarch64-sme-attributes
    -Wabstract-vbase-init
    -Walloca
    -Wanon-enum-enum-conversion
    -Warc-repeated-use-of-weak
    -Warray-bounds-pointer-arithmetic
    -Wassign-enum
    -Watomic-implicit-seq-cst
    -Watomic-properties
    -Wbinary-literal
    -Wbind-to-temporary-copy
    -Wcalled-once-parameter
    -Wcast-align
    -Wcast-function-type
    -Wcast-qual
    -Wclass-varargs
    -Wcomma
    -Wcompound-token-split
    -Wconditional-uninitialized
    -Wconsumed
    -Wcovered-switch-default
    -Wcstring-format-directive
    -Wctad-maybe-unsupported
    -Wcuda-compat
    -Wdate-time
    -Wdecls-in-multiple-modules
    -Wdeprecated
    -Wdeprecated-implementations
    -Wdirect-ivar-access
    -Wdisabled-macro-expansion
    -Wdocumentation
    -Wdocumentation-pedantic
    -Wdouble-promotion
    -Wdtor-name
    -Wduplicate-enum
    -Wduplicate-method-arg
    -Wduplicate-method-match
    -Wdynamic-exception-spec
    -Wexit-time-destructors
    -Wexpansion-to-defined
    -Wexperimental-lifetime-safety
    -Wexperimental-lifetime-safety-suggestions
    -Wexplicit-ownership-type
    -Wextra-semi
    -Wextra-semi-stmt
    -Wfloat-equal
    -Wformat-non-iso
    -Wformat-pedantic
    -Wformat-signedness
    -Wformat-type-confusion
    -Wformat=2
    -Wfour-char-constants
    -Wfunction-effect-redeclarations
    -Wfunction-effects
    -Wgcc-compat
    -Wglobal-constructors
    -Wgnu
    -Wheader-hygiene
    -Whlsl-implicit-binding
    -Widiomatic-parentheses
    -Wignored-base-class-qualifiers
    -Wimplicit-fallthrough
    -Wimplicit-retain-self
    -Wincompatible-function-pointer-types-strict
    -Wincomplete-module
    -Winconsistent-missing-destructor-override
    -Winvalid-or-nonexistent-directory
    -Wloop-analysis
    -Wmain
    -Wmain-return-type
    -Wmax-tokens
    -Wmethod-signatures
    -Wmicrosoft
    -Wmissing-include-dirs
    -Wmissing-noreturn
    -Wmodule-file-mapping-mismatch
    -Wms-bitfield-padding
    -Wnewline-eof
    -Wnon-gcc
    -Wnon-virtual-dtor
    -Wnonportable-private-system-apinotes-path
    -Wnonportable-system-include-path
    -Wnrvo
    -Wnullable-to-nonnull-conversion
    -Wnvcc-compat
    -Wobjc-interface-ivars
    -Wobjc-messaging-id
    -Wobjc-missing-property-synthesis
    -Wobjc-property-assign-on-object-type
    -Wobjc-signed-char-bool
    -Wold-style-cast
    -Wopenmp
    -Wover-aligned
    -Woverriding-method-mismatch
    -Wpacked
    -Wpadded
    -Wpedantic
    -Wpedantic-core-features
    -Wperf-constraint-implies-noexcept
    -Wpointer-arith
    -Wpoison-system-directories
    -Wpragmas
    -Wpre-openmp-51-compat
    -Wprofile-instr-missing
    -Wquoted-include-in-framework-header
    -Wreceiver-forward-class
    -Wredundant-parens
    -Wreserved-identifier
    -Wreserved-user-defined-literal
    -Rsanitize-address
    -Wshadow-all
    -Wshadow-header
    -Wshift-bool
    -Wshift-sign-overflow
    -Wsigned-enum-bitfield
    -Wsource-uses-openacc
    -Wspir-compat
    -Wstrict-potentially-direct-selector
    -Wsuggest-destructor-override
    -Wsuggest-override
    -Wsuper-class-method-mismatch
    -Wswitch-default
    -Wswitch-enum
    -Wtautological-constant-in-range-compare
    -Wthread-safety
    -Wthread-safety-beta
    -Wthread-safety-negative
    -Wthread-safety-pointer
    -Wthread-safety-verbose
    -Wunaligned-access
    -Wundef
    -Wundef-prefix
    -Wundefined-func-template
    -Wundefined-reinterpret-cast
    -Wunguarded-availability
    -Wunique-object-duplication
    -Wunnamed-type-template-args
    -Wunreachable-code-aggressive
    -Wunsafe-buffer-usage
    -Wunsupported-dll-base-class-template
    -Wunused-exception-parameter
    -Wunused-macros
    -Wunused-member-function
    -Wunused-template
    -Wused-but-marked-unused
    -Wvariadic-macros
    -Wvector-conversion
    -Wvla
    -Wweak-template-vtables
    -Wweak-vtables
    -Wzero-as-null-pointer-constant
)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
set(
    VULKAN_TEMPLATE_DEFINITIONS
    $<$<CONFIG:Release>:-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3>
)

set(
    VULKAN_TEMPLATE_FLAGS
    -pedantic-errors
    -pipe
    -fno-common

    # -fno-rtti
    # -fsafe-buffer-usage-suggestions
    # -fno-exceptions

    #ssf
    -fstrict-flex-arrays=3
    # -fstack-clash-protection
    # -fstack-protector-strong
    # -fcf-protection=full

    # -Wl,-z,nodlopen -Wl,-z,noexecstack \
    # -Wl,-z,relro -Wl,-z,now \
    # -Wl,--as-needed -Wl,--no-copy-dt-needed-entries
    # -D_GLIBCXX_ASSERTIONS \
    # -D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_FAST \

    #redhat
    # -mshstk
    # -fsplit-stack
    # -fstack-limit-register
    # -fstack-limit-symbol
    # -fno-stack-array
)

set(
    VULKAN_TEMPLATE_WARNINGS
    -Wall
    -Wextra
    -Walloca
    -Wcast-align
    -Wcast-function-type
    -Wcast-qual
    -Wctad-maybe-unsupported
    -Wdate-time
    -Wdeprecated
    -Wdouble-promotion
    -Wexpansion-to-defined
    -Wextra-semi
    -Wfloat-equal
    -Wformat-signedness
    -Wformat=2
    -Wimplicit-fallthrough
    -Wmain
    -Wmissing-include-dirs
    -Wmissing-noreturn
    -Wnon-virtual-dtor
    -Wnrvo
    -Wold-style-cast
    -Wopenmp
    -Wpacked
    -Wpadded
    -Wpedantic
    -Wpointer-arith
    -Wpragmas
    -Wsuggest-override
    -Wswitch-default
    -Wswitch-enum
    -Wundef
    -Wunused-macros
    -Wvariadic-macros
    -Wvla
    -Wzero-as-null-pointer-constant
)
endif()

set(
    SLANG_MODULE_SOURCES
    shaders/shader.slang
)

set(VULKAN_MODULE_DEFINITIONS ${VULKAN_TEMPLATE_DEFINITIONS})
set(VULKAN_MODULE_FLAGS ${VULKAN_TEMPLATE_FLAGS})
set(VULKAN_MODULE_WARNINGS ${VULKAN_TEMPLATE_WARNINGS})
