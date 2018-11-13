/*
Copyright (c) 2015, Cable Television Laboratories, Inc.(“CableLabs”)
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of CableLabs nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL CABLELABS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef __H_SCTE35
#define __H_SCTE35

#include <stdint.h>
#include <vqarray.h>
#include <ts.h>
#include <descriptors.h>
#include <psi.h>


#define SCTE35_NULL_CMD                       0x00
#define SCTE35_SPLICE_SCHEDULE_CMD            0x04
#define SCTE35_SPLICE_INSERT_CMD              0x05
#define SCTE35_TIME_SIGNAL_CMD                0x06
#define SCTE35_BANDWIDTH_RESERVATION_CMD      0x07
#define SCTE35_PRIVATE_COMMAND_CMD            0xFF


typedef struct 
{
   uint8_t time_specified_flag;
   uint64_t pts_time;

} scte35_splice_time;

typedef struct 
{
   uint8_t component_tag;
   scte35_splice_time *splice_time;

} scte35_splice_insert_component;

typedef struct 
{
   uint8_t component_tag;
   uint32_t splice_time;

} scte35_splice_event_component;

typedef struct 
{
   uint8_t auto_return;
   uint64_t duration;

} scte35_break_duration;

typedef struct 
{
   uint8_t tag;
   uint8_t length;
   uint32_t identifier;

   uint8_t* private_bytes;

} scte35_splice_descriptor;

typedef struct 
{
   uint8_t table_id; 
   uint8_t section_syntax_indicator; 
   uint8_t private_indicator; 
   uint16_t section_length; 
   uint8_t protocol_version; 
   uint8_t encrypted_packet; 
   uint8_t encryption_algorithm; 
   uint64_t pts_adjustment; 
   uint8_t cw_index;
   uint16_t tier; 
   uint16_t splice_command_length; 
   uint8_t splice_command_type;

   void *splice_command;

   vqarray_t *splice_descriptors;

   uint32_t E_CRC_32;
   uint32_t CRC_32;

} scte35_splice_info_section;

typedef struct 
{
   uint32_t splice_event_id;
   uint8_t splice_event_cancel_indicator;
   uint8_t out_of_network_indicator;
   uint8_t program_splice_flag;
   uint8_t duration_flag;
   uint8_t splice_immediate_flag;
   scte35_splice_time *splice_time;
   vqarray_t* components;
   scte35_break_duration *break_duration;
   uint16_t unique_program_id;
   uint8_t avail_num;
   uint8_t avails_expected;

} scte35_splice_insert;

typedef struct 
{
   uint32_t splice_event_id;
   uint8_t splice_event_cancel_indicator;
   uint8_t out_of_network_indicator;
   uint8_t program_splice_flag;
   uint8_t duration_flag;
   uint32_t utc_splice_time;
   vqarray_t* components;
   scte35_break_duration *break_duration;
   uint16_t unique_program_id;
   uint8_t avail_num;
   uint8_t avails_expected;

} scte35_splice_event;

typedef struct 
{
   vqarray_t* splice_events;

} scte35_splice_schedule;

typedef struct 
{
   scte35_splice_time *splice_time;

} scte35_time_signal;

typedef struct 
{
   uint32_t identifier;
   uint32_t private_bytes_sz;
   uint8_t* private_bytes;

} scte35_private_command;


#define SCTE35_SPLICE_TABLE_ID 0xFC  // GORP??

scte35_splice_info_section* scte35_splice_info_section_new(); 
void scte35_splice_info_section_free(scte35_splice_info_section *sis); 
int scte35_splice_info_section_read(scte35_splice_info_section *sis, uint8_t *buf, size_t buf_len, 
    uint32_t payload_unit_start_indicator, psi_table_buffer_t *scte35TableBuffer); 
scte35_splice_info_section* scte35_splice_info_section_copy(scte35_splice_info_section *sis);
void scte35_splice_info_section_print_stdout(const scte35_splice_info_section *sis); 

uint64_t get_splice_insert_PTS (scte35_splice_info_section *sis);
uint32_t get_splice_insert_eventID (scte35_splice_info_section *sis);
scte35_splice_insert* get_splice_insert (scte35_splice_info_section *sis);
int is_splice_insert (scte35_splice_info_section *sis);

scte35_time_signal* get_time_signal (scte35_splice_info_section *sis);
int is_time_signal (scte35_splice_info_section *sis);

void scte35_parse_splice_null(bs_t *b);
scte35_splice_schedule* scte35_parse_splice_schedule(bs_t *b);
scte35_splice_insert* scte35_parse_splice_insert(bs_t *b);
scte35_time_signal* scte35_parse_time_signal(bs_t *b);
void scte35_parse_bandwidth_reservation(bs_t *b);
scte35_private_command* scte35_parse_private_command(bs_t *b, uint16_t command_sz);
scte35_splice_event* scte35_parse_splice_event(bs_t *b);

uint64_t scte35_get_latest_PTS (scte35_splice_info_section *sis);

void scte35_free_splice_null();
void scte35_free_splice_schedule(scte35_splice_schedule *splice_schedule);
void scte35_free_splice_insert(scte35_splice_insert* splice_insert);
void scte35_free_time_signal(scte35_time_signal* time_signal);
void scte35_free_bandwidth_reservation();
void scte35_free_private_command(scte35_private_command *private_command);
void scte35_free_splice_event(scte35_splice_event* spice_event);

scte35_splice_schedule * scte35_copy_splice_schedule(scte35_splice_schedule *splice_schedule);
scte35_splice_insert* scte35_copy_splice_insert(scte35_splice_insert* splice_insert);
scte35_time_signal* scte35_copy_time_signal(scte35_time_signal* time_signal);
scte35_private_command * scte35_copy_private_command(scte35_private_command *private_command);

void scte35_splice_null_print_stdout ();
void scte35_splice_schedule_print_stdout (const scte35_splice_schedule *splice_schedule);
void scte35_splice_event_print_stdout (const scte35_splice_event *splice_event, int splice_event_num);
void scte35_splice_insert_print_stdout (const scte35_splice_insert *splice_insert);
void scte35_time_signal_print_stdout (scte35_time_signal* time_signal);
void scte35_bandwidth_reservation_print_stdout ();
void scte35_private_command_print_stdout (scte35_private_command *private_command);


scte35_splice_time* scte35_parse_splice_time(bs_t *b);
scte35_splice_insert_component* scte35_parse_splice_insert_component(bs_t *b, uint8_t splice_immediate_flag);
scte35_splice_event_component* scte35_parse_splice_event_component(bs_t *b);
scte35_break_duration* scte35_parse_break_duration (bs_t *b);

scte35_splice_descriptor* scte35_parse_splice_descriptor (bs_t *b);
scte35_splice_descriptor* scte35_copy_splice_descriptor (scte35_splice_descriptor* splice_descriptor);
void scte35_free_splice_descriptor (scte35_splice_descriptor* splice_descriptor);

scte35_splice_event* scte35_copy_splice_event(scte35_splice_event* splice_event);


#endif  // __H_SCTE35

