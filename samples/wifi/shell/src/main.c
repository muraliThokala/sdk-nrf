/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief WiFi shell sample main function
 */

#include <zephyr/sys/printk.h>
#include <zephyr/kernel.h>
#if NRFX_CLOCK_ENABLED && (defined(CLOCK_FEATURE_HFCLK_DIVIDE_PRESENT) || NRF_CLOCK_HAS_HFCLK192M)
#include <nrfx_clock.h>
#endif
#include <zephyr/device.h>
#include <zephyr/net/net_config.h>

#if defined(CONFIG_USB_DEVICE_STACK) && !defined(CONFIG_BOARD_THINGY91X_NRF5340_CPUAPP)
#define USES_USB_ETH 1
#else
#define USES_USB_ETH 0
#endif

#if USES_USB_ETH
#include <zephyr/usb/usb_device.h>
#endif

#if USES_USB_ETH || defined(CONFIG_SLIP)
static struct in_addr addr = { { { 192, 0, 2, 1 } } };
static struct in_addr mask = { { { 255, 255, 255, 0 } } };
#endif /* CONFIG_USB_DEVICE_STACK || CONFIG_SLIP */

#if USES_USB_ETH
int init_usb(void)
{
	int ret;

	ret = usb_enable(NULL);
	if (ret != 0) {
		printk("Cannot enable USB (%d)", ret);
		return ret;
	}

	return 0;
}
#endif

#define CONFIGURE_ENABLE_COEX

