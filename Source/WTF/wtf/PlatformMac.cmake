set(WTF_LIBRARY_TYPE SHARED)

find_library(COREFOUNDATION_LIBRARY CoreFoundation)
find_library(FOUNDATION_LIBRARY Foundation)
find_library(SECURITY_LIBRARY Security)
list(APPEND WTF_LIBRARIES
    ${COREFOUNDATION_LIBRARY}
    ${FOUNDATION_LIBRARY}
    ${SECURITY_LIBRARY}
    libicucore.dylib
)

list(APPEND WTF_SOURCES
    AutodrainedPoolMac.mm
    RunLoopTimerCF.cpp
    SchedulePairCF.cpp
    SchedulePairMac.mm

    cf/RunLoopCF.cpp

    cocoa/WorkQueueCocoa.cpp

    mac/DeprecatedSymbolsUsedBySafari.mm
    mac/MainThreadMac.mm

    ios/WebCoreThread.cpp

    text/cf/AtomicStringImplCF.cpp
    text/cf/StringCF.cpp
    text/cf/StringImplCF.cpp
    text/cf/StringViewCF.cpp

    text/mac/StringImplMac.mm
    text/mac/StringMac.mm
    text/mac/StringViewObjC.mm
)

list(APPEND WTF_INCLUDE_DIRECTORIES
    "${WTF_DIR}/icu"
    "${WTF_DIR}/wtf/spi/darwin"
)
