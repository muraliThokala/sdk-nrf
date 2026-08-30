/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief CM statistics printing and consistency checking.
 */

#include <stdbool.h>

#include <zephyr/sys/printk.h>

#include "coex_tb_stats.h"

/**
 * Print one consistency check and report whether it failed.
 *
 * @return 1 if the check failed, 0 if it passed, so callers can sum the result.
 */
static unsigned int tb_check(unsigned int *check_num, bool pass, const char *what)
{
	(*check_num)++;
	printk("  [CHECK %02u] %-56s %s\n", *check_num, what, pass ? "PASS" : "FAIL");

	return pass ? 0U : 1U;
}

/** Per-event counters that only patched CM firmware reports. */
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
	printk("\n==================== PPW Statistics ====================\n");
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

	printk("\n==================== Band Select ====================\n");
	printk("  cfg_band_sel_5g_cnt:       %u\n", s->cfg_band_sel_5g_cnt);
	printk("  cfg_band_sel_2pt4g_cnt:    %u\n", s->cfg_band_sel_2pt4g_cnt);

	printk("\n==================== Wi-Fi SW Client Aggregate ====================\n");
	printk("  wifi_sw_client_req_cnt:                        %u\n", s->wifi_sw_client_req_cnt);
	printk("  wifi_sw_client_rel_cnt:                        %u\n", s->wifi_sw_client_rel_cnt);
	printk("  wifi_sw_client_gnt_cnt:                        %u\n", s->wifi_sw_client_gnt_cnt);
	printk("  wifi_sw_client_rel_timeout_cnt:                %u\n",
	       s->wifi_sw_client_rel_timeout_cnt);
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
	printk("  wifi_sw_client_rel_cnt_2pt4g:                  %u\n",
	       s->wifi_sw_client_rel_cnt_2pt4g);
	printk("  wifi_sw_client_rel_cnt_non_2pt4g:              %u\n",
	       s->wifi_sw_client_rel_cnt_non_2pt4g);
	printk("  wifi_sw_client_gnt_cnt_2pt4g:                  %u\n",
	       s->wifi_sw_client_gnt_cnt_2pt4g);
	printk("  wifi_sw_client_gnt_cnt_non_2pt4g:              %u\n",
	       s->wifi_sw_client_gnt_cnt_non_2pt4g);

	printk("\n==================== Wi-Fi Per-Client Req/Rel/Gnt ====================\n");
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

	printk("\n==================== SR Rx Protection ====================\n");
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
	printk("  sr_rx_prot_2pt4g_band_cnt:                     %u\n",
	       s->sr_rx_prot_2pt4g_band_cnt);
	printk("  sr_rx_prot_non2pt4g_band_cnt:                  %u\n",
	       s->sr_rx_prot_non2pt4g_band_cnt);
	printk("  sr_rx_prot_force_wifi_cnt_2pt4g:               %u\n",
	       s->sr_rx_prot_force_wifi_cnt_2pt4g);

	printk("\n==================== Coex Enable/Disable ====================\n");
	printk("  coex_enable_cnt:           %u\n", s->coex_enable_cnt);
	printk("  coex_disable_cnt:          %u\n", s->coex_disable_cnt);

	printk("\n==================== ANTSWC Configuration ====================\n");
	printk("  antswc_inhibit_cfg_cnt:          %u\n", s->antswc_inhibit_cfg_cnt);
	printk("  antswc_anten_override_cfg_cnt:   %u\n", s->antswc_anten_override_cfg_cnt);
	printk("  antswc_pa_override_cfg_cnt:      %u\n", s->antswc_pa_override_cfg_cnt);
	printk("  antswc_lnasw_override_cfg_cnt:   %u\n", s->antswc_lnasw_override_cfg_cnt);

	printk("\n==================== CCCONF ====================\n");
	printk("  wifi_ccconf_save_cnt:      %u\n", s->wifi_ccconf_save_cnt);
	printk("  wifi_ccconf_restore_cnt:   %u\n", s->wifi_ccconf_restore_cnt);

	printk("\n==================== Command Counts ====================\n");
	printk("  cmd_update_coex_params_cnt:   %u\n", s->cmd_update_coex_params_cnt);
	printk("  cmd_update_user_params_cnt:   %u\n", s->cmd_update_user_params_cnt);
	printk("  cmd_enable_coex_cnt:          %u\n", s->cmd_enable_coex_cnt);
	printk("  cmd_allocate_ppw_cnt:         %u\n", s->cmd_allocate_ppw_cnt);
	printk("  cmd_set_pti_ranges_cnt:       %u\n", s->cmd_set_pti_ranges_cnt);
	printk("  cmd_get_stats_cnt:            %u\n", s->cmd_get_stats_cnt);
	printk("  cmd_sw_client_req_cnt:        %u\n", s->cmd_sw_client_req_cnt);

	printk("\n==================== Events ====================\n");
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
		printk("  sw_client_event_to_host_cnt:       %u\n",
		       s->sw_client_event_to_host_cnt);
		printk("  (patch per-event counters require cm_fsm_patch_stats_t payload)\n");
	}

	/*
	 * These should all be zero. A non-zero value means the CM rejected
	 * something the test bench (or the driver) sent it.
	 */
	printk("\n==================== Error Counts ==============\n");
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
	printk("  wrong_sr_rx_prot_scenario_cnt:           %u\n",
	       s->wrong_sr_rx_prot_scenario_cnt);
	printk("  wrong_sr_rx_prot_perc_prob_cnt:          %u\n",
	       s->wrong_sr_rx_prot_perc_prob_cnt);
	printk("  wrong_wifi_sw_client_id_cnt:             %u\n", s->wrong_wifi_sw_client_id_cnt);
	printk("  wrong_wifi_sw_client_perc_prob_cnt:      %u\n",
	       s->wrong_wifi_sw_client_perc_prob_cnt);
	printk("  zero_ppw_win_duration_cnt:               %u\n", s->zero_ppw_win_duration_cnt);
	printk("  zero_ppw_timeout_cnt:                    %u\n", s->zero_ppw_timeout_cnt);
}