#ifdef CONFIGURE_ENABLE_COEX

	#include <stdlib.h> /* for exit */
	#include <string.h> 
	#include <unistd.h>    
	#include <stdio.h>
	#include <stdint.h>

	//#include "cm_host_if.h"	
	//#include "wlan_enums.h"

	#include "./uccp720_80_registers.h"

	#define UCC_PERIP_SHIFT (0)
	#define UCC_HW_READ32(addr)        (*(volatile uint32_t *)(addr))
	#define UCC_HW_WRITE32(addr, val)  (*(volatile uint32_t *)(addr)) = (val)
	#define UCC_LOW_READ32      UCC_HW_READ32
	#define UCC_LOW_WRITE32      UCC_HW_WRITE32
	#define UCC_READ32      UCC_LOW_READ32
	#define UCC_WRITE32     UCC_LOW_WRITE32
	#define UCC_WRITE_PERIP(addr, val) UCC_WRITE32((addr), (val) << UCC_PERIP_SHIFT)
	#define UCC_READ_PERIP(addr) (UCC_READ32(addr) >> UCC_PERIP_SHIFT)	


	#define COEXC_CLK_FREQ_MHZ                 16u

	#define WLAN_HIGH_PTI_RX_CCCONF_PTI        1u 
	#define WLAN_HIGH_PTI_TX_CCCONF_PTI        2u
	#define SR_RX_CCCONF_PTI                   5u
	#define SR_TX_CCCONF_PTI                   6u
	#define WLAN_LOW_PTI_RX_CCCONF_PTI         3u   
	#define WLAN_LOW_PTI_TX_CCCONF_PTI         4u
	#define LTE_PTI_RX_CCCONF_PTI              7u   
	#define LTE_PTI_TX_CCCONF_PTI              8u
											   
	#define TURNAROUND_WIFI_IN_US              2u
	#define TURNAROUND_SR_IN_US                3u
	#define TURNAROUND_LTE_IN_US               4u

	#define NUM_OF_CLIENTS_SUPPORTED           8
	#define NUM_OF_MODES_SUPPORTED             4

	#define CCMALLOW_TABLE_SIZE                (NUM_OF_CLIENTS_SUPPORTED * NUM_OF_MODES_SUPPORTED)
	#define NUM_OF_WIFI_CLIENTS_SUPPORTED      4u

	#define OFFSET_BETWEEN_MODES               (COEXC_CCMALLOW_0_MODE_1 - COEXC_CCMALLOW_0_MODE_0)
	#define OFFSET_BETWEEN_CLIENTS             (COEXC_CCMALLOW_1_MODE_0 - COEXC_CCMALLOW_0_MODE_0)

	#define CCMALLOW_PREPARED_OFFLINE          1u

	// Indicates if COEXC is to be enabled/disabled
	typedef enum
	{
		// To disable
		COEXC_DISABLE = 0,
		// To enable
		COEXC_ENABLE,
	} coexc_config_en_or_dis_t;

	// COEXC mode for Wi-Fi band.
	typedef enum
	{
		/** COEXC mode - Wi-Fi operating in 2.4G. */
		COEXC_MODE_WIFI_2PT4G = 0,
		/** COEXC mode - Wi-Fi operating in 5G. */
		COEXC_MODE_WIFI_5G,
		/** COEXC mode - Wi-Fi operating in 6G. Currently unused as band sel register is of 1-bit. */
		COEXC_MODE_WIFI_6G,
	} coexc_mode_wifi_t;

	// Indicates antenna configuration type
	typedef enum
	{
		// Shared antenna
		COEX_SHARED_ANT_CFG = 0,
		// Separate antennas
		COEX_SEPARATE_ANT_CFG = 1
	} ant_cfg_type_t;

	void configure_coexc(ant_cfg_type_t antenna_cfg_type);
	void enable_coex_in_macc(coexc_config_en_or_dis_t coexc_en_or_dis);
	void configure_coexistence_controller(void);


	// Enable/disable coexistence 
	void enable_coex_in_macc(coexc_config_en_or_dis_t coexc_en_or_dis)
	{
		printf("=================== Enable COEX in MACC \n");
		uint32_t coexc_config_in = UCC_READ_PERIP(ABS_PMB_WLAN_MAC_CTRL_COEX);
		uint32_t coexc_config_out = (coexc_config_in & (~PMB_WLAN_MAC_CTRL_COEX_ENABLE_MASK)) | 
					   (coexc_en_or_dis << PMB_WLAN_MAC_CTRL_COEX_ENABLE_SHIFT);
		UCC_WRITE_PERIP(ABS_PMB_WLAN_MAC_CTRL_COEX, coexc_config_out);
		
		printf("MACC : address = 0x%x in = 0x%x and out = 0x%x \n", ABS_PMB_WLAN_MAC_CTRL_COEX, coexc_config_in, coexc_config_out);		
	}

	// This configures the coexistence controller registers
	void configure_coexc(ant_cfg_type_t antenna_cfg_type) 
	{
		printf("=================== configuring COEXC (arb, non-arb) FM \n");
		const uint32_t turnaround_wifi = TURNAROUND_WIFI_IN_US * COEXC_CLK_FREQ_MHZ;
		const uint32_t turnaround_sr = TURNAROUND_SR_IN_US * COEXC_CLK_FREQ_MHZ;
		const uint32_t turnaround_lte = TURNAROUND_LTE_IN_US * COEXC_CLK_FREQ_MHZ;
		volatile uint32_t turnaround_config[NUM_OF_CLIENTS_SUPPORTED] = {0,0,0,0,0,0,0,0};
		volatile uint32_t ccconf_config[NUM_OF_CLIENTS_SUPPORTED] = {0,0,0,0,0,0,0,0};
		
		volatile int32_t wlan_priority0 = WLAN_HIGH_PTI_RX_CCCONF_PTI;
		volatile int32_t wlan_priority1 = WLAN_HIGH_PTI_TX_CCCONF_PTI;
		volatile int32_t wlan_priority2 = WLAN_LOW_PTI_RX_CCCONF_PTI;
		volatile int32_t wlan_priority3 = WLAN_LOW_PTI_TX_CCCONF_PTI;

		volatile int32_t sr_priority0 = SR_RX_CCCONF_PTI;     
		volatile int32_t sr_priority1 = SR_TX_CCCONF_PTI; 
		
		volatile int32_t lte_priority0 = LTE_PTI_RX_CCCONF_PTI; 
		volatile int32_t lte_priority1 = LTE_PTI_TX_CCCONF_PTI; 
		
		coexc_mode_wifi_t coexc_mode_wifi = COEXC_MODE_WIFI_5G;
			

		// CCMALLOW table prepared offline
		const uint32_t ccmallow_sep_ant[NUM_OF_CLIENTS_SUPPORTED][NUM_OF_MODES_SUPPORTED] = 
		{ 
			{0x00011111UL, 0x00222222UL,  0x0UL, 0x0UL}, // client 0: mode 0, 1, 2, 3
			{0x00001111UL, 0x00202222UL,  0x0UL, 0x0UL}, // client 1: mode 0, 1, 2, 3
			{0x00011111UL, 0x00222222UL,  0x0UL, 0x0UL}, // client 2: mode 0, 1, 2, 3
			{0x00101111UL, 0x00202222UL,  0x0UL, 0x0UL}, // client 3: mode 0, 1, 2, 3
			{0x00110101UL, 0x00220202UL,  0x0UL, 0x0UL}, // client 4: mode 0, 1, 2, 3
			{0x00111000UL, 0x00222222UL,  0x0UL, 0x0UL}, // client 5: mode 0, 1, 2, 3
			{0x11000000UL, 0x22000000UL,  0x0UL, 0x0UL}, // client 6: mode 0, 1, 2, 3
			{0x11000000UL, 0x22000000UL,  0x0UL, 0x0UL}, // client 7: mode 0, 1, 2, 3

		};
		const uint32_t ccmallow_sha_ant[NUM_OF_CLIENTS_SUPPORTED][NUM_OF_MODES_SUPPORTED] = 
		{
			
			{0x00011111UL, 0x00022222UL, 0x0UL, 0x0UL}, // client 0: mode 0, 1, 2, 3
			{0x00001111UL, 0x00202222UL, 0x0UL, 0x0UL}, // client 1: mode 0, 1, 2, 3
			{0x00011111UL, 0x00022222UL, 0x0UL, 0x0UL}, // client 2: mode 0, 1, 2, 3
			{0x00001111UL, 0x00202222UL, 0x0UL, 0x0UL}, // client 3: mode 0, 1, 2, 3
			{0x00110101UL, 0x00220202UL, 0x0UL, 0x0UL}, // client 4: mode 0, 1, 2, 3
			{0x00110000UL, 0x00222020UL, 0x0UL, 0x0UL}, // client 5: mode 0, 1, 2, 3
			{0x11000000UL, 0x22000000UL, 0x0UL, 0x0UL}, // client 6: mode 0, 1, 2, 3
			{0x11000000UL, 0x22000000UL, 0x0UL, 0x0UL}, // client 7: mode 0, 1, 2, 3
			
			//{0xffffffffUL, 0xffffffffUL, 0xffffffffUL, 0xffffffffUL}, // client 0: mode 0, 1, 2, 3
			//{0xffffffffUL, 0xffffffffUL, 0xffffffffUL, 0xffffffffUL}, // client 1: mode 0, 1, 2, 3
			//{0xffffffffUL, 0xffffffffUL, 0xffffffffUL, 0xffffffffUL}, // client 2: mode 0, 1, 2, 3
			//{0xffffffffUL, 0xffffffffUL, 0xffffffffUL, 0xffffffffUL}, // client 3: mode 0, 1, 2, 3
			//{0xffffffffUL, 0xffffffffUL, 0xffffffffUL, 0xffffffffUL}, // client 4: mode 0, 1, 2, 3
			//{0xffffffffUL, 0xffffffffUL, 0xffffffffUL, 0xffffffffUL}, // client 5: mode 0, 1, 2, 3
			//{0xffffffffUL, 0xffffffffUL, 0xffffffffUL, 0xffffffffUL}, // client 6: mode 0, 1, 2, 3
			//{0xffffffffUL, 0xffffffffUL, 0xffffffffUL, 0xffffffffUL}, // client 7: mode 0, 1, 2, 3
		};


		// Configure the arbitration logic table i.e. CCMALLOW registers
		const uint32_t (*p_ccmallow)[NUM_OF_MODES_SUPPORTED] = 
			(antenna_cfg_type == COEX_SEPARATE_ANT_CFG) ? ccmallow_sep_ant : ccmallow_sha_ant;
		
		if ((antenna_cfg_type != COEX_SEPARATE_ANT_CFG) && (antenna_cfg_type != COEX_SHARED_ANT_CFG))
		{
			printf("Wrong antenna configuration.");
			return;
		}

		for (uint32_t clients = 0u; clients < NUM_OF_CLIENTS_SUPPORTED; clients++)
		{
			const uint32_t base_client_offset = clients * OFFSET_BETWEEN_CLIENTS;
			for (uint32_t modes = 0u; modes < NUM_OF_MODES_SUPPORTED; modes++)
			{
				const uint32_t offset = (OFFSET_BETWEEN_MODES * modes) + base_client_offset;
				UCC_WRITE32(ABS_COEXC_CCMALLOW_0_MODE_0 + offset, p_ccmallow[clients][modes]);
			}
		}

		ccconf_config[0] = (wlan_priority0 << COEXC_CCCONF_0_PRIORITY_SHIFT) |
							(coexc_mode_wifi << COEXC_CCCONF_0_MODE_SHIFT);
		ccconf_config[1] = (wlan_priority1 << COEXC_CCCONF_1_PRIORITY_SHIFT) |
							(coexc_mode_wifi << COEXC_CCCONF_1_MODE_SHIFT);
		ccconf_config[2] = (wlan_priority2 << COEXC_CCCONF_2_PRIORITY_SHIFT) |
							(coexc_mode_wifi << COEXC_CCCONF_2_MODE_SHIFT);
		ccconf_config[3] = (wlan_priority3 << COEXC_CCCONF_3_PRIORITY_SHIFT) |
							(coexc_mode_wifi << COEXC_CCCONF_3_MODE_SHIFT);;
		
		ccconf_config[4] = (sr_priority0 << COEXC_CCCONF_4_PRIORITY_SHIFT) |
							(0 << COEXC_CCCONF_4_MODE_SHIFT);
		ccconf_config[5] = (sr_priority1 << COEXC_CCCONF_5_PRIORITY_SHIFT) |
							(0 << COEXC_CCCONF_5_MODE_SHIFT);

		ccconf_config[6] = (lte_priority0 << COEXC_CCCONF_6_PRIORITY_SHIFT) | /* LTE RX */
							(0 << COEXC_CCCONF_6_MODE_SHIFT);        
		ccconf_config[7] = (lte_priority1 << COEXC_CCCONF_7_PRIORITY_SHIFT) | /* LTE TX */
							(0 << COEXC_CCCONF_7_MODE_SHIFT);                                        
			

		
		for (uint32_t clients = 0; clients < NUM_OF_CLIENTS_SUPPORTED; clients++) {
			UCC_WRITE32(ABS_COEXC_CCCONF_0+(OFFSET_BETWEEN_MODES*clients),
						ccconf_config[clients]);
		}

		// Configure the TURNAROUND register i.e. turnaround values of different clients
		turnaround_config[0] = turnaround_wifi; 
		turnaround_config[1] = turnaround_wifi; 
		turnaround_config[2] = turnaround_wifi; 
		turnaround_config[3] = turnaround_wifi; 
		
		turnaround_config[4] = turnaround_sr; 
		turnaround_config[5] = turnaround_sr;
		
		turnaround_config[6] = turnaround_lte; /* LTE RX */
		turnaround_config[7] = turnaround_lte; /* LTE TX */

		for (uint32_t clients = 0u; clients < NUM_OF_CLIENTS_SUPPORTED; clients++)
		{
			UCC_WRITE32(ABS_COEXC_TURNAROUND_0 + (OFFSET_BETWEEN_MODES * clients), 
						turnaround_config[clients]);
		}
		
		
		printf("=================== Arbitration CCMALLOW values\n");
		for (uint32_t clients = 0u; clients < NUM_OF_CLIENTS_SUPPORTED; clients++)
		{
			const uint32_t base_client_offset = clients * OFFSET_BETWEEN_CLIENTS;
			for (uint32_t modes = 0u; modes < NUM_OF_MODES_SUPPORTED; modes++)
			{
				const uint32_t offset = (OFFSET_BETWEEN_MODES * modes) + base_client_offset;
				printf("address = 0x%x value = 0x%x\n",ABS_COEXC_CCMALLOW_0_MODE_0 + offset,
				                            UCC_READ32(ABS_COEXC_CCMALLOW_0_MODE_0 + offset));
			}
		}
		
		printf("=================== CCCONF  values\n");
		for (uint32_t clients = 0; clients < NUM_OF_CLIENTS_SUPPORTED; clients++) {
			;
			printf("address = 0x%x value = 0x%x\n",ABS_COEXC_CCCONF_0+(OFFSET_BETWEEN_MODES*clients),
			                            UCC_READ32(ABS_COEXC_CCCONF_0+(OFFSET_BETWEEN_MODES*clients)));
		}
		
		printf("=================== TURNAROUND  values\n");
		for (uint32_t clients = 0u; clients < NUM_OF_CLIENTS_SUPPORTED; clients++)
		{
			printf("address = 0x%x value = 0x%x\n",ABS_COEXC_TURNAROUND_0 + (OFFSET_BETWEEN_MODES * clients),
			                            UCC_READ32(ABS_COEXC_TURNAROUND_0 + (OFFSET_BETWEEN_MODES * clients)));
		}
	}

	void configure_coexistence_controller(void)
	{
		/* Configuration of COEXC */

		configure_coexc(COEX_SHARED_ANT_CFG); 

		/* Enable Coexistence Controller */
		enable_coex_in_macc(COEXC_ENABLE);
	}
	
