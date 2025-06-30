.. _wifi_thread_coex_sample:

Wi-Fi: Thread coexistence
#########################

.. contents::
   :local:
   :depth: 2

The Thread coexistence sample demonstrates coexistence between Wi-Fi® and OpenThread device radios in 2.4 GHz frequency.

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

Running test cases for this sample requires additional software, such as the Wi-Fi **iPerf** application.
Use iPerf version 2.0.5.
For more details, see `Network Traffic Generator`_.

Overview
********

The sample demonstrates how the coexistence mechanism is implemented, and how it can be enabled and disabled between Wi-Fi and Thread radios in the 2.4 GHz band.
This is done by using the throughput of the Wi-Fi client and the Thread client, as well as the throughput of the Wi-Fi client and the Thread server.

Test setup
==========

The following figure shows a reference test setup.

.. figure:: /images/wifi_thread_coex.svg
     :alt: Wi-Fi and Thread Coex test setup

     Wi-Fi and Thread coexistence reference test and evaluation setup

The reference test setup shows the connections between the following devices:

* :term:`Device Under Test (DUT)` (nRF7002 DK on which the coexistence sample runs)
* Wi-Fi peer device (access point with test PC that runs iPerf)
* Thread peer device (nRF7002 DK on which Thread-only throughput runs)

The following table provides more details on the sample or application that runs on DUT and peer devices:

+--------------+-------------------+-------------------------------------------------------------------------------------+
| Device       | Application       |                             Details                                                 |
+==============+===================+=====================================================================================+
| nRF7002 DK   | Thread            | The sample runs Wi-Fi throughput only, Thread throughput only, or a combination     |
| (DUT)        | coexistence sample| of both.                                                                            |
+--------------+-------------------+-------------------------------------------------------------------------------------+
| Test PC      | iPerf             | Wi-Fi iPerf UDP server is run on the test PC, and this acts as a peer device to     |
|              | application       | the Wi-Fi UDP client.                                                               |
+--------------+-------------------+-------------------------------------------------------------------------------------+
| nRF7002 DK   | Thread only       | Case 1: Thread-only UDP throughput is run in server mode on the peer nRF7002 DK     |
| (peer)       | throughput using  | device if Thread role on DUT is a client.                                           |
|              | Thread coexistence|                                                                                     |
|              | sample            | Case 2: Thread-only UDP throughput is run in client mode on the peer nRF7002 DK     |
|              |                   | device if Thread role on DUT is a server.                                           |
+--------------+-------------------+-------------------------------------------------------------------------------------+

Configuration
*************

|config|

Configuration options
=====================

The following sample-specific Kconfig options are used in this sample (located in :file:`samples/wifi/thread_coex/Kconfig`):

.. options-from-kconfig::
   :show-type:

Building and running
********************

.. |sample path| replace:: :file:`samples/wifi/thread_coex`

.. include:: /includes/build_and_run_ns.txt

To build for the nRF7002 DK, use the ``nrf7002dk/nrf5340/cpuapp`` board target.
The following are examples of the CLI commands:

* Build with coexistence disabled:

  .. code-block:: console

     west build -p -b nrf7002dk/nrf5340/cpuapp -- -DCONFIG_MPSL_CX=n -Dipc_radio_CONFIG_MPSL_CX=n

Use this command to run the tests with coexistence disabled mode.

* Build with coexistence enabled:

  .. code-block:: console

     west build -p -b nrf7002dk/nrf5340/cpuapp -- -DCONFIG_MPSL_CX=y -Dipc_radio_CONFIG_MPSL_CX=y

Use this command to run the tests with coexistence enabled mode.

Change the board target as given below for the nRF7001 DK, nRF7002 EK, and nRF7001 EK.

* Board target for nRF7001 DK:

  .. code-block:: console

     nrf7002dk/nrf5340/cpuapp/nrf7001

* Board target for nRF7002 EK and nRF7001 EK:

  .. code-block:: console

     nrf5340dk/nrf5340/cpuapp

Add the following SHIELD options for the nRF7002 EK and nRF7001 EK.

* For nRF7002 EK:

  .. code-block:: console

     -Dthread_coex_SHIELD="nrf7002ek;nrf7002ek_coex"
     -Dipc_radio_SHIELD="nrf7002ek_coex"

* For nRF7001 EK:

  .. code-block:: console

     -Dthread_coex_SHIELD="nrf7002ek_nrf7001;nrf7002ek_coex"
     -Dipc_radio_SHIELD="nrf7002ek_coex"

