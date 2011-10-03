# wtf - qmake build info
HEADERS += \
    wtf/Alignment.h \
    wtf/AlwaysInline.h \
    wtf/ASCIICType.h \
    wtf/Assertions.h \
    wtf/Atomics.h \
    wtf/AVLTree.h \
    wtf/Bitmap.h \
    wtf/BitVector.h \
    wtf/BloomFilter.h \
    wtf/BoundsCheckedPointer.h \
    wtf/BumpPointerAllocator.h \
    wtf/ByteArray.h \
    wtf/CheckedArithmetic.h \
    wtf/Compiler.h \
    wtf/CrossThreadRefCounted.h \
    wtf/CryptographicallyRandomNumber.h \
    wtf/CurrentTime.h \
    wtf/DateMath.h \
    wtf/DecimalNumber.h \
    wtf/Decoder.h \
    wtf/Deque.h \
    wtf/DisallowCType.h \
    wtf/dtoa/bignum-dtoa.h \
    wtf/dtoa/bignum.h \
    wtf/dtoa/cached-powers.h \
    wtf/dtoa/diy-fp.h \
    wtf/dtoa/double-conversion.h \
    wtf/dtoa/double.h \
    wtf/dtoa/fast-dtoa.h \
    wtf/dtoa/fixed-dtoa.h \
    wtf/dtoa.h \
    wtf/dtoa/strtod.h \
    wtf/dtoa/utils.h \
    wtf/DynamicAnnotations.h \
    wtf/Encoder.h \
    wtf/FastAllocBase.h \
    wtf/FastMalloc.h \
    wtf/FixedArray.h \
    wtf/Forward.h \
    wtf/GetPtr.h \
    wtf/HashCountedSet.h \
    wtf/HashFunctions.h \
    wtf/HashIterators.h \
    wtf/HashMap.h \
    wtf/HashSet.h \
    wtf/HashTable.h \
    wtf/HashTraits.h \
    wtf/HexNumber.h \
    wtf/ListHashSet.h \
    wtf/ListRefPtr.h \
    wtf/Locker.h \
    wtf/MainThread.h \
    wtf/MallocZoneSupport.h \
    wtf/MathExtras.h \
    wtf/MD5.h \
    wtf/MessageQueue.h \
    wtf/MetaAllocator.h \
    wtf/MetaAllocatorHandle.h \
    wtf/Noncopyable.h \
    wtf/NonCopyingSort.h \
    wtf/NotFound.h \
    wtf/NullPtr.h \
    wtf/OSAllocator.h \
    wtf/OSRandomSource.h \
    wtf/OwnArrayPtr.h \
    wtf/OwnFastMallocPtr.h \
    wtf/OwnPtrCommon.h \
    wtf/OwnPtr.h \
    wtf/PackedIntVector.h \
    wtf/PageAllocationAligned.h \
    wtf/PageAllocation.h \
    wtf/PageAllocatorSymbian.h \
    wtf/PageBlock.h \
    wtf/PageReservation.h \
    wtf/ParallelJobsGeneric.h \
    wtf/ParallelJobs.h \
    wtf/ParallelJobsLibdispatch.h \
    wtf/ParallelJobsOpenMP.h \
    wtf/PassOwnArrayPtr.h \
    wtf/PassOwnPtr.h \
    wtf/PassRefPtr.h \
    wtf/PassTraits.h \
    wtf/Platform.h \
    wtf/PossiblyNull.h \
    wtf/qt/UtilsQt.h \
    wtf/RandomNumber.h \
    wtf/RandomNumberSeed.h \
    wtf/RedBlackTree.h \
    wtf/RefCounted.h \
    wtf/RefCountedLeakCounter.h \
    wtf/RefPtr.h \
    wtf/RefPtrHashMap.h \
    wtf/RetainPtr.h \
    wtf/SHA1.h \
    wtf/StackBounds.h \
    wtf/StaticConstructors.h \
    wtf/StdLibExtras.h \
    wtf/StringExtras.h \
    wtf/StringHasher.h \
    wtf/TCPackedCache.h \
    wtf/TCSpinLock.h \
    wtf/TCSystemAlloc.h \
    wtf/text/AtomicString.h \
    wtf/text/AtomicStringHash.h \
    wtf/text/AtomicStringImpl.h \
    wtf/text/CString.h \
    wtf/text/StringBuffer.h \
    wtf/text/StringBuilder.h \
    wtf/text/StringConcatenate.h \
    wtf/text/StringHash.h \
    wtf/text/StringImplBase.h \
    wtf/text/StringImpl.h \
    wtf/text/StringOperators.h \
    wtf/text/TextPosition.h \
    wtf/text/WTFString.h \
    wtf/Threading.h \
    wtf/ThreadingPrimitives.h \
    wtf/ThreadRestrictionVerifier.h \
    wtf/ThreadSafeRefCounted.h \
    wtf/ThreadSpecific.h \
    wtf/TypeTraits.h \
    wtf/unicode/CharacterNames.h \
    wtf/unicode/Collator.h \
    wtf/unicode/icu/UnicodeIcu.h \
    wtf/unicode/qt4/UnicodeQt4.h \
    wtf/unicode/ScriptCodesFromICU.h \
    wtf/unicode/Unicode.h \
    wtf/unicode/UnicodeMacrosFromICU.h \
    wtf/unicode/UTF8.h \
    wtf/UnusedParam.h \
    wtf/ValueCheck.h \
    wtf/Vector.h \
    wtf/VectorTraits.h \
    wtf/VMTags.h \
    wtf/WTFThreadData.h

