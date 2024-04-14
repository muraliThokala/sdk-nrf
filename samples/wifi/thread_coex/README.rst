.. _wifi_thread_coex_sample:

Wi-Fi: Thread coexistence
###############################

.. contents::
   :local:
   :depth: 2

The Thread coexistence sample demonstrates coexistence between Wi-Fi® and openThread(OT) device radios in 2.4 GHz frequency.
The sample documentation includes details of test setup, build procedure, test procedure and the results obtained when the sample is run on the nRF7002 DK.

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

Overview
********

The sample demonstrates how the coexistence mechanism is implemented and enabled and disabled between Wi-Fi and Thread radios in 2.4 GHz band using Wi-Fi client’s throughput and Thread client’s throughput, Wi-Fi client’s throughput and Thread server’s throughput.

Test setup
==========

The following figure shows a reference test setup.

.. figure:: /images/wifi_thread_coex.svg
     :alt: Wi-Fi Thread Coex test setup

     Wi-Fi Thread coexistence reference test and evaluation setup

The reference test setup shows the connections between the following devices:

* :term:`Device Under Test (DUT)` (nRF7002 DK on which the coexistence sample runs)
* Wi-Fi peer device (access point with test PC that runs **iperf**)
* Thread peer device (nRF7002 DK on which Thread throughput runs)

The following table provides more details on the sample or application that runs on DUT and peer devices:

+--------------+----------------+------------------------------------------------------------------------------------+
| Device       | Application    |                             Details                                                |
+==============+================+====================================================================================+
| nRF7002 DK   | Thread         | The sample runs Wi-Fi throughputs, Thread throughputs or a combination of both     |
| (DUT)        | coex sample    | based on configuration selections in the :file:`prj.conf` file.                    |
+--------------+----------------+------------------------------------------------------------------------------------+
| Test PC      | **iperf**      | Wi-Fi **iperf** UDP server is run on the test PC, and this acts as a peer device to|
|              | application    | Wi-Fi UDP client.                                                                  |
+--------------+----------------+------------------------------------------------------------------------------------+
| nRF7002 DK   | Thread         | case1: Thread UDP throughput is run in server mode on the peer nRF7002 DK device,  |
| (peer)       | throughput     | and this acts as peer device to Thread client that runs on the DUT nRF7002 DK.     |
|              |                | case2: Thread UDP throughput is run in client mode on the peer nRF7002 DK device,  |
|              |                | and this acts as peer device to Thread server that runs on the DUT nRF7002 DK.     |
+--------------+----------------+------------------------------------------------------------------------------------+

Configuration
*************

|config|

Configuration options
=====================

The following sample-specific Kconfig options are used in this sample (located in :file:`samples/wifi/thread_coex/Kconfig`):

.. _CONFIG_COEX_SEP_ANTENNAS:

CONFIG_COEX_SEP_ANTENNAS
   This option specifies whether the antennas are shared or separate for Wi-Fi and Thread.

.. _CONFIG_TEST_TYPE_WLAN_ONLY:

CONFIG_TEST_TYPE_WLAN_ONLY
   This option enables the Wi-Fi test type.

.. _CONFIG_TEST_TYPE_OT_ONLY:

CONFIG_TEST_TYPE_OT_ONLY
   This option enables the Thread test type.

.. _CONFIG_TEST_TYPE_WLAN_OT:

CONFIG_TEST_TYPE_WLAN_OT
   This option enables concurrent Wi-Fi and Thread tests.

.. _CONFIG_COEX_TEST_DURATION:

CONFIG_COEX_TEST_DURATION
   This option sets the Wi-Fi/Thread/both test duration in milliseconds.

.. _CONFIG_STA_SSID:

CONFIG_STA_SSID
   This option specifies the SSID of Wi-Fi access point to connect.

.. _CONFIG_STA_PASSWORD:

CONFIG_STA_PASSWORD
   This option specifies the Wi-Fi passphrase (WPA2) or password WPA3 to connect.

.. _CONFIG_STA_KEY_MGMT_*:

CONFIG_STA_KEY_MGMT_*
   These options specify the Wi-Fi key security option.

Configuration files
===================

To enable different test modes, set up the following configuration parameters in the :file:`prj.conf` file:

* Antenna configuration: Use the :ref:`CONFIG_COEX_SEP_ANTENNAS <CONFIG_COEX_SEP_ANTENNAS>` Kconfig option to select the antenna configuration.
  Always set it to ``y`` to enable separate antennas mode. Thread doesn't support shared antenna configuration as it is in "idle listening" mode when not active.

