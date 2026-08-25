/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdbool.h>

#include <zephyr/sys/printk.h>

#include "coex_tb_stats.h"

static void coex_tb_print_patch_stats(const struct cm_fsm_patch_stats_t *p)
{
	printk("\n==================== Patch Command Counts (cm_fsm_patch.c) ====================\n");
	printk("  cm_coex_process_cmd_cnt_patch:      %u\n", p->cm_coex_process_cmd_cnt_patch);
	printk("  cmd_update_coex_params_cnt_patch:   %u\n", p->cmd_update_coex_params_cnt_patch);
	printk("  cmd_update_user_params_cnt_patch:   %u\n", p->cmd_update_user_params_cnt_patch);
	printk("  cmd_enable_coex_cnt_patch:          %u\n", p->cmd_enable_coex_cnt_patch);
	printk("  cmd_allocate_ppw_cnt_patch:         %u\n", p->cmd_allocate_ppw_cnt_patch);
	printk("  cmd_set_pti_ranges_cnt_patch:       %u\n", p->cmd_set_pti_ranges_cnt_patch);
	printk("  cmd_get_stats_cnt_patch:            %u\n", p->cmd_get_stats_cnt_patch);
	printk("  cmd_wifi_sw_client_req_cnt_patch:   %u\n", p->cmd_wifi_sw_client_req_cnt_patch);
}

