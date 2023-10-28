/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <assert.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>

#include "thread_utils.h"
#include "zephyr/net/openthread.h"

#include <zephyr/logging/log.h>
#include <openthread/instance.h>
#include <version.h>

#include <openthread/config.h>
#include <openthread/cli.h>
#include <openthread/diag.h>
#include <openthread/error.h>
#include <openthread/joiner.h>
#include <openthread/link.h>
#include <openthread/tasklet.h>
#include <openthread/platform/logging.h>
#include <openthread/dataset_ftd.h>
#include <openthread/thread.h>

extern uint8_t is_ot_discovery_done;
extern uint8_t is_ot_join_done;

void setNullNetworkKey(otInstance *aInstance)
{
	static char          aNetworkName[] = "TestNetwork";
    otOperationalDataset aDataset;

    memset(&aDataset, 0, sizeof(otOperationalDataset));

    /* Set network key */
    uint8_t key[OT_NETWORK_KEY_SIZE] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    memcpy(aDataset.mNetworkKey.m8, key, sizeof(aDataset.mNetworkKey));
    aDataset.mComponents.mIsNetworkKeyPresent = true;
	
    otDatasetSetActive(aInstance, &aDataset);
}

static void setNetworkConfiguration(otInstance *aInstance)
{
	static char          aNetworkName[] = "TestNetwork";
    otOperationalDataset aDataset;

    memset(&aDataset, 0, sizeof(otOperationalDataset));

    /*
     * Fields that can be configured in otOperationDataset to override defaults:
     *     Network Name, Mesh Local Prefix, Extended PAN ID, PAN ID, Delay Timer,
     *     Channel, Channel Mask Page 0, Network Key, PSKc, Security Policy
     */
    aDataset.mActiveTimestamp.mSeconds             = 1;
    aDataset.mActiveTimestamp.mTicks               = 0;
    aDataset.mActiveTimestamp.mAuthoritative       = false;
    aDataset.mComponents.mIsActiveTimestampPresent = true;

    /* Set Channel */
    aDataset.mChannel                      = CONFIG_OT_CHANNEL;
    aDataset.mComponents.mIsChannelPresent = true;

    /* Set Pan ID */
    aDataset.mPanId                      = (otPanId)CONFIG_OT_PAN_ID;
    aDataset.mComponents.mIsPanIdPresent = true;

    /* Set Extended Pan ID */
    /* uint8_t extPanId[OT_EXT_PAN_ID_SIZE] = {0xC0, 0xDE, 0x1A, 0xB5, 0xC0, 0xDE, 0x1A, 0xB5}; */
    uint8_t extPanId[OT_EXT_PAN_ID_SIZE] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11};
    memcpy(aDataset.mExtendedPanId.m8, extPanId, sizeof(aDataset.mExtendedPanId));
    aDataset.mComponents.mIsExtendedPanIdPresent = true;

    /* Set network key */
    /* uint8_t key[OT_NETWORK_KEY_SIZE] = {0x12, 0x34, 0xC0, 0xDE, 0x1A, 0xB5, 0x12, 0x34, 0xC0, 0xDE, 0x1A, 0xB5, 0x12, 0x34, 0xC0, 0xDE}; */
    uint8_t key[OT_NETWORK_KEY_SIZE] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11};
    memcpy(aDataset.mNetworkKey.m8, key, sizeof(aDataset.mNetworkKey));
    aDataset.mComponents.mIsNetworkKeyPresent = true;

    /* Set Network Name */
    size_t length = strlen(aNetworkName);
    assert(length <= OT_NETWORK_NAME_MAX_SIZE);
    memcpy(aDataset.mNetworkName.m8, aNetworkName, length);
    aDataset.mComponents.mIsNetworkNamePresent = true;

    otDatasetSetActive(aInstance, &aDataset);
}

LOG_MODULE_REGISTER(thread_utils, CONFIG_LOG_DEFAULT_LEVEL);

#define DEVICE_NAME	CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

extern uint8_t wait4_peer_thread2_start_connection;

