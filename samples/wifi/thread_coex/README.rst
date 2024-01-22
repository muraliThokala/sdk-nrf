.. _wifi_thread_coex_sample:

Wi-Fi: Thread coexistence
###############################

.. contents::
   :local:
   :depth: 2

The Thread coexistence sample demonstrates coexistence between Wi-Fi® and openThread(OT) device radios in 2.4 GHz frequency.
The sample documentation includes details of test setup used, build procedure, test procedure and the results obtained when the sample is run on the nRF7002 DK.

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

Overview
********

The sample demonstrates how the coexistence mechanism is implemented and enabled and disabled between Wi-Fi and Thread radios in 2.4 GHz band using Wi-Fi client’s throughput and Thread client’s throughput.

Test setup
==========

The following figure shows a reference test setup.

.. figure:: /images/wifi_coex.svg
     :alt: Wi-Fi Thread Coex test setup

     Wi-Fi Thread coexistence reference test and evaluation setup

The reference test setup shows the connections between the following devices:

* :term:`Device Under Test (DUT)` (nRF7002 DK on which the coexistence sample runs)
* Wi-Fi peer device (access point with test PC that runs **iperf**)
* Thread peer device (nRF7002 DK on which Thread throughput sample runs)

This setup is kept in a shielded test enclosure box (for example, a Ramsey box).
The following table provides more details on the sample or application that runs on DUT and peer devices:

+--------------+----------------+------------------------------------------------------------------------------------+
| Device       | Application    |                             Details                                                |
+==============+================+====================================================================================+
| nRF7002 DK   | Thread         | The sample runs Wi-Fi throughputs, Thread throughputs or a combination of both     |
| (DUT)        | coex sample    | based on configuration selections in the :file:`prj.conf` file.                    |
+--------------+----------------+------------------------------------------------------------------------------------+
| Test PC      | **iperf**      | Wi-Fi **iperf** UDP server is run on the test PC, and this acts as a peer device to|
|              | application    | Wi-Fi UDP client that runs on the nRF7002 DK.                                      |
+--------------+----------------+------------------------------------------------------------------------------------+
| nRF7002 DK   | Thread         | Thread UDP throughput is run in server mode on the nRF7002 DK device, and this acts|
| (peer)       | throughput     | as a peer device to Thread client that runs on the DUT nRF7002 DK.                 |
+--------------+----------------+------------------------------------------------------------------------------------+

To trigger concurrent transmissions at RF level on both Wi-Fi and Thread, the sample runs traffic on separate threads, one for each.
The sample uses standard Zephyr threads.
The threads are configured with non-negative priority (pre-emptible thread).
For details on threads and scheduling, refer to `Threads`_.

Configuration
*************

|config|

Configuration options
=====================

The following sample-specific Kconfig options are used in this sample (located in :file:`samples/wifi/thread_coex/Kconfig`):

.. _CONFIG_COEX_SEP_ANTENNAS:

CONFIG_COEX_SEP_ANTENNAS
   This option specifies whether the antennas are shared or separate for WLAN and Thread.

.. _CONFIG_TEST_TYPE_WLAN_ONLY:

CONFIG_TEST_TYPE_WLAN_ONLY
   This option enables the WLAN test type.

.. _CONFIG_TEST_TYPE_OT_ONLY:

CONFIG_TEST_TYPE_OT_ONLY
   This option enables the Thread test type.

.. _CONFIG_TEST_TYPE_WLAN_OT:

CONFIG_TEST_TYPE_WLAN_OT
   This option enables concurrent WLAN and Thread tests.

.. _CONFIG_COEX_TEST_DURATION:

CONFIG_COEX_TEST_DURATION
   This option sets the Wi-Fi/Thread/both test duration in milliseconds.

.. _CONFIG_STA_SSID:

CONFIG_STA_SSID
   This option specifies the SSID to connect.

.. _CONFIG_STA_PASSWORD:

CONFIG_STA_PASSWORD
   This option specifies the passphrase (WPA2) or password WPA3 to connect.

.. _CONFIG_STA_KEY_MGMT_*:

CONFIG_STA_KEY_MGMT_*
   These options specify the key security option.

Configuration files
===================

To enable different test modes, set up the following configuration parameters in the :file:`prj.conf` file:

* Antenna configuration: Use the :ref:`CONFIG_COEX_SEP_ANTENNAS <CONFIG_COEX_SEP_ANTENNAS>` Kconfig option to select the antenna configuration.
  Always set it to ``y`` to enable separate antennas mode.