static void coex_tb_print_stats(const struct cm_stats_t *s,
				const struct cm_fsm_patch_stats_t *patch_stats)
{
	printk("\n==================== PPW Statistics (ROM 1.0) ====================\n");
	printk("  init_count:                %u\n", s->init_count);
	printk("  ppw_start_msgs_cnt:        %u\n", s->ppw_start_msgs_cnt);
	printk("  ppw_stop_msgs_cnt:         %u\n", s->ppw_stop_msgs_cnt);
	printk("  ppw_stop_timeout_cnt:      %u\n", s->ppw_stop_timeout_cnt);
	printk("  num_wifi_pti_windows:      %u\n", s->num_wifi_pti_windows);
	printk("  num_sr_pti_windows:        %u\n", s->num_sr_pti_windows);
	printk("  wifi_ccconf_wifi_window:   %u\n", s->wifi_ccconf_wifi_window);
	printk("  wifi_ccconf_sr_window:     %u\n", s->wifi_ccconf_sr_window);
	printk("  ppw_invalid_params_cnt:    %u\n", s->ppw_invalid_params_cnt);
	printk("  ppw_invalid_request_cnt:   %u\n", s->ppw_invalid_request_cnt);

	printk("\n==================== Band Select (ROM 1.0) ====================\n");
	printk("  cfg_band_sel_5g_cnt:       %u\n", s->cfg_band_sel_5g_cnt);
	printk("  cfg_band_sel_2pt4g_cnt:    %u\n", s->cfg_band_sel_2pt4g_cnt);

	printk("\n==================== Wi-Fi SW Client Aggregate (ROM 1.0) ====================\n");
	printk("  wifi_sw_client_req_cnt:                        %u\n", s->wifi_sw_client_req_cnt);
	printk("  wifi_sw_client_rel_cnt:                        %u\n", s->wifi_sw_client_rel_cnt);
	printk("  wifi_sw_client_gnt_cnt:                        %u\n", s->wifi_sw_client_gnt_cnt);
	printk("  wifi_sw_client_rel_timeout_cnt:                %u\n", s->wifi_sw_client_rel_timeout_cnt);
	printk("  wifi_sw_client_req_non2pt4g_band_cnt:          %u\n",
	       s->wifi_sw_client_req_non2pt4g_band_cnt);
	printk("  wifi_sw_client_req_2pt4g_band_cnt:             %u\n",
	       s->wifi_sw_client_req_2pt4g_band_cnt);
	printk("  wifi_sw_client_decison_in_favour_cnt_2pt4g:    %u\n",
	       s->wifi_sw_client_decison_in_favour_cnt_2pt4g);
	printk("  wifi_sw_client_decison_not_in_favour_cnt_2pt4g:%u\n",
	       s->wifi_sw_client_decison_not_in_favour_cnt_2pt4g);
	printk("  wifi_sw_client_no_gnt_cnt_2pt4g:               %u\n",
	       s->wifi_sw_client_no_gnt_cnt_2pt4g);
	printk("  wifi_sw_client_no_gnt_cnt_non_2pt4g:           %u\n",
	       s->wifi_sw_client_no_gnt_cnt_non_2pt4g);
	printk("  wifi_sw_client_rel_cnt_2pt4g:                  %u\n", s->wifi_sw_client_rel_cnt_2pt4g);
	printk("  wifi_sw_client_rel_cnt_non_2pt4g:              %u\n",
	       s->wifi_sw_client_rel_cnt_non_2pt4g);
	printk("  wifi_sw_client_gnt_cnt_2pt4g:                  %u\n", s->wifi_sw_client_gnt_cnt_2pt4g);
	printk("  wifi_sw_client_gnt_cnt_non_2pt4g:              %u\n",
	       s->wifi_sw_client_gnt_cnt_non_2pt4g);

	printk("\n==================== Wi-Fi Per-Client Req/Rel/Gnt (ROM 1.0) ====================\n");
	printk("  wifi_beacon_rx_req_cnt:    %u\n", s->wifi_beacon_rx_req_cnt);
	printk("  wifi_conn_req_cnt:         %u\n", s->wifi_conn_req_cnt);
	printk("  wifi_calib_req_cnt:        %u\n", s->wifi_calib_req_cnt);
	printk("  wifi_scan_req_cnt:         %u\n", s->wifi_scan_req_cnt);
	printk("  wifi_beacon_rx_rel_cnt:    %u\n", s->wifi_beacon_rx_rel_cnt);
	printk("  wifi_conn_rel_cnt:         %u\n", s->wifi_conn_rel_cnt);
	printk("  wifi_calib_rel_cnt:        %u\n", s->wifi_calib_rel_cnt);
	printk("  wifi_scan_rel_cnt:         %u\n", s->wifi_scan_rel_cnt);
	printk("  wifi_beacon_rx_gnt_cnt:    %u\n", s->wifi_beacon_rx_gnt_cnt);
	printk("  wifi_conn_gnt_cnt:         %u\n", s->wifi_conn_gnt_cnt);
	printk("  wifi_calib_gnt_cnt:        %u\n", s->wifi_calib_gnt_cnt);
	printk("  wifi_scan_gnt_cnt:         %u\n", s->wifi_scan_gnt_cnt);
	printk("  wifi_beacon_rx_no_gnt_cnt: %u\n", s->wifi_beacon_rx_no_gnt_cnt);
	printk("  wifi_conn_no_gnt_cnt:      %u\n", s->wifi_conn_no_gnt_cnt);
	printk("  wifi_calib_no_gnt_cnt:     %u\n", s->wifi_calib_no_gnt_cnt);
	printk("  wifi_scan_no_gnt_cnt:      %u\n", s->wifi_scan_no_gnt_cnt);
	printk("  wifi_beacon_rx_rel_to_cnt: %u\n", s->wifi_beacon_rx_rel_to_cnt);
	printk("  wifi_conn_rel_to_cnt:      %u\n", s->wifi_conn_rel_to_cnt);
	printk("  wifi_calib_rel_to_cnt:     %u\n", s->wifi_calib_rel_to_cnt);
	printk("  wifi_scan_rel_to_cnt:      %u\n", s->wifi_scan_rel_to_cnt);

	printk("\n==================== SR Rx Protection (ROM 1.0) ====================\n");
	printk("  sr_rx_prot_success_cnt_2pt4g:                  %u\n",
	       s->sr_rx_prot_success_cnt_2pt4g);
	printk("  sr_rx_prot_fail_cnt_2pt4g:                     %u\n",
	       s->sr_rx_prot_fail_cnt_2pt4g);
	printk("  sr_rx_prot_success_cnt_non_2pt4g:              %u\n",
	       s->sr_rx_prot_success_cnt_non_2pt4g);
	printk("  sr_rx_prot_fail_cnt_non_2pt4g:                 %u\n",
	       s->sr_rx_prot_fail_cnt_non_2pt4g);
	printk("  sr_rx_prot_inact2listen_ps_cnt_2pt4g:          %u\n",
	       s->sr_rx_prot_inact2listen_ps_cnt_2pt4g);
	printk("  sr_rx_prot_listen2inact_ps_cnt_2pt4g:          %u\n",
	       s->sr_rx_prot_listen2inact_ps_cnt_2pt4g);
	printk("  sr_rx_prot_inact2listen_calib_cnt_2pt4g:       %u\n",
	       s->sr_rx_prot_inact2listen_calib_cnt_2pt4g);
	printk("  sr_rx_prot_listen2inact_calib_cnt_2pt4g:       %u\n",
	       s->sr_rx_prot_listen2inact_calib_cnt_2pt4g);
	printk("  sr_rx_prot_inact2listen_ps_cnt_non_2pt4g:      %u\n",
	       s->sr_rx_prot_inact2listen_ps_cnt_non_2pt4g);
	printk("  sr_rx_prot_listen2inact_ps_cnt_non_2pt4g:      %u\n",
	       s->sr_rx_prot_listen2inact_ps_cnt_non_2pt4g);
	printk("  sr_rx_prot_inact2listen_calib_cnt_non_2pt4g:   %u\n",
	       s->sr_rx_prot_inact2listen_calib_cnt_non_2pt4g);
	printk("  sr_rx_prot_listen2inact_calib_cnt_non_2pt4g:   %u\n",
	       s->sr_rx_prot_listen2inact_calib_cnt_non_2pt4g);
	printk("  sr_rx_prot_2pt4g_band_cnt:                     %u\n", s->sr_rx_prot_2pt4g_band_cnt);
	printk("  sr_rx_prot_non2pt4g_band_cnt:                  %u\n",
	       s->sr_rx_prot_non2pt4g_band_cnt);
	printk("  sr_rx_prot_force_wifi_cnt_2pt4g:               %u\n",
	       s->sr_rx_prot_force_wifi_cnt_2pt4g);

	printk("\n==================== Coex Enable/Disable (ROM 1.0) ====================\n");
	printk("  coex_enable_cnt:           %u\n", s->coex_enable_cnt);
	printk("  coex_disable_cnt:          %u\n", s->coex_disable_cnt);

	printk("\n==================== ANTSWC Configuration (ROM 1.0) ====================\n");
	printk("  antswc_inhibit_cfg_cnt:          %u\n", s->antswc_inhibit_cfg_cnt);
	printk("  antswc_anten_override_cfg_cnt:   %u\n", s->antswc_anten_override_cfg_cnt);
	printk("  antswc_pa_override_cfg_cnt:      %u\n", s->antswc_pa_override_cfg_cnt);
	printk("  antswc_lnasw_override_cfg_cnt:   %u\n", s->antswc_lnasw_override_cfg_cnt);

	printk("\n==================== CCCONF (ROM 1.0) ====================\n");
	printk("  wifi_ccconf_save_cnt:      %u\n", s->wifi_ccconf_save_cnt);
	printk("  wifi_ccconf_restore_cnt:   %u\n", s->wifi_ccconf_restore_cnt);

	printk("\n==================== Command Counts (ROM 1.0) ====================\n");
	printk("  cmd_update_coex_params_cnt:   %u\n", s->cmd_update_coex_params_cnt);
	printk("  cmd_update_user_params_cnt:   %u\n", s->cmd_update_user_params_cnt);
	printk("  cmd_enable_coex_cnt:          %u\n", s->cmd_enable_coex_cnt);
	printk("  cmd_allocate_ppw_cnt:         %u\n", s->cmd_allocate_ppw_cnt);
	printk("  cmd_set_pti_ranges_cnt:       %u\n", s->cmd_set_pti_ranges_cnt);
	printk("  cmd_get_stats_cnt:            %u\n", s->cmd_get_stats_cnt);
	printk("  cmd_sw_client_req_cnt:        %u\n", s->cmd_sw_client_req_cnt);

	printk("\n==================== Events (ROM 1.0 / patch) ====================\n");
	printk("  stats_event_to_host_cnt:           %u\n", s->stats_event_to_host_cnt);
	if (patch_stats != NULL) {
		printk("  wifi_sw_client_event_to_host_cnt:  %u\n",
		       patch_stats->wifi_sw_client_event_to_host_cnt);
		printk("  coex_params_event_to_host_cnt:     %u\n",
		       patch_stats->coex_params_event_to_host_cnt);
		printk("  user_params_event_to_host_cnt:     %u\n",
		       patch_stats->user_params_event_to_host_cnt);
		printk("  enable_coex_event_to_host_cnt:     %u\n",
		       patch_stats->enable_coex_event_to_host_cnt);
		printk("  allocate_ppw_event_to_host_cnt:    %u\n",
		       patch_stats->allocate_ppw_event_to_host_cnt);
		printk("  set_pti_ranges_event_to_host_cnt:  %u\n",
		       patch_stats->set_pti_ranges_event_to_host_cnt);
	} else {
		printk("  sw_client_event_to_host_cnt:       %u\n", s->sw_client_event_to_host_cnt);
		printk("  (patch per-event counters require cm_fsm_patch_stats_t payload)\n");
	}

	printk("\n==================== Error Counts (ROM 1.0) ====================\n");
	printk("  cd2cm_null_cmd_buf_cnt:                  %u\n", s->cd2cm_null_cmd_buf_cnt);
	printk("  cd2cm_invalid_msg_id_cnt:                %u\n", s->cd2cm_invalid_msg_id_cnt);
	printk("  coex_params_buf_error:                   %u\n", s->coex_params_buf_error);
	printk("  user_params_buf_error:                   %u\n", s->user_params_buf_error);
	printk("  en_coex_buf_error:                       %u\n", s->en_coex_buf_error);
	printk("  ppw_buf_error:                           %u\n", s->ppw_buf_error);
	printk("  set_pti_ranges_buf_error:                %u\n", s->set_pti_ranges_buf_error);
	printk("  sw_client_req_buf_error:                 %u\n", s->sw_client_req_buf_error);
	printk("  event_buf_unavailable_cnt:               %u\n", s->event_buf_unavailable_cnt);
	printk("  unknown_event_to_host:                   %u\n", s->unknown_event_to_host);
	printk("  wifi_wrong_sw_client_id_cnt:             %u\n", s->wifi_wrong_sw_client_id_cnt);
	printk("  wifi_wrong_sw_client_request_type_cnt:   %u\n",
	       s->wifi_wrong_sw_client_request_type_cnt);
	printk("  wrong_antswc_ctrl_type_cnt:              %u\n", s->wrong_antswc_ctrl_type_cnt);
	printk("  wrong_wifi_hw_client_pti_level_cnt:      %u\n",
	       s->wrong_wifi_hw_client_pti_level_cnt);
	printk("  wrong_input_populate_pti_cnt:            %u\n", s->wrong_input_populate_pti_cnt);
	printk("  wrong_sr_rx_prot_scenario_cnt:           %u\n", s->wrong_sr_rx_prot_scenario_cnt);
	printk("  wrong_sr_rx_prot_perc_prob_cnt:          %u\n", s->wrong_sr_rx_prot_perc_prob_cnt);
	printk("  wrong_wifi_sw_client_id_cnt:             %u\n", s->wrong_wifi_sw_client_id_cnt);
	printk("  wrong_wifi_sw_client_perc_prob_cnt:      %u\n", s->wrong_wifi_sw_client_perc_prob_cnt);
	printk("  zero_ppw_win_duration_cnt:               %u\n", s->zero_ppw_win_duration_cnt);
	printk("  zero_ppw_timeout_cnt:                    %u\n", s->zero_ppw_timeout_cnt);
}