int thread_throughput_test_run(void)
{

	return 0;
}

void handle_active_scan_result(struct otActiveScanResult *result, void *context)
{
	if (!result) {
		return;
	}
	LOG_INF("----------------------------------");
	LOG_INF("Discovered Thread network details");
	LOG_INF("name: %-16s", result->mNetworkName.m8);
	LOG_INF("panid: %04x", result->mPanId);
	LOG_INF("channel: %2u", result->mChannel);
	LOG_INF("rssi: %3d", result->mRssi);
	LOG_INF("----------------------------------");

	is_ot_discovery_done = 1;
}

int thread_throughput_test_init(bool is_thread_client)
{
	struct openthread_context *context = openthread_get_default_context();
	otInstance *instance = openthread_get_default_instance();
	LOG_INF("Updating thread parameters");
	setNetworkConfiguration(instance);
	LOG_INF("Enabling thread");

	/* otIp6SetEnabled(instance, true); */ /* cli `ifconfig up` */
	/* otThreadSetEnabled(instance, true); */ /* cli `thread start` */
	otError err = openthread_start(context);	// 'ifconfig up && thread start'
	if (err != OT_ERROR_NONE) {
		LOG_ERR("Starting openthread: %d (%s)", err, otThreadErrorToString(err));
	}

	otDeviceRole current_role = otThreadGetDeviceRole(instance);
	LOG_INF("Current role of Thread device: %s", otThreadDeviceRoleToString(current_role));

	LOG_INF("Performing Thread discover");
	openthread_api_mutex_lock(context);
	otThreadDiscover(openthread_get_default_instance(), 0 /* all channels */,
		OT_PANID_BROADCAST, false, false, handle_active_scan_result, NULL);
	openthread_api_mutex_unlock(openthread_get_default_context());

	return 0;
}


int thread_throughput_test_exit(void)
{
	openthread_api_mutex_lock(openthread_get_default_context());
	otThreadSetEnabled(openthread_get_default_instance(), false);
	otIp6SetEnabled(openthread_get_default_instance(), false);
	openthread_api_mutex_unlock(openthread_get_default_context());
	return 0;
}


const char* check_ot_state()
{
	otDeviceRole current_role = otThreadGetDeviceRole(openthread_get_default_instance());
	/* LOG_INF("Current state of thread device: %s", otThreadDeviceRoleToString(current_role)); */
	return(otThreadDeviceRoleToString(current_role));
}


void thread_start_commissioner(const char *pskd, const otExtAddress *allowed_eui64)
{
	struct openthread_context *context = openthread_get_default_context();

	openthread_api_mutex_lock(context);
	otCommissionerAddJoiner(openthread_get_default_instance(), allowed_eui64, pskd, 120);
	openthread_api_mutex_unlock(context);
}

static void ot_joiner_start_handler(otError error, void *context)
{
    switch (error)
    {
    case OT_ERROR_NONE:
        LOG_INF("Join success");
		openthread_api_mutex_lock(openthread_get_default_context());
		otThreadSetEnabled(openthread_get_default_instance(), true);
		openthread_api_mutex_unlock(openthread_get_default_context());
		
		is_ot_join_done = 1;
        break;

    default:
        LOG_ERR("Join failed [%s]", otThreadErrorToString(error));
        break;
    }
}

//void thread_start_joiner(const char *pskd) 
void thread_start_joiner(const char *pskd, otInstance *instance)
//void thread_start_joiner(const char *pskd, otInstance *instance, struct openthread_context *context)
{
	LOG_INF("Starting joiner");

	//otInstance *instance = openthread_get_default_instance();
	struct openthread_context *context = openthread_get_default_context();

	openthread_api_mutex_lock(context);
	otIp6SetEnabled(instance, true);
	otJoinerStart(instance, pskd, NULL,
				"Zephyr", "Zephyr",
				KERNEL_VERSION_STRING, NULL,
				&ot_joiner_start_handler, NULL);
	openthread_api_mutex_unlock(context);
	LOG_INF("Done.");
}