* Overlay files

   * Use the :file:`overlay-wifi-udp-client-thread-udp-client.conf` file to build for both Wi-Fi and Thread in client roles.
   * Use the :file:`overlay-wifi-udp-client-thread-udp-server.conf` file to build for Wi-Fi in the client role and Thread in the server role.

The generated HEX file to be used is :file:`thread_coex/build/merged.hex`.

Supported CLI commands
======================
*Coexistence configuration:

	``coex_cfg_sr_switch`` is the command to configure the SR side switch.
	Usage: coex_cfg_sr_switch <is_sep_antennas>
             is_sep_antennas: 0 for shared antenna mode and 1 for separate antennas mode.
	Example: coex_cfg_sr_switch 1

	``coex_config_pta`` is the command to configure the Packet Traffic Arbiter (PTA).
	Usage: coex_config_pta <wifi_band> <is_sep_antennas> <is_sr_ble>
			 wifi_band: 0 for 2.4GHz, and 1 for 5GHz.
			 is_sep_antennas: 0 for shared antenna mode, and 1 for separate antennas mode.
			 is_sr_ble: 0 for Thread coexistence, and 1 for Bluetooth LE coexistence.
	Example: coex_config_pta 0 1 0

*Thread throughput configuration:

	``ot_cfg_tput`` is the command to configure the Thread throughput configuration.
	Usage: ot_cfg_tput <is_ot_client> <is_ot_zperf_udp>
			is_ot_client: 0 for server, and 1 for client.
			is_ot_zperf_udp: 0 for TCP, and 1 for UDP.
	Example: ot_cfg_tput 0 1

*Wi-Fi connection
	``wifi connect`` is the command to connect to the access point of interest.
	Usage: wifi connect -s <SSID> -k <KeyManagement> -p <Passphrase>
			SSID: SSID of the access point
			KeyManagement: KeyManagement type for secured SSIDs. 0:None, 1:WPA2-PSK
			Passphrase: Passphrase for secure SSIDs only.
	Example: wifi connect -s wifi_sr_coex_24 -k 1 -p 12345678

*Wi-Fi throughput:
	``wifi_run_tput`` is the command to run the Wi-Fi throughput.
	Usage: wifi_run_tput <protocol> <direction> <peerIP> <destPort> <duration> <pktSize> <baudrate>
			protocol: udp or tcp.
			direction: upload or download.
			peerIP: IP address of the peer device.
			destPort: port of the peer device.
			duration: test duration in seconds.
			pktSize: in bytes or kilobyte (with suffix K).
			baudrate: baudrate in kilobyte/megabyte (with suffix K/M).
	Example: wifi_run_tput udp upload 192.168.1.253 5001 60 1K 10M

*Thread throughput:
	``ot_run_tput`` is the command to run the Thread throughput.
	Usage: ot_run_tput <is_ot_client> <is_ot_zperf_udp> <test_duration>
			is_ot_client: 0 for server, and 1 for client.
			is_ot_zperf_udp: 0 for TCP, and 1 for UDP.
			test_duration: test duration in seconds.
	Example: ot_run_tput 1 1 60

.. note::
	You can use the Tab key to autocomplete commands and enter the command for usage details.
	Set the same duration for Wi-Fi throughput and Bluetooth LE throughput tests.

Testing
=======

|test_sample|

1. |connect_kit|
#. |connect_terminal|

#. Run the following command to check the available devices:

   .. code-block:: console

      nrfutil device list

   This command returned the following output in the setup used to run the coexistence tests.

   .. code-block:: console

      1050779496
      product         J-Link
      board version   PCA10095
      ports           /dev/ttyACM0, vcom: 0
                      /dev/ttyACM1, vcom: 1
      traits          devkit, jlink, seggerUsb, serialPorts, usb

      1050759502
      product         J-Link
      board version   PCA10143
      ports           /dev/ttyACM2, vcom: 0
                      /dev/ttyACM3, vcom: 1
      traits          devkit, jlink, seggerUsb, serialPorts, usb

      Found 2 supported device(s)


   In this example, ``1050779496`` is the serial number of the first nRF7002 DK and ``1050759502`` is the serial number of the other one.

   While connecting to a particular device, use the ``/dev/ttyACM`` corresponding to VCOM1.
   In the example, use ``/dev/ttyACM1`` to connect to the device with the serial number ``1050779496``.
   Similarly, use ``/dev/ttyACM3`` to connect to the device with the serial number ``1050759502``.

