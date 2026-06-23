//
// Created by gabri on 6/21/2026.
//

#ifndef D_BOX_TESTFAKESERAL_H
#define D_BOX_TESTFAKESERAL_H
#include <cassert>
#include <array>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <span>
#include <string>
#include <thread>
#include <future>
#include <vector>
int TestFakeSeral();
/*
static void assertVectorEq(const std::vector<uint8_t>& actual, std::initializer_list<uint8_t> expected);
static void assertStringVectorEq(const std::vector<std::string>& actual,std::initializer_list<std::string> expected);
static void test_starts_closed();
static void test_open_succeeds_and_marks_open();
static void test_close_marks_closed();
static void test_close_is_safe_when_already_closed();
static void test_can_reopen_after_close();
static void test_rawwrite_returns_zero_when_closed();
static void test_rawwrite_returns_zero_for_zero_requested_size();
static void test_rawwrite_returns_zero_for_empty_span();
static void test_rawwrite_writes_all_bytes_when_size_matches();
static void test_rawwrite_only_writes_requested_prefix();
static void test_rawwrite_clamps_requested_size_to_span_size();
static void test_multiple_rawwrites_append_in_order();
static void test_writtenData_returns_copy_not_alias();
static void test_takeWritten_returns_all_written_bytes_and_clears_tx();
static void test_clearWritten_empties_tx();
static void test_waitForWrite_returns_true_when_enough_bytes_already_exist();
static void test_waitForWrite_returns_false_on_timeout();
static void test_waitForWrite_unblocks_when_another_thread_writes();
static void test_waitForWrite_returns_true_if_port_closes_before_threshold();
static void test_rawread_returns_zero_when_closed();

static void test_rawread_returns_zero_for_zero_requested_size();
static void test_rawread_returns_zero_for_empty_span();
static void test_pushIncoming_makes_bytes_available_to_rawread();
static void test_rawread_reads_only_requested_prefix_when_less_than_available();
static void test_rawread_clamps_requested_size_to_buffer_size();
static void test_multiple_rawreads_consume_in_order();
static void test_pushIncoming_vector_overload();
static void test_pushIncoming_cstring_overload();
static void test_pendingIncoming_tracks_remaining_rx_bytes();
static void test_clearIncoming_clears_rx_queue();

static void test_rawread_blocks_until_data_arrives();
static void test_close_wakes_blocked_rawread_and_returns_zero();
static void test_close_clears_rx_and_tx_buffers();
static void test_find_returns_default_fake_port();
static void test_setAvailablePorts_changes_find_results();

static void test_find_returns_copy_not_alias();
static void test_flushBuss_is_safe_when_closed();
static void test_flushBuss_does_not_modify_buffers();
*/
#endif //D_BOX_TESTFAKESERAL_H