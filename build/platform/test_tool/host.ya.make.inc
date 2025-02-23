IF (HOST_OS_DARWIN AND HOST_ARCH_X86_64)
    DECLARE_EXTERNAL_RESOURCE(TEST_TOOL_HOST sbr:2995258782)
    DECLARE_EXTERNAL_RESOURCE(TEST_TOOL3_HOST sbr:2995567094)
ELSEIF (HOST_OS_DARWIN AND HOST_ARCH_ARM64)
    DECLARE_EXTERNAL_RESOURCE(TEST_TOOL_HOST sbr:2995254290)
    DECLARE_EXTERNAL_RESOURCE(TEST_TOOL3_HOST sbr:2995563952)
ELSEIF (HOST_OS_LINUX AND HOST_ARCH_PPC64LE)
    DECLARE_EXTERNAL_RESOURCE(TEST_TOOL_HOST sbr:2995258058)
    DECLARE_EXTERNAL_RESOURCE(TEST_TOOL3_HOST sbr:2995564776)
ELSEIF (HOST_OS_LINUX AND HOST_ARCH_X86_64)
    DECLARE_EXTERNAL_RESOURCE(TEST_TOOL_HOST sbr:2995259840)
    DECLARE_EXTERNAL_RESOURCE(TEST_TOOL3_HOST sbr:2995568438)
ELSEIF (HOST_OS_WINDOWS AND HOST_ARCH_X86_64)
    DECLARE_EXTERNAL_RESOURCE(TEST_TOOL_HOST sbr:2995253441)
    DECLARE_EXTERNAL_RESOURCE(TEST_TOOL3_HOST sbr:2995563309)
ENDIF()
