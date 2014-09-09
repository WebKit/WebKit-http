#!/bin/sh

postProcessInDirectory()
{
    cd "$1"

    local unifdefOptions sedExpression

    if [[ ${PLATFORM_NAME} == iphoneos ]]; then
        unifdefOptions="-DTARGET_OS_EMBEDDED=1 -DTARGET_OS_IPHONE=1 -DTARGET_IPHONE_SIMULATOR=0";
    elif [[ ${PLATFORM_NAME} == iphonesimulator ]]; then
        unifdefOptions="-DTARGET_OS_EMBEDDED=0 -DTARGET_OS_IPHONE=1 -DTARGET_IPHONE_SIMULATOR=1";
    else
        unifdefOptions="-DTARGET_OS_EMBEDDED=0 -DTARGET_OS_IPHONE=0 -DTARGET_IPHONE_SIMULATOR=0";
    fi

    # FIXME: We should consider making this logic general purpose so as to support keeping or removing
    # code guarded by an arbitrary feature define. For now it's sufficient to process touch- and gesture-
    # guarded code.
    for featureDefine in "ENABLE_TOUCH_EVENTS" "ENABLE_IOS_GESTURE_EVENTS"
    do
        # We assume a disabled feature is either undefined or has the empty string as its value.
        eval "isFeatureEnabled=\$$featureDefine"
        if [[ -z $isFeatureEnabled ]]; then
            unifdefOptions="$unifdefOptions -D$featureDefine=0"
        else
            unifdefOptions="$unifdefOptions -D$featureDefine=1"
        fi
    done

    if [[ ${PLATFORM_NAME} == iphone* ]]; then
        sedExpression='s/ *WEBKIT_((CLASS_|ENUM_)?AVAILABLE|DEPRECATED)_MAC\([^)]+\)//g';
    else
        sedExpression='s/WEBKIT_((CLASS_|ENUM_)?AVAILABLE|DEPRECATED)/NS_\1/g';
    fi

    for header in $(find . -name '*.h' -type f); do
        unifdef -B ${unifdefOptions} -o ${header}.unifdef ${header}
        case $? in
        0)
            rm ${header}.unifdef
            ;;
        1)
            mv ${header}{.unifdef,}
            ;;
        *)
            exit 1
            ;;
        esac

        if [[ ${header} == "./WebKitAvailability.h" ]]; then
            continue
        fi

        sed -E -e "${sedExpression}" < ${header} > ${header}.sed
        if cmp -s ${header} ${header}.sed; then
            rm ${header}.sed
        else
            mv ${header}.sed ${header}
        fi
    done
}

postProcessInDirectory "${TARGET_BUILD_DIR}/${PUBLIC_HEADERS_FOLDER_PATH}"
postProcessInDirectory "${TARGET_BUILD_DIR}/${PRIVATE_HEADERS_FOLDER_PATH}"