/**
 * Cross-check the CM counters against each other.
 *
 * Every check here is an identity that must hold regardless of what the test
 * bench did, so a failure points at the CM's bookkeeping rather than at the
 * test sequence.
 *
 * @return Number of failed checks.
 */
static unsigned int coex_tb_validate_stats(const struct cm_stats_t *s)
{
	unsigned int check_num = 0U;
	unsigned int failures = 0U;

	printk("\n==================== Validation Checks ====================\n");

	/* Every request eventually ends either in an explicit release or a timeout. */
	failures += tb_check(&check_num,
			     s->wifi_sw_client_req_cnt ==
				     s->wifi_sw_client_rel_cnt +
					     s->wifi_sw_client_rel_timeout_cnt,
			     "Wi-Fi SW req == rel + rel_timeout");

	/* Every request was counted in exactly one band bucket. */
	failures += tb_check(&check_num,
			     s->wifi_sw_client_req_cnt ==
				     s->wifi_sw_client_req_non2pt4g_band_cnt +
					     s->wifi_sw_client_req_2pt4g_band_cnt,
			     "Wi-Fi SW req == non2pt4g_band + 2pt4g_band");

	/* Every request was either granted or lost the 2.4 GHz arbitration. */
	failures += tb_check(&check_num,
			     s->wifi_sw_client_req_cnt ==
				     s->wifi_sw_client_gnt_cnt +
					     s->wifi_sw_client_decison_not_in_favour_cnt_2pt4g,
			     "Wi-Fi SW req == gnt + not_in_favour_2pt4g");

	/* Grants come from non-2.4 GHz (no contention) plus won 2.4 GHz decisions. */
	failures += tb_check(&check_num,
			     s->wifi_sw_client_gnt_cnt ==
				     s->wifi_sw_client_req_non2pt4g_band_cnt +
					     s->wifi_sw_client_decison_in_favour_cnt_2pt4g,
			     "Wi-Fi SW gnt == non2pt4g_band + in_favour_2pt4g");

	/* Aggregate counters must equal the sum over the four Wi-Fi client types. */
	failures += tb_check(&check_num,
			     s->wifi_sw_client_req_cnt ==
				     s->wifi_beacon_rx_req_cnt + s->wifi_conn_req_cnt +
					     s->wifi_calib_req_cnt + s->wifi_scan_req_cnt,
			     "Wi-Fi SW req == sum(per-client req)");

	failures += tb_check(&check_num,
			     s->wifi_sw_client_rel_cnt ==
				     s->wifi_beacon_rx_rel_cnt + s->wifi_conn_rel_cnt +
					     s->wifi_calib_rel_cnt + s->wifi_scan_rel_cnt,
			     "Wi-Fi SW rel == sum(per-client rel)");

	failures += tb_check(&check_num,
			     s->wifi_sw_client_gnt_cnt ==
				     s->wifi_beacon_rx_gnt_cnt + s->wifi_conn_gnt_cnt +
					     s->wifi_calib_gnt_cnt + s->wifi_scan_gnt_cnt,
			     "Wi-Fi SW gnt == sum(per-client gnt)");

	failures += tb_check(&check_num,
			     s->wifi_sw_client_rel_timeout_cnt ==
				     s->wifi_beacon_rx_rel_to_cnt + s->wifi_conn_rel_to_cnt +
					     s->wifi_calib_rel_to_cnt + s->wifi_scan_rel_to_cnt,
			     "Wi-Fi SW rel_timeout == sum(per-client rel_to)");

	/* The same request/grant and request/release identities, per client type. */
	failures += tb_check(&check_num,
			     s->wifi_beacon_rx_req_cnt ==
				     s->wifi_beacon_rx_gnt_cnt + s->wifi_beacon_rx_no_gnt_cnt,
			     "Wi-Fi beacon_rx: req == gnt + no_gnt");

	failures += tb_check(&check_num,
			     s->wifi_beacon_rx_req_cnt ==
				     s->wifi_beacon_rx_rel_cnt + s->wifi_beacon_rx_rel_to_cnt,
			     "Wi-Fi beacon_rx: req == rel + rel_to");

	failures += tb_check(&check_num,
			     s->wifi_conn_req_cnt ==
				     s->wifi_conn_gnt_cnt + s->wifi_conn_no_gnt_cnt,
			     "Wi-Fi conn: req == gnt + no_gnt");

	failures += tb_check(&check_num,
			     s->wifi_conn_req_cnt ==
				     s->wifi_conn_rel_cnt + s->wifi_conn_rel_to_cnt,
			     "Wi-Fi conn: req == rel + rel_to");

	failures += tb_check(&check_num,
			     s->wifi_calib_req_cnt ==
				     s->wifi_calib_gnt_cnt + s->wifi_calib_no_gnt_cnt,
			     "Wi-Fi calib: req == gnt + no_gnt");

	failures += tb_check(&check_num,
			     s->wifi_calib_req_cnt ==
				     s->wifi_calib_rel_cnt + s->wifi_calib_rel_to_cnt,
			     "Wi-Fi calib: req == rel + rel_to");

	failures += tb_check(&check_num,
			     s->wifi_scan_req_cnt ==
				     s->wifi_scan_gnt_cnt + s->wifi_scan_no_gnt_cnt,
			     "Wi-Fi scan: req == gnt + no_gnt");

	failures += tb_check(&check_num,
			     s->wifi_scan_req_cnt ==
				     s->wifi_scan_rel_cnt + s->wifi_scan_rel_to_cnt,
			     "Wi-Fi scan: req == rel + rel_to");

	/* Every SR Rx protection attempt either succeeded or failed. */
	failures += tb_check(&check_num,
			     s->sr_rx_prot_2pt4g_band_cnt ==
				     s->sr_rx_prot_success_cnt_2pt4g +
					     s->sr_rx_prot_fail_cnt_2pt4g,
			     "SR Rx prot 2.4G: band == success + fail");

	failures += tb_check(&check_num,
			     s->sr_rx_prot_non2pt4g_band_cnt ==
				     s->sr_rx_prot_success_cnt_non_2pt4g +
					     s->sr_rx_prot_fail_cnt_non_2pt4g,
			     "SR Rx prot non-2.4G: band == success + fail");

	/* ... and was attributed to exactly one of the four protection scenarios. */
	failures += tb_check(&check_num,
			     s->sr_rx_prot_2pt4g_band_cnt ==
				     s->sr_rx_prot_inact2listen_ps_cnt_2pt4g +
					     s->sr_rx_prot_listen2inact_ps_cnt_2pt4g +
					     s->sr_rx_prot_inact2listen_calib_cnt_2pt4g +
					     s->sr_rx_prot_listen2inact_calib_cnt_2pt4g,
			     "SR Rx prot 2.4G: band == sum(scenario counts)");

	failures += tb_check(&check_num,
			     s->sr_rx_prot_non2pt4g_band_cnt ==
				     s->sr_rx_prot_inact2listen_ps_cnt_non_2pt4g +
					     s->sr_rx_prot_listen2inact_ps_cnt_non_2pt4g +
					     s->sr_rx_prot_inact2listen_calib_cnt_non_2pt4g +
					     s->sr_rx_prot_listen2inact_calib_cnt_non_2pt4g,
			     "SR Rx prot non-2.4G: band == sum(scenario counts)");

	/* Each coex-params command reprograms the ANTSWC inhibit/PA/LNA overrides. */
	failures += tb_check(&check_num,
			     s->antswc_inhibit_cfg_cnt == s->cmd_update_coex_params_cnt,
			     "ANTSWC inhibit_cfg == cmd_update_coex_params");

	/* Each user-params command reprograms the antenna override. */
	failures += tb_check(&check_num,
			     s->antswc_anten_override_cfg_cnt == s->cmd_update_user_params_cnt,
			     "ANTSWC anten_override_cfg == cmd_update_user_params");

	failures += tb_check(&check_num,
			     s->antswc_pa_override_cfg_cnt == s->cmd_update_coex_params_cnt,
			     "ANTSWC pa_override_cfg == cmd_update_coex_params");

	failures += tb_check(&check_num,
			     s->antswc_lnasw_override_cfg_cnt == s->cmd_update_coex_params_cnt,
			     "ANTSWC lnasw_override_cfg == cmd_update_coex_params");

	/* Wi-Fi CCCONF is saved before an SR window and restored after it. */
	failures += tb_check(&check_num,
			     s->wifi_ccconf_save_cnt == s->wifi_ccconf_restore_cnt,
			     "Wi-Fi CCCONF save == restore");

	/* Every PPW command was either a start or a stop. */
	failures += tb_check(&check_num,
			     s->cmd_allocate_ppw_cnt ==
				     s->ppw_start_msgs_cnt + s->ppw_stop_msgs_cnt,
			     "cmd_allocate_ppw == ppw_start + ppw_stop");

	printk("\n============ End of Validation: %u checks, %u failed ============\n\n",
	       check_num, failures);

	return failures;
}

unsigned int coex_tb_print_and_validate_stats(const struct cm_stats_t *stats,
					      const struct cm_fsm_patch_stats_t *patch_stats)
{
	unsigned int failures;

	if (stats == NULL) {
		/*
		 * The driver only hands out a pointer once a statistics event
		 * with a payload has been received, so NULL here means the
		 * round trip did not deliver what it should have.
		 */
		printk("Coex TB stats: no statistics payload retained\n");
		return 1U;
	}

	coex_tb_print_stats(stats, patch_stats);
	failures = coex_tb_validate_stats(stats);

	if (patch_stats != NULL) {
		coex_tb_print_patch_stats(patch_stats);
	} else {
		/* Unpatched CM firmware: informational, not a failure. */
		printk("\nPatch command counters not available in this CM2CD event payload.\n");
	}

	return failures;
}
