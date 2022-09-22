#!/bin/bash -x

# This script allows to build set of sample applications with support for
# MPSL_CX_NRF700X
# It works with DTC overlay files provided in the same directory as the script

# Let's fail in case of error
set -e

# The directory in which current script resides
script_dir=$(dirname "$0")

# The board for which build is done
board="nrf7002dk_nrf5340_cpuapp"

# Additional flags to be passed to west build to enable CX
cx_west_flags="-DCONFIG_MPSL_CX=y -DCONFIG_MPSL_CX_NRF700X=y"

rm -fr "build/artifacts_53"
mkdir -p "build/artifacts_53"

function build_app() {
    local app=$1
    local artifact_name=$2
    local other_west_flags=$3
    local app_src_dir="${script_dir}/../${app}"
    local build_dir="build/${artifact_name}"

    # The overlay file needs to be in the app_src_dir to be reachable
    cp -vf ${script_dir}/overlay-ble.conf ${app_src_dir}/overlay-ble.conf
    cp -vf ${script_dir}/overlay-wifi.conf ${app_src_dir}/
    cp -vf ${script_dir}/overlay-wifi-zperf.conf ${app_src_dir}/

    # The DTC_OVERLAY_FILE replaces other means of finding overlays, we need
    # to find the manually and apply in DTC_OVERLAY_FILE list
    local dtc_overlay_files=""
    if [ -f "${app_src_dir}/boards/${board}.overlay" ]; then
        dtc_overlay_files+=";boards/${board}.overlay"
    fi

    west build -p -b "${board}" -d "${build_dir}" ${app_src_dir} -- \
        ${cx_west_flags} \
        -DDTC_OVERLAY_FILE=${dtc_overlay_files} \
        ${other_west_flags}

    # clean-up
    rm ${app_src_dir}/overlay-ble.conf
    rm ${app_src_dir}/overlay-wifi.conf
    rm ${app_src_dir}/overlay-wifi-zperf.conf

    # Gather artifacts
    cp -v "${build_dir}/zephyr/zephyr.hex" "build/artifacts_53/${artifact_name}.hex"
    cp -v "${build_dir}/zephyr/merged_domains.hex" "build/artifacts_53/${artifact_name}_merged_domains.hex"
    cp -v "${build_dir}/hci_rpmsg/zephyr/zephyr.hex" "build/artifacts_53/${artifact_name}_hci_rpmsg.hex"
}

#build_app "samples/zigbee/shell" "zigbee_shell"
#build_app "samples/openthread/cli" "openthread_cli"
#build_app "samples/openthread/coap_client" "openthread_coap_client"
#build_app "samples/openthread/coap_server" "openthread_coap_server"
#build_app "samples/openthread/coprocessor" "openthread_rcp" "-DCONFIG_OPENTHREAD_COPROCESSOR_RCP=y -DCONFIG_OPENTHREAD_THREAD_VERSION_1_2=y"

#build_app "samples/zigbee/shell" "zigbee_bt_shell" "-DOVERLAY_CONFIG=overlay-ble.conf"
#build_app "samples/openthread/cli" "openthread_bt_cli" "-DOVERLAY_CONFIG=overlay-ble.conf"
#build_app "../zephyr/tests/bluetooth/shell" "bt_shell_sample_53" "-DOVERLAY_CONFIG=overlay-ble.conf"
build_app "../zephyr/tests/bluetooth/shell" "bt_shell_sample_53" "-DOVERLAY_CONFIG=overlay-ble.conf;overlay-wifi.conf;overlay-wifi-zperf.conf"
build_app "samples/bluetooth/llpm" "bt_llpm_sample_53" "-DOVERLAY_CONFIG=overlay-ble.conf;overlay-wifi.conf;overlay-wifi-zperf.conf"
build_app "samples/bluetooth/throughput" "bt_throughput_sample_53" "-DOVERLAY_CONFIG=overlay-ble.conf;overlay-wifi.conf;overlay-wifi-zperf.conf"