SOURCES += \
    wtf/Assertions.cpp \
    wtf/BitVector.cpp \
    wtf/ByteArray.cpp \
    wtf/CryptographicallyRandomNumber.cpp \
    wtf/CurrentTime.cpp \
    wtf/DateMath.cpp \
    wtf/DecimalNumber.cpp \
    wtf/dtoa.cpp \
    wtf/dtoa/bignum-dtoa.cc \
    wtf/dtoa/bignum.cc \
    wtf/dtoa/cached-powers.cc \
    wtf/dtoa/diy-fp.cc \
    wtf/dtoa/double-conversion.cc \
    wtf/dtoa/fast-dtoa.cc \
    wtf/dtoa/fixed-dtoa.cc \
    wtf/dtoa/strtod.cc \
    wtf/FastMalloc.cpp \
    wtf/gobject/GOwnPtr.cpp \
    wtf/gobject/GRefPtr.cpp \
    wtf/HashTable.cpp \
    wtf/MD5.cpp \
    wtf/MainThread.cpp \
    wtf/MetaAllocator.cpp \
    wtf/NullPtr.cpp \
    wtf/OSRandomSource.cpp \
    wtf/qt/MainThreadQt.cpp \
    wtf/qt/StringQt.cpp \
    wtf/qt/ThreadingQt.cpp \
    wtf/PageAllocationAligned.cpp \
    wtf/PageBlock.cpp \
    wtf/ParallelJobsGeneric.cpp \
    wtf/RandomNumber.cpp \
    wtf/RefCountedLeakCounter.cpp \
    wtf/SHA1.cpp \
    wtf/StackBounds.cpp \
    wtf/TCSystemAlloc.cpp \
    wtf/Threading.cpp \
    wtf/TypeTraits.cpp \
    wtf/WTFThreadData.cpp \
    wtf/text/AtomicString.cpp \
    wtf/text/CString.cpp \
    wtf/text/StringBuilder.cpp \
    wtf/text/StringImpl.cpp \
    wtf/text/StringStatics.cpp \
    wtf/text/WTFString.cpp \
    wtf/unicode/CollatorDefault.cpp \
    wtf/unicode/icu/CollatorICU.cpp \
    wtf/unicode/UTF8.cpp

linux-*:!contains(DEFINES, USE_QTMULTIMEDIA=1) {
    !contains(QT_CONFIG, no-pkg-config):system(pkg-config --exists glib-2.0 gio-2.0 gstreamer-0.10): {
        DEFINES += ENABLE_GLIB_SUPPORT=1
        PKGCONFIG += glib-2.0 gio-2.0
        CONFIG += link_pkgconfig

        HEADERS += wtf/gobject/GOwnPtr.h
    }
}

unix:!symbian: SOURCES += wtf/OSAllocatorPosix.cpp
symbian: SOURCES += wtf/OSAllocatorSymbian.cpp
win*|wince*: SOURCES += wtf/OSAllocatorWin.cpp