* Test modes: Use the following Kconfig options to select the required test case:
  * :ref:`CONFIG_TEST_TYPE_WLAN_ONLY <CONFIG_TEST_TYPE_WLAN_ONLY>` for Wi-Fi only test
  * :ref:`CONFIG_TEST_TYPE_OT_ONLY <CONFIG_TEST_TYPE_OT_ONLY>` for Thread only test
  * :ref:`CONFIG_TEST_TYPE_WLAN_OT <CONFIG_TEST_TYPE_WLAN_OT>` for concurrent Wi-Fi and Thread test.

* Test duration: Use the :ref:`CONFIG_COEX_TEST_DURATION <CONFIG_COEX_TEST_DURATION>` Kconfig option to set the duration of the Wi-Fi only test or
  Thread only test or both.
  The units are in milliseconds.
  For example, to set the test for 20 seconds, set this value to ``20000``.

* Wi-Fi connection: Set the following options appropriately as per the credentials of the access point used for this testing:

  * :ref:`CONFIG_STA_SSID <CONFIG_STA_SSID>`
  * :ref:`CONFIG_STA_PASSWORD <CONFIG_STA_PASSWORD>`
  * :ref:`CONFIG_STA_KEY_MGMT_* <CONFIG_STA_KEY_MGMT_*>`

* Wi-Fi throughput test: Set the following option appropriately as per the IP address of the test PC on which iperf is run:
  * :kconfig:option:`CONFIG_NET_CONFIG_PEER_IPV4_ADDR`

.. note::
   ``menuconfig`` can also be used to enable the ``Key management`` option.

See :ref:`zephyr:menuconfig` in the Zephyr documentation for instructions on how to run ``menuconfig``.

Building and running
********************

.. |sample path| replace:: :file:`samples/wifi/thread_coex`

.. include:: /includes/build_and_run_ns.txt

The sample can be built for the following configurations:

* Wi-Fi throughput only
* Thread throughput only
* Concurrent Wi-Fi and Thread throughput (with coexistence enabled and disabled modes)

To build for the nRF7002 DK, use the ``nrf7002dk_nrf5340_cpuapp`` build target.
The following are examples of the CLI commands:

* Build with coexistence disabled:

  .. code-block:: console

	 west build -p -b nrf7002dk_nrf5340_cpuapp -- -DCONFIG_MPSL_CX=n -D802154_rpmsg_CONFIG_MPSL_CX=n


Use this command for Wi-Fi throughput only, Thread throughput only, or concurrent Wi-Fi and Thread throughput with coexistence disabled tests.

* Build with coexistence enabled:

  .. code-block:: console

	 west build -p -b nrf7002dk_nrf5340_cpuapp -- -DCONFIG_MPSL_CX=y -D802154_rpmsg_CONFIG_MPSL_CX=y

Use this command for concurrent Wi-Fi and Thread throughput with coexistence enabled test.

Change the build target as given below for the nRF7001 DK, nRF7002 EK and nRF7001 EK.

* Build target for nRF7001 DK:

  .. code-block:: console

     nrf7002dk_nrf7001_nrf5340_cpuapp

* Build target for nRF7002 EK and nRF7001 EK:

  .. code-block:: console

     nrf5340dk_nrf5340_cpuapp

Add the following SHIELD options for the nRF7002 EK and nRF7001 EK.

* For nRF7002 EK:

  .. code-block:: console

     -DSHIELD=nrf7002ek -D802154_rpmsg_SHIELD=nrf7002ek_coex

* For nRF7001 EK:

  .. code-block:: console

     -DSHIELD=nrf7002ek_nrf7001 -D802154_rpmsg_SHIELD=nrf7002ek_nrf7001_coex

The generated HEX file to be used is :file:`thread_coex/build/zephyr/merged_domains.hex`.

* Overlay files
Use "overlay-openthread.conf overlay-wifi-udp-client-thread-udp-client.conf" to build for both Wi-Fi and Thread in client roles.
Use "overlay-openthread.conf overlay-wifi-udp-client-thread-udp-server.conf" to build for Wi-Fi in client role and Thread in server role.

Connecting to DKs
=================

After the DKs are connected to the test PC through USB connectors and powered on, open a suitable terminal, and run the following command:

.. code-block:: console

   $ nrfjprog --com
   1050779496         /dev/ttyACM0    VCOM0
   1050779496         /dev/ttyACM1    VCOM1
   1050759502         /dev/ttyACM2    VCOM0
   1050759502         /dev/ttyACM3    VCOM1
   $

In this example, ``1050779496`` is the device ID of the first nRF7002 DK and ``1050759502`` is device ID of the second nRF7002 DK.

While connecting to a particular board, use the ttyACMx corresponding to VCOM1.
In the example, use ttyACM1 to connect to the board with device ID ``1050779496``.
Similarly, use ttyACM3 to connect to the board with device ID ``1050759502``.

.. code-block:: console

   $ minicom -D /dev/ttyACM1 -b 115200
   $ minicom -D /dev/ttyACM3 -b 115200

Programming DKs
===============

To program the nRF7002 DK:

1. Open a new terminal in the test PC.
#. Navigate to :file:`<ncs code>/nrf/samples/wifi/thread_coex/`.
#. Run the following command:

   .. code-block:: console

      $ west flash --dev-id <device-id> --hex-file build/zephyr/merged_domains.hex

Testing
=======

Running coexistence sample test cases require additional software such as the Wi-Fi **iperf** application.
When the sample runs Wi-Fi UDP throughput in client mode, a peer device (test PC) runs UDP throughput in server mode using the following command:

.. code-block:: console

   $ iperf -s -i 1 -u

Use **iperf** version 2.0.5.
For more details, see `Network Traffic Generator`_.

Test procedure to run Wi-Fi client + Thread client
+---------------+-------------+--------------------------------------------------------------------+
| Test case     | Coexistence | Test procedure                                                     |
+===============+=============+====================================================================+
| Wi-Fi only    | NA          | Run Wi-Fi **iperf** in server mode on the test PC.                 |
| throughput    |             | Build the coexistence sample for Wi-Fi only throughput in client   |
|               |             | role and program the DUT nRF7002 DK.                               |
+---------------+-------------+--------------------------------------------------------------------+
| Thread        | NA          | Build the coexistence sample for thread only throughput in server  |
| only          |             | role and program the peer nRF7002 DK.                              |
| throughput    |             | Build the coexistence sample for thread only throughput in client  |
|               |             | role and program the DUT nRF7002 DK after "Run thread application  |
|               |             | on client" is seen on the peer nRF7002 DK's UART console window.   |
+---------------+-------------+--------------------------------------------------------------------+
| Wi-Fi and     | Disabled/   | Build the coexistence sample for thread only throughput in server  |
| Thread        | Enabled     | role and program the peer nRF7002 DK.                              |
| combined      |             | Run Wi-Fi **iperf** in server mode on the test PC.                 |
| throughput    |             | Build the coexistence sample for concurrent Wi-Fi and Thread       |
|               |             | throughput with both Wi-Fi and Thread in client roles. Program this|
|               |             | on the DUT nRF7002 DK after "Run thread application on client" is  |
|               |             | seen on the peer nRF7002 DK's UART console window.                 |
+---------------+-------------+--------------------------------------------------------------------+

Test procedure to run Wi-Fi client + Thread server
+---------------+-------------+--------------------------------------------------------------------+
| Test case     | Coexistence | Test procedure                                                     |
+===============+=============+====================================================================+
| Wi-Fi only    | NA          | Run Wi-Fi **iperf** in server mode on the test PC.                 |
| throughput    |             | Build the coexistence sample for Wi-Fi only throughput in client   |
|               |             | role and program the DUT nRF7002 DK.                               |
+---------------+-------------+--------------------------------------------------------------------+
| Thread        | NA          | Build the coexistence sample for thread only throughput in server  |
| only          |             | role and program the DUT nRF7002 DK.                               |
| throughput    |             | Build the coexistence sample for thread only throughput in client  |
|               |             | role and program the peer nRF7002 DK after "Run thread application |
|               |             | on client" is seen on the DUT nRF7002 DK's UART console window.    |
+---------------+-------------+--------------------------------------------------------------------+
| Wi-Fi and     | Disabled/   | Build the coexistence sample for concurrent Wi-Fi and Thread       |
| Thread        | Enabled     | throughput with Wi-Fi in client role and Thread in server role,    |
| combined      |             | program this on the DUT nRF7002 DK.                                |
| throughput    |             | Run Wi-Fi **iperf** in server mode on the test PC.                 |
|               |             | Build the coexistence sample for thread only throughput in client  |
|               |             | role and program the peer nRF7002 DK after "Run thread application |
|               |             | on client" is seen on the DUT nRF7002 DK's UART console window.    |
+---------------+-------------+--------------------------------------------------------------------+