#. Run the following commands to connect to the desired devices:

   .. code-block:: console

      minicom -D /dev/ttyACM1 -b 115200
      minicom -D /dev/ttyACM3 -b 115200

Programming DKs
===============

Complete the following steps to program the nRF7002 DK:

1. |open_terminal_window_with_environment|
#. Navigate to the :file:`<ncs code>/nrf/samples/wifi/thread_coex/` folder.
#. Run the following command:

   .. code-block:: console

      west flash --dev-id <device-id> --hex-file build/merged.hex

Test procedure
==============

The following table provides the procedure to run Wi-Fi-only, Thread-only, and combined throughput.

#. Wi-Fi client and Thread client:

+---------------+--------------+----------------------------------------------------------------+
| Test case     | Coexistence  | Test procedure                                                 |
+===============+==============+================================================================+
| Wi-Fi-only    | Disabled     | Run Wi-Fi iPerf using the command "iperf -s -i 1 -u" on the    |
| throughput    |              | test PC. This runs Wi-Fi iperf in server mode.                 |
|               |              | Build the coexistence sample for coexistence disabled mode and |
|               |              | program the application on the DUT nRF7002 DK.                 |
|               |              | Do the following as described under "Supported CLI commands"   |
|               |              | section.                                                       |
|               |              | Do the coexistence configuration on the DUT.                   |
|               |              | Connect the DUT to the access point of interest using the      |
|               |              | command ``wifi connect`` and wait for status "Connected".      |
|               |              | Run Wi-Fi UDP throughput client (upload) on the DUT using the  |
|               |              | command ``wifi_run_tput``.                                     |
+---------------+--------------+----------------------------------------------------------------+
| Thread-only   | Disabled     | Build the sample for coexistence disabled with Thread role set |
| throughput    |              | to server, and program the peer nRF7002 DK.                    |
|               |              | Build the sample for coexistence disabled and Thread role set  |
|               |              | client, and program the DUT nRF7002 DK.                        |
|               |              | Do the following as described under "Supported CLI commands"   |
|               |              | section.                                                       |
|               |              | Thread throughput configuration on peer with Thread role set to|
|               |              | server and zperf UDP and wait until **Run thread application on|
|               |              | client** is seen on the peer nRF7002 DK's UART console window. |
|               |              | coexistence configuration" and Thread throughput configuration |
|               |              | on the DUT with Thread role set to client and zperf UDP.       |
|               |              | Thread connection happens in this process.                     |
|               |              | Run Thread throughput on the DUT using the command             |
|               |              | ``ot_run_tput`` with role set to client and zperf UDP and wait |
|               |              | for the results.                                               |
+---------------+--------------+----------------------------------------------------------------+
| Wi-Fi and     | Disabled/    | Run Wi-Fi iPerf using the command "iperf -s -i 1 -u" on the    |
| Thread        | Enabled      | test PC. This runs Wi-Fi iperf in server mode.                 |
| combined      |              | Build the sample for coexistence disabled and Thread role set  |
| throughput    |              | to server, and program the peer nRF7002 DK.                    |
|               |              | Build the coexistence sample for coexistence disabled mode and |
|               |              | Thread role set to client, and program the application on the  |
|               |              | DUT nRF7002 DK.                                                |
|               |              | Do the following as described under "Supported CLI commands"   |
|               |              | section.                                                       |
|               |              | Thread throughput configuration on peer with role set to server|
|               |              | and zperf UDP, and wait until **Run thread application on      |
|               |              | client** is seen on the peer nRF7002 DK's UART console window. |
|               |              | Coexistence configuration and Thread throughput configuration  |
|               |              | on the DUT with Thread role set to client and zperf UDP        |
|               |              | Thread connection happens in this process.                     |
|               |              | Connect the DUT to the Wi-Fi access point of interest using the|
|               |              | command ``wifi connect`` and wait for status "connected".      |
|               |              | Run Wi-Fi UDP throughput client (upload) on the DUT using the  |
|               |              | command ``wifi_run_tput``.                                     |
|               |              | Run Thread throughput on the DUT using the command             |
|               |              | ``ot_run_tput`` with role set to client and zperf UDP and wait |
|               |              | for the results.                                               |
|               |              |                                                                |
|               |              | Repeat the above procedure by building the coexistence sample  |
|               |              | for coexistence enabled mode.                                  |
+---------------+--------------+----------------------------------------------------------------+

#. Wi-Fi client and Thread server:

Test procedure is like the Wi-Fi client and Thread client described above except that the thread role set to server on the DUT and client on the peer.

Observe that the Wi-Fi throughput result appears on the test PC terminal on which iPerf server is run.
The Thread throughput result appears on the minicom terminal connected to the nRF7002 DK, on which Thread is run in the client role.

Results
=======

The following tables summarize the results obtained from coexistence tests conducted in a clean RF environment for different Wi-Fi operating bands, with Wi-Fi and Thread data rates set to 10 Mbps and 65 kbps, respectively.
These results are representative and might vary based on the RSSI and the level of external interference.

The Wi-Fi and Thread channels (both in the 2.4 GHz band) are selected to overlap, thereby maximizing interference.
Tests are run using separate antenna configurations only, as Thread does not support shared antenna configurations due to being in idle listening mode when not active.

Wi-Fi (802.11n mode) in 2.4 GHz
-------------------------------

Wi-Fi in client role (channel 11), Thread in client role (channel 22):

+------------------------+--------------------+--------------------+
| Test case              | Wi-Fi UDP TX       | Thread             |
|                        | throughput in Mbps | throughput in kbps |
+========================+====================+====================+
| Wi-Fi-only             | 9.6                | N.A.               |
|                        |                    |                    |
+------------------------+--------------------+--------------------+
| Thread-only            | N.A.               | 63                 |
|                        |                    |                    |
+------------------------+--------------------+--------------------+
| Wi-Fi and Thread,      | 9.5                | 14                 |
| coexistence disabled   |                    |                    |
+------------------------+--------------------+--------------------+
| Wi-Fi and Thread,      | 9.3                | 59                 |
| coexistence enabled    |                    |                    |
+------------------------+--------------------+--------------------+

Wi-Fi in client role (channel 11), Thread in server role (channel 22):

+------------------------+--------------------+--------------------+
| Test case              | Wi-Fi UDP TX       | Thread             |
|                        | throughput in Mbps | throughput in kbps |
+========================+====================+====================+
| Wi-Fi-only             | 9.6                | N.A.               |
|                        |                    |                    |
+------------------------+--------------------+--------------------+
| Thread-only            | N.A.               | 63                 |
|                        |                    |                    |
+------------------------+--------------------+--------------------+
| Wi-Fi and Thread,      | 9.4                | 14                 |
| coexistence disabled   |                    |                    |
+------------------------+--------------------+--------------------+
| Wi-Fi and Thread,      | 8.8                | 63                 |
| coexistence enabled    |                    |                    |
+------------------------+--------------------+--------------------+


Wi-Fi (802.11n mode) in 5 GHz
-----------------------------

Wi-Fi in client role (channel 48), Thread in client role (channel 22):

+------------------------+--------------------+--------------------+
| Test case              | Wi-Fi UDP TX       | Thread             |
|                        | throughput in Mbps | throughput in kbps |
+========================+====================+====================+
| Wi-Fi-only             | 9.6                | N.A.               |
|                        |                    |                    |
+------------------------+--------------------+--------------------+
| Thread-only            | N.A.               | 63                 |
|                        |                    |                    |
+------------------------+--------------------+--------------------+
| Wi-Fi and Thread,      | 9.2                | 63                 |
| coexistence disabled   |                    |                    |
+------------------------+--------------------+--------------------+
| Wi-Fi and Thread,      | 9.1                | 63                 |
| coexistence enabled    |                    |                    |
+------------------------+--------------------+--------------------+

Wi-Fi in client role (channel 48), Thread in server role (channel 22):

+------------------------+--------------------+--------------------+
| Test case              | Wi-Fi UDP TX       | Thread             |
|                        | throughput in Mbps | throughput in kbps |
+========================+====================+====================+
| Wi-Fi-only             | 9.6                | N.A.               |
|                        |                    |                    |
+------------------------+--------------------+--------------------+
| Thread-only            | N.A.               | 63                 |
|                        |                    |                    |
+------------------------+--------------------+--------------------+
| Wi-Fi and Thread,      | 8.6                | 63                 |
| coexistence disabled   |                    |                    |
+------------------------+--------------------+--------------------+
| Wi-Fi and Thread,      | 8.5                | 63                 |
| coexistence enabled    |                    |                    |
+------------------------+--------------------+--------------------+

The results show that coexistence harmonizes airtime between Wi-Fi and Thread rather than resulting in a higher combined throughput.
This is consistent with the design intent.

Dependencies
************

This sample uses the following library:

* :ref:`nrf_security`