* Test modes: Use the following Kconfig options to select the required test case:
  * :ref:`CONFIG_TEST_TYPE_WLAN_ONLY <CONFIG_TEST_TYPE_WLAN_ONLY>` for Wi-Fi only test
  * :ref:`CONFIG_TEST_TYPE_OT_ONLY <CONFIG_TEST_TYPE_OT_ONLY>` for Thread only test
  * :ref:`CONFIG_TEST_TYPE_WLAN_OT <CONFIG_TEST_TYPE_WLAN_OT>` for concurrent Wi-Fi and Thread test.

  Based on the required test, set only one of these to ``y``.
* Test duration: Use the :ref:`CONFIG_COEX_TEST_DURATION <CONFIG_COEX_TEST_DURATION>` Kconfig option to set the duration of the Wi-Fi only test or 
  Thread only test or both.
  The units are in milliseconds.
  For example, to set the tests for 20 seconds, set the respective values to ``20000``.

* Wi-Fi connection: Set the following options appropriately as per the credentials of the access point used for this testing:

  * :ref:`CONFIG_STA_SSID <CONFIG_STA_SSID>`
  * :ref:`CONFIG_STA_PASSWORD <CONFIG_STA_PASSWORD>`
  * :ref:`CONFIG_STA_KEY_MGMT_* <CONFIG_STA_KEY_MGMT_*>`
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
* Concurrent Wi-Fi and Thread throughput (with coexistence enabled and disabled mode)

To build for the nRF7002 DK, use the ``nrf7002dk_nrf5340_cpuapp`` build target.
The following are examples of the CLI commands:

* Build with coexistence disabled:

  .. code-block:: console

	 west build -p -b nrf7002dk_nrf5340_cpuapp -- -DCONFIG_MPSL_CX=n -Dhci_rpmsg_CONFIG_MPSL_CX=n
	 

Use this command for Wi-Fi throughput only, Thread throughput only, or concurrent Wi-Fi and Thread throughput with coexistence disabled tests.

* Build with coexistence enabled:

  .. code-block:: console

	 west build -p -b nrf7002dk_nrf5340_cpuapp -- -DCONFIG_MPSL_CX=y -Dhci_rpmsg_CONFIG_MPSL_CX=y

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

     -DSHIELD=nrf7002ek -Dhci_rpmsg_SHIELD=nrf7002ek_coex

* For nRF7001 EK:

  .. code-block:: console

     -DSHIELD=nrf7002ek_nrf7001 -Dhci_rpmsg_SHIELD=nrf7002ek_nrf7001_coex

The generated HEX file to be used is :file:`thread_coex/build/zephyr/merged_domains.hex`.

************************************ PENDING Add how to generate hex file for OT client role (DUT) and server role (PEER)

Connecting to DKs
=================

After the DKs are connected to the test PC through USB connectors and powered on, open a suitable terminal, and run the following command:

.. code-block:: console

   $ nrfjprog --com
   1050779496         /dev/ttyACM0    VCOM0
   1050779496         /dev/ttyACM1    VCOM1
   1050724225         /dev/ttyACM2    VCOM0
   1050724225         /dev/ttyACM3    VCOM1
   $

In this example, ``1050779496`` is the device ID of the first nRF7002 DK and ``1050724225`` is device ID of the second nRF7002 DK.

While connecting to a particular board, use the ttyACMx corresponding to VCOM1.
In the example, use ttyACM1 to connect to the board with device ID ``1050779496``.
Similarly, use ttyACM3 to connect to the board with device ID ``1050724225``.

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
When the sample runs Wi-Fi UDP throughput in client mode, a peer device runs UDP throughput in server mode using the following command:

.. code-block:: console

   $ iperf -s -i 1 -u
   $ iperf -s -i 1 -u -p 5002                                              ************************************ PENDING 

Use **iperf** version 2.0.5.
For more details, see `Network Traffic Generator`_.

+---------------+--------------+----------------------------------------------------------------+
| Test case     | Coexistence  | Test procedure                                                 |
|               |              |                                                                |
+===============+==============+================================================================+
| Wi-Fi only    | NA           | Run Wi-Fi **iperf** in server mode on the test PC.             |
| throughput    |              | Program the coexistence sample application on the nRF7002 DK.  |
+---------------+--------------+----------------------------------------------------------------+
| Thread        | NA           | Program Thread throughput application built for server role on |
| only          |              | peer nRF7002 DK.                                               |
| throughput    |              | Program the coexistence sample application on the nRF7002 DK.  |
+---------------+--------------+----------------------------------------------------------------+
| Wi-Fi and     | Disabled/    | Run Wi-Fi **iperf** in server mode on the test PC.             |
| Thread        | Enabled      | Program Thread throughput application built for server role on |
| combined      |              | peer nRF7002 DK.                                               |
| throughput    |              | Program the coexistence sample application on the DUT          |
|               |              | nRF7002 DK.                                                    |
+---------------+--------------+----------------------------------------------------------------+