#endif	


int main(void)
{
#if NRFX_CLOCK_ENABLED && (defined(CLOCK_FEATURE_HFCLK_DIVIDE_PRESENT) || NRF_CLOCK_HAS_HFCLK192M)
	/* For now hardcode to 128MHz */
	nrfx_clock_divider_set(NRF_CLOCK_DOMAIN_HFCLK,
			       NRF_CLOCK_HFCLK_DIV_1);
#endif
	printk("Starting %s with CPU frequency: %d MHz\n", CONFIG_BOARD, SystemCoreClock/MHZ(1));

#if USES_USB_ETH
	init_usb();

	/* Redirect static IP address to netusb*/
	const struct device *usb_dev = device_get_binding("eth_netusb");
	struct net_if *iface = net_if_lookup_by_dev(usb_dev);

	if (!iface) {
		printk("Cannot find network interface: %s", "eth_netusb");
		return -1;
	}

	net_if_ipv4_addr_add(iface, &addr, NET_ADDR_MANUAL, 0);
	net_if_ipv4_set_netmask_by_addr(iface, &addr, &mask);
#endif

#ifdef CONFIG_SLIP
	const struct device *slip_dev = device_get_binding(CONFIG_SLIP_DRV_NAME);
	struct net_if *slip_iface = net_if_lookup_by_dev(slip_dev);

	if (!slip_iface) {
		printk("Cannot find network interface: %s", CONFIG_SLIP_DRV_NAME);
		return -1;
	}

	net_if_ipv4_addr_add(slip_iface, &addr, NET_ADDR_MANUAL, 0);
	net_if_ipv4_set_netmask_by_addr(slip_iface, &addr, &mask);
#endif /* CONFIG_SLIP */

#ifdef CONFIG_NET_CONFIG_SETTINGS
	/* Without this, DHCPv4 starts on first interface and if that is not Wi-Fi or
	 * only supports IPv6, then its an issue. (E.g., OpenThread)
	 *
	 * So, we start DHCPv4 on Wi-Fi interface always, independent of the ordering.
	 */
	const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_wifi));
	struct net_if *wifi_iface = net_if_lookup_by_dev(dev);

	/* As both are Ethernet, we need to set specific interface*/
	net_if_set_default(wifi_iface);

	net_config_init_app(dev, "Initializing network");
#endif

#ifdef CONFIGURE_ENABLE_COEX
	printf("============================ Configure and enable COEXC FM \n");
	// COEXC configuration done from TB and CM tests are run from firmware.
	configure_coexistence_controller();
#endif

	return 0;
}