The Wi-Fi throughput result appears on the test PC terminal on which **iperf** server is run.
The Thread throughput result appears on the minicom terminal connected to the nRF7002 DK on which Thread is run in client role.

Results
=======

The following tables collect a summary of results obtained when coexistence tests are run for different Wi-Fi operating bands with
Wi-Fi and Thread data rates set to 10Mbps and 65kbps respectively.
The results are representative and might change based on the RSSI and the level of external interference.

Wi-Fi in 2.4 GHz
----------------

Separate antennas, Wi-Fi in 802.11n mode, Thread in client role:

+------------------------+--------------------+--------------------+
| Test case              | Wi-Fi UDP Tx       | Thread             |
|                        | throughput in Mbps | throughput in kbps |
+========================+====================+====================+
| Wi-Fi only,            | 9.2                | N.A                |
| client (UDP Tx)        |                    |                    |
+------------------------+--------------------+--------------------+
| Thread only,           | N.A                | 63                 |
| client (UDP Tx)        |                    |                    |
+------------------------+--------------------+--------------------+
| Wi-Fi and Thread,      | 9.2                | 14                 |
| coexistence disabled   |                    |                    |
+------------------------+--------------------+--------------------+
| Wi-Fi and Thread,      | 7.8                | 58                 |
| coexistence enabled    |                    |                    |
+------------------------+--------------------+--------------------+

Separate antennas, Wi-Fi in 802.11n mode, Thread in server role:

+------------------------+--------------------+--------------------+
| Test case              | Wi-Fi UDP Tx       | Thread             |
|                        | throughput in Mbps | throughput in kbps |
+========================+====================+====================+
| Wi-Fi only,            | 9.8                | N.A                |
| client (UDP Tx)        |                    |                    |
+------------------------+--------------------+--------------------+
| Thread only,           | N.A                | 63                 |
| server (UDP Rx)        |                    |                    |
+------------------------+--------------------+--------------------+
| Wi-Fi and Thread,      | 9.1                | 20                 |
| coexistence disabled   |                    |                    |
+------------------------+--------------------+--------------------+
| Wi-Fi and Thread,      | 9.0                | 54                 |
| coexistence enabled    |                    |                    |
+------------------------+--------------------+--------------------+


Wi-Fi in 5 GHz
--------------

Separate antennas, Wi-Fi in 802.11n mode, Thread in client role:

+------------------------+--------------------+--------------------+
| Test case              | Wi-Fi UDP Tx       | Thread             |
|                        | throughput in Mbps | throughput in kbps |
+========================+====================+====================+
| Wi-Fi only,            | 9.9                | N.A                |
| client (UDP Tx)        |                    |                    |
+------------------------+--------------------+--------------------+
| Thread only,           | N.A                | 39                 |
| client (UDP Tx)        |                    |                    |
+------------------------+--------------------+--------------------+
| Wi-Fi and Thread,      | 9.8                | 37                 |
| coexistence disabled   |                    |                    |
+------------------------+--------------------+--------------------+
| Wi-Fi and Thread,      | 9.8                | 37                 |
| coexistence enabled    |                    |                    |
+------------------------+--------------------+--------------------+

Separate antennas, Wi-Fi in 802.11n mode, Thread in server role:

+------------------------+--------------------+--------------------+
| Test case              | Wi-Fi UDP Tx       | Thread             |
|                        | throughput in Mbps | throughput in kbps |
+========================+====================+====================+
| Wi-Fi only,            | 9.9                | N.A                |
| client (UDP Tx)        |                    |                    |
+------------------------+--------------------+--------------------+
| Thread only,           | N.A                | 39                 |
| server (UDP Rx)        |                    |                    |
+------------------------+--------------------+--------------------+
| Wi-Fi and Thread,      | 9.7                | 39                 |
| coexistence disabled   |                    |                    |
+------------------------+--------------------+--------------------+
| Wi-Fi and Thread,      | 9.7                | 39                 |
| coexistence enabled    |                    |                    |
+------------------------+--------------------+--------------------+

As is evident from the results of the sample execution, coexistence harmonizes air-time between Wi-Fi and Thread rather than resulting in a higher combined throughput.
This is consistent with the design intent.

Dependencies
************

This sample uses the following library:

* :ref:`nrf_security`