The Wi-Fi throughput result appears on the test PC terminal on which **iperf** server is run.
The Thread throughput result appears on the minicom terminal connected to the peer nRF7002 DK.

Results
=======

The following tables collect a summary of results obtained when coexistence tests are run for different Wi-Fi operating bands.
The tests are run with the test setup inside an RF shield box.
Therefore, the results are representative and might change with adjustments in the test setup.

Wi-Fi in 2.4 GHz
----------------

Separate antennas, Wi-Fi in 802.11n mode:

+------------------------+--------------------+--------------------+
| Test case              | Wi-Fi UDP Tx       | Thread             |
|                        | throughput in Mbps | throughput in kbps |
+========================+====================+====================+
| Wi-Fi only,            | 12.5               | N.A                |
| client (UDP Tx)        |                    |                    |
+------------------------+--------------------+--------------------+
| Thread only,           | N.A                | 9                  |
| client                 |                    |                    |
+------------------------+--------------------+--------------------+
| Wi-Fi and Thread,      | 12.5               | 9                  |
| coexistence disabled   |                    |                    |
+------------------------+--------------------+--------------------+
| Wi-Fi and Thread,      | 12.5               | 9                  |
| coexistence enabled    |                    |                    |
+------------------------+--------------------+--------------------+


Wi-Fi in 5 GHz
--------------

Separate antennas, Wi-Fi in 802.11n mode:

+------------------------+--------------------+--------------------+
| Test case              | Wi-Fi UDP Tx       | Thread             |
|                        | throughput in Mbps | throughput in kbps |
+========================+====================+====================+
| Wi-Fi only,            | 12.5               | N.A                |
| client (UDP Tx)        |                    |                    |
+------------------------+--------------------+--------------------+
| Thread only,           | N.A                | 9                  |
| client                 |                    |                    |
+------------------------+--------------------+--------------------+
| Wi-Fi and Thread,      | 12.5               | 9                  |
| coexistence disabled   |                    |                    |
+------------------------+--------------------+--------------------+
| Wi-Fi and Thread,      | 12.5               | 9                  |
| coexistence enabled    |                    |                    |
+------------------------+--------------------+--------------------+

.. note::
   Shared antenna configuration is not supported for Wi-Fi and Thread coexistence as Thread requires idle listening and``menuconfig`` can also be used to using shared antenna cause very high attenuation in Thread antenna path when shared antenna connected to Wi-Fi.

Sample output
=============

The following screenshots show coexistence test results obtained for separate antenna configuration with Wi-Fi mode set to 802.11n.
These tests were run with WLAN connected to an AP in 2.4 GHz band.
In the images, the top image result shows Wi-Fi throughput that appears on a test PC terminal in which Wi-Fi **iperf** server is run and the bottom image result shows Thread throughput that appears on a minicom terminal in which the Thread throughput sample is run.

.. figure:: /images/thread_coex_wlan.png
     :width: 780px
     :align: center
     :alt: Wi-Fi only throughput

     Wi-Fi only throughput 10.2 Mbps

.. figure:: /images/thread_coex_thread.png
     :width: 780px
     :align: center
     :alt: Thread only throughput

     Thread only throughput: 1107 kbps

.. figure:: /images/thread_coex_wlan_thread_cd.png
     :width: 780px
     :align: center
     :alt: Wi-Fi and Thread CD

     Wi-Fi and Thread throughput, coexistence disabled: Wi-Fi 9.9 Mbps and Thread 145 kbps

.. figure:: /images/thread_coex_wlan_thread_ce.png
     :width: 780px
     :align: center
     :alt: Wi-Fi and Thread CE

     Wi-Fi and Thread throughput, coexistence enabled: Wi-Fi 8.3 Mbps and Thread 478 kbps

As is evident from the results of the sample execution, coexistence harmonizes air-time between Wi-Fi and Thread rather than resulting in a higher combined throughput.
This is consistent with the design intent.

Dependencies
************

This sample uses the following library:

* :ref:`nrf_security`