static void coex_tb_validate_stats(const struct cm_stats_t *s)
{
	unsigned int check_num = 0U;
	bool pass;

	printk("\n==================== Validation Checks (ROM 1.0) ====================\n");

	check_num++;
	pass = (s->wifi_sw_client_req_cnt ==
		s->wifi_sw_client_rel_cnt + s->wifi_sw_client_rel_timeout_cnt);
	printk("  [CHECK %02u] Wi-Fi SW req == rel + rel_timeout: %s\n", check_num,
	       pass ? "PASS" : "FAIL");

	check_num++;
	pass = (s->wifi_sw_client_req_cnt ==
		s->wifi_sw_client_req_non2pt4g_band_cnt + s->wifi_sw_client_req_2pt4g_band_cnt);
	printk("  [CHECK %02u] Wi-Fi SW req == non2pt4g_band + 2pt4g_band: %s\n", check_num,
	       pass ? "PASS" : "FAIL");

	check_num++;
	pass = (s->wifi_sw_client_req_cnt ==
		s->wifi_sw_client_gnt_cnt + s->wifi_sw_client_decison_not_in_favour_cnt_2pt4g);
	printk("  [CHECK %02u] Wi-Fi SW req == gnt + not_in_favour_2pt4g: %s\n", check_num,
	       pass ? "PASS" : "FAIL");

	check_num++;
	pass = (s->wifi_sw_client_gnt_cnt ==
		s->wifi_sw_client_req_non2pt4g_band_cnt +
			s->wifi_sw_client_decison_in_favour_cnt_2pt4g);
	printk("  [CHECK %02u] Wi-Fi SW gnt == non2pt4g_band + in_favour_2pt4g: %s\n", check_num,
	       pass ? "PASS" : "FAIL");

	check_num++;
	{
		unsigned int sum_req = s->wifi_beacon_rx_req_cnt + s->wifi_conn_req_cnt +
				       s->wifi_calib_req_cnt + s->wifi_scan_req_cnt;

		pass = (s->wifi_sw_client_req_cnt == sum_req);
		printk("  [CHECK %02u] Wi-Fi SW req == sum(per-client req): %s\n", check_num,
		       pass ? "PASS" : "FAIL");
	}

	check_num++;
	{
		unsigned int sum_rel = s->wifi_beacon_rx_rel_cnt + s->wifi_conn_rel_cnt +
				       s->wifi_calib_rel_cnt + s->wifi_scan_rel_cnt;

		pass = (s->wifi_sw_client_rel_cnt == sum_rel);
		printk("  [CHECK %02u] Wi-Fi SW rel == sum(per-client rel): %s\n", check_num,
		       pass ? "PASS" : "FAIL");
	}

	check_num++;
	{
		unsigned int sum_gnt = s->wifi_beacon_rx_gnt_cnt + s->wifi_conn_gnt_cnt +
				       s->wifi_calib_gnt_cnt + s->wifi_scan_gnt_cnt;

		pass = (s->wifi_sw_client_gnt_cnt == sum_gnt);
		printk("  [CHECK %02u] Wi-Fi SW gnt == sum(per-client gnt): %s\n", check_num,
		       pass ? "PASS" : "FAIL");
	}

	check_num++;
	{
		unsigned int sum_to = s->wifi_beacon_rx_rel_to_cnt + s->wifi_conn_rel_to_cnt +
				      s->wifi_calib_rel_to_cnt + s->wifi_scan_rel_to_cnt;

		pass = (s->wifi_sw_client_rel_timeout_cnt == sum_to);
		printk("  [CHECK %02u] Wi-Fi SW rel_timeout == sum(per-client rel_to): %s\n",
		       check_num, pass ? "PASS" : "FAIL");
	}

	check_num++;
	pass = (s->wifi_beacon_rx_req_cnt ==
		s->wifi_beacon_rx_gnt_cnt + s->wifi_beacon_rx_no_gnt_cnt);
	printk("  [CHECK %02u] Wi-Fi beacon_rx: req == gnt + no_gnt: %s\n", check_num,
	       pass ? "PASS" : "FAIL");

	check_num++;
	pass = (s->wifi_beacon_rx_req_cnt ==
		s->wifi_beacon_rx_rel_cnt + s->wifi_beacon_rx_rel_to_cnt);
	printk("  [CHECK %02u] Wi-Fi beacon_rx: req == rel + rel_to: %s\n", check_num,
	       pass ? "PASS" : "FAIL");

	check_num++;
	pass = (s->wifi_conn_req_cnt == s->wifi_conn_gnt_cnt + s->wifi_conn_no_gnt_cnt);
	printk("  [CHECK %02u] Wi-Fi conn: req == gnt + no_gnt: %s\n", check_num,
	       pass ? "PASS" : "FAIL");

	check_num++;
	pass = (s->wifi_conn_req_cnt == s->wifi_conn_rel_cnt + s->wifi_conn_rel_to_cnt);
	printk("  [CHECK %02u] Wi-Fi conn: req == rel + rel_to: %s\n", check_num,
	       pass ? "PASS" : "FAIL");

	check_num++;
	pass = (s->wifi_calib_req_cnt == s->wifi_calib_gnt_cnt + s->wifi_calib_no_gnt_cnt);
	printk("  [CHECK %02u] Wi-Fi calib: req == gnt + no_gnt: %s\n", check_num,
	       pass ? "PASS" : "FAIL");

	check_num++;
	pass = (s->wifi_calib_req_cnt == s->wifi_calib_rel_cnt + s->wifi_calib_rel_to_cnt);
	printk("  [CHECK %02u] Wi-Fi calib: req == rel + rel_to: %s\n", check_num,
	       pass ? "PASS" : "FAIL");

	check_num++;
	pass = (s->wifi_scan_req_cnt == s->wifi_scan_gnt_cnt + s->wifi_scan_no_gnt_cnt);
	printk("  [CHECK %02u] Wi-Fi scan: req == gnt + no_gnt: %s\n", check_num,
	       pass ? "PASS" : "FAIL");

	check_num++;
	pass = (s->wifi_scan_req_cnt == s->wifi_scan_rel_cnt + s->wifi_scan_rel_to_cnt);
	printk("  [CHECK %02u] Wi-Fi scan: req == rel + rel_to: %s\n", check_num,
	       pass ? "PASS" : "FAIL");

	check_num++;
	pass = (s->sr_rx_prot_2pt4g_band_cnt ==
		s->sr_rx_prot_success_cnt_2pt4g + s->sr_rx_prot_fail_cnt_2pt4g);
	printk("  [CHECK %02u] SR Rx prot 2.4G: band == success + fail: %s\n", check_num,
	       pass ? "PASS" : "FAIL");

	check_num++;
	pass = (s->sr_rx_prot_non2pt4g_band_cnt ==
		s->sr_rx_prot_success_cnt_non_2pt4g + s->sr_rx_prot_fail_cnt_non_2pt4g);
	printk("  [CHECK %02u] SR Rx prot non-2.4G: band == success + fail: %s\n", check_num,
	       pass ? "PASS" : "FAIL");

	check_num++;
	{
		unsigned int sum_2g = s->sr_rx_prot_inact2listen_ps_cnt_2pt4g +
				      s->sr_rx_prot_listen2inact_ps_cnt_2pt4g +
				      s->sr_rx_prot_inact2listen_calib_cnt_2pt4g +
				      s->sr_rx_prot_listen2inact_calib_cnt_2pt4g;

		pass = (s->sr_rx_prot_2pt4g_band_cnt == sum_2g);
		printk("  [CHECK %02u] SR Rx prot 2.4G: band == sum(scenario counts): %s\n",
		       check_num, pass ? "PASS" : "FAIL");
	}

	check_num++;
	{
		unsigned int sum_non2g = s->sr_rx_prot_inact2listen_ps_cnt_non_2pt4g +
				       s->sr_rx_prot_listen2inact_ps_cnt_non_2pt4g +
				       s->sr_rx_prot_inact2listen_calib_cnt_non_2pt4g +
				       s->sr_rx_prot_listen2inact_calib_cnt_non_2pt4g;

		pass = (s->sr_rx_prot_non2pt4g_band_cnt == sum_non2g);
		printk("  [CHECK %02u] SR Rx prot non-2.4G: band == sum(scenario counts): %s\n",
		       check_num, pass ? "PASS" : "FAIL");
	}

	check_num++;
	pass = (s->antswc_inhibit_cfg_cnt == s->cmd_update_coex_params_cnt);
	printk("  [CHECK %02u] ANTSWC inhibit_cfg == cmd_update_coex_params: %s\n", check_num,
	       pass ? "PASS" : "FAIL");

	check_num++;
	pass = (s->antswc_anten_override_cfg_cnt == s->cmd_update_user_params_cnt);
	printk("  [CHECK %02u] ANTSWC anten_override_cfg == cmd_update_user_params: %s\n",
	       check_num, pass ? "PASS" : "FAIL");

	check_num++;
	pass = (s->antswc_pa_override_cfg_cnt == s->cmd_update_coex_params_cnt);
	printk("  [CHECK %02u] ANTSWC pa_override_cfg == cmd_update_coex_params: %s\n", check_num,
	       pass ? "PASS" : "FAIL");

	check_num++;
	pass = (s->antswc_lnasw_override_cfg_cnt == s->cmd_update_coex_params_cnt);
	printk("  [CHECK %02u] ANTSWC lnasw_override_cfg == cmd_update_coex_params: %s\n",
	       check_num, pass ? "PASS" : "FAIL");

	check_num++;
	pass = (s->wifi_ccconf_save_cnt == s->wifi_ccconf_restore_cnt);
	printk("  [CHECK %02u] Wi-Fi CCCONF save == restore: %s\n", check_num,
	       pass ? "PASS" : "FAIL");

	check_num++;
	pass = (s->cmd_allocate_ppw_cnt == s->ppw_start_msgs_cnt + s->ppw_stop_msgs_cnt);
	printk("  [CHECK %02u] cmd_allocate_ppw == ppw_start + ppw_stop: %s\n", check_num,
	       pass ? "PASS" : "FAIL");

	printk("\n==================== End of Validation (%u checks) ====================\n\n",
	       check_num);
}

void coex_tb_print_and_validate_stats(const struct cm_stats_t *stats,
				      const struct cm_fsm_patch_stats_t *patch_stats)
{
	if (stats == NULL) {
		printk("Coex TB stats: no statistics payload\n");
		return;
	}

	coex_tb_print_stats(stats, patch_stats);
	coex_tb_validate_stats(stats);

	if (patch_stats != NULL) {
		coex_tb_print_patch_stats(patch_stats);
	} else {
		printk("\nPatch command counters not available in this CM2CD event payload.\n");
	}
}
