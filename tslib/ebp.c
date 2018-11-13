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

#include <ebp.h>
#include <bs.h>
#include <arpa/inet.h>
#include "ATSTestReport.h"


ebp_t *ebp_new()
{
   return calloc(1, sizeof(ebp_t));
}

void ebp_free(ebp_t *ebp)
{
   if (ebp != NULL)
   {
      free(ebp);
   }
}

int ebp_read(ebp_t *ebp, ts_scte128_private_data_t *scte128)
{
   if (ebp == NULL || scte128 == NULL)
   {
      return 0;
   }

   bs_t *b = bs_new(scte128->private_data_bytes.bytes, scte128->private_data_bytes.len);

   ebp->ebp_fragment_flag = bs_read_u1(b);
   ebp->ebp_segment_flag = bs_read_u1(b);
   ebp->ebp_sap_flag = bs_read_u1(b);
   ebp->ebp_grouping_flag = bs_read_u1(b);
   ebp->ebp_time_flag = bs_read_u1(b);
   ebp->ebp_concealment_flag = bs_read_u1(b);

   bs_skip_u1(b);

   ebp->ebp_extension_flag = bs_read_u1(b);

   if (ebp->ebp_extension_flag)
   {
      ebp->ebp_ext_partition_flag = bs_read_u1(b);
      bs_skip_u(b,7);
   }

   if (ebp->ebp_sap_flag)
   {
      ebp->ebp_sap_type = bs_read_u(b, 3);
      bs_skip_u(b, 5);
   }

   if (ebp->ebp_grouping_flag)
   {
      ebp->ebp_grouping_ids = vqarray_new();
      uint32_t more = 1;
      while (more)
      {
         more = bs_read_u1(b);
         uint32_t *grouping_id = calloc(1, sizeof(uint32_t));
         *grouping_id = bs_read_u(b, 7);
         vqarray_add(ebp->ebp_grouping_ids, (vqarray_elem_t*)grouping_id);
      }

   }

   if (ebp->ebp_time_flag)
   {
      ebp->ebp_acquisition_time = bs_read_u64(b);
 //     ebp->ebp_acquisition_time = ntohll(ebp->ebp_acquisition_time);
   }

   if (ebp->ebp_ext_partition_flag)
   {
      ebp->ebp_ext_partitions = bs_read_u8(b);
   }

   bs_free (b);

   ebp_print_stdout(ebp);

   return 1;
}

int ebp_validate_groups(const ebp_t *ebp)
{
   // GroupId range:
   //    0: default
   //    1-31: private
   //    35: Ad insertion
   //    126: start
   //    127: end
   
   // For start, group Id is followed by 126; for end it is followed by 127; could
   // be followed by both 126 and 127.
   //
   // except for 126/127, group id cannot appear more than once
   //
   // groupid does not require associated 126 or 127


   // check range of each group id
   // check that no group ids are present more than once
   // check that every 126 is preceded by another groupid
   // check that every 127 is preceded by either 126 or another groupid

   int returnCode = 0;

   if (ebp->ebp_grouping_flag == 0)
   {
      return returnCode;
   }

   // first, check for grouping id duplicates
   for (int i=0; i<vqarray_length (ebp->ebp_grouping_ids); i++)
   {
      uint32_t grouping_id_i = *((uint32_t *) vqarray_get (ebp->ebp_grouping_ids, i));

      for (int j=0; j<vqarray_length (ebp->ebp_grouping_ids); j++)
      {
         if (i==j) continue;
      
         uint32_t grouping_id_j = *((uint32_t *) vqarray_get (ebp->ebp_grouping_ids, j));

         if (grouping_id_i == grouping_id_j)
         {
            LOG_ERROR_ARGS ("ebp_validate_groups: FAIL: duplicate group id %d detected", grouping_id_i);
            reportAddErrorLogArgs ("ebp_validate_groups: FAIL: duplicate group id %d detected", grouping_id_i);
            returnCode = -1;
         }
      }
   }


   for (int i=0; i<vqarray_length (ebp->ebp_grouping_ids); i++)
   {
      uint32_t grouping_id = *((uint32_t *) vqarray_get (ebp->ebp_grouping_ids, i));

      if (grouping_id == 126)
      {
         if (i == 0)
         {
            LOG_ERROR ("ebp_validate_groups: FAIL: orphan group id 126 detected (1)");
            reportAddErrorLog ("ebp_validate_groups: FAIL: orphan group id 126 detected (1)");
            returnCode = -1;
            continue;
         }

         uint32_t previous_grouping_id = *((uint32_t *) vqarray_get (ebp->ebp_grouping_ids, i-1));
         if (previous_grouping_id == 126 || previous_grouping_id == 127)
         {
            LOG_ERROR ("ebp_validate_groups: FAIL: orphan group id 126 detected (2)");
            reportAddErrorLog ("ebp_validate_groups: FAIL: orphan group id 126 detected (2)");
            returnCode = -1;
         }
      }
      else if (grouping_id == 127)
      {
         if (i == 0)
         {
            LOG_ERROR ("ebp_validate_groups: FAIL: orphan group id 127 detected (1)");
            reportAddErrorLog ("ebp_validate_groups: FAIL: orphan group id 127 detected (1)");
            returnCode = -1;
            continue;
         }

         uint32_t previous_grouping_id = *((uint32_t *) vqarray_get (ebp->ebp_grouping_ids, i-1));
         if (previous_grouping_id == 127)
         {
            LOG_ERROR ("ebp_validate_groups: FAIL: orphan group id 127 detected (2)");
            reportAddErrorLog ("ebp_validate_groups: FAIL: orphan group id 127 detected (2)");
            returnCode = -1;
         }
      }
   }

   return returnCode;
}

ebp_t* ebp_copy(const ebp_t *ebp)
{
   ebp_t * ebpNew = ebp_new();
         
   ebpNew->ebp_fragment_flag = ebp->ebp_fragment_flag;
   ebpNew->ebp_segment_flag = ebp->ebp_segment_flag;
   ebpNew->ebp_sap_flag = ebp->ebp_sap_flag;
   ebpNew->ebp_grouping_flag = ebp->ebp_grouping_flag;
   ebpNew->ebp_time_flag = ebp->ebp_time_flag;
   ebpNew->ebp_concealment_flag = ebp->ebp_concealment_flag;
   ebpNew->ebp_extension_flag = ebp->ebp_extension_flag;
   ebpNew->ebp_ext_partition_flag = ebp->ebp_ext_partition_flag;
   ebpNew->ebp_sap_type = ebp->ebp_sap_type;
   ebpNew->ebp_acquisition_time = ebp->ebp_acquisition_time;
   ebpNew->ebp_ext_partitions = ebp->ebp_ext_partitions;

   if (ebpNew->ebp_grouping_flag)
   {
      ebpNew->ebp_grouping_ids = vqarray_new();
      for (int i=0; i<vqarray_length(ebp->ebp_grouping_ids); i++)
      {
         vqarray_add(ebpNew->ebp_grouping_ids, vqarray_get(ebp->ebp_grouping_ids, i));
      }
   }

   return ebpNew;
}

int ebp_print(const ebp_t *ebp, char *str, size_t str_len)
{
   return 1;
}

void ebp_print_stdout(const ebp_t *ebp)
{
    LOG_INFO ("EBP struct:");
    LOG_INFO_ARGS ("   ebp_fragment_flag = %d", ebp->ebp_fragment_flag);
    LOG_INFO_ARGS ("   ebp_segment_flag = %d", ebp->ebp_segment_flag);
    LOG_INFO_ARGS ("   ebp_sap_flag = %d", ebp->ebp_sap_flag);
    LOG_INFO_ARGS ("   ebp_grouping_flag = %d", ebp->ebp_grouping_flag);
    LOG_INFO_ARGS ("   ebp_time_flag = %d", ebp->ebp_time_flag);
    LOG_INFO_ARGS ("   ebp_concealment_flag = %d", ebp->ebp_concealment_flag);
    LOG_INFO_ARGS ("   ebp_extension_flag = %d", ebp->ebp_extension_flag);
    LOG_INFO_ARGS ("   ebp_ext_partition_flag = %d", ebp->ebp_ext_partition_flag);
    LOG_INFO_ARGS ("   ebp_sap_type = %d", ebp->ebp_sap_type);
    
    int num_ebp_grouping_ids = 0;
    if ((uint32_t)ebp->ebp_grouping_ids != 0)
    {
        LOG_INFO_ARGS ("   ebp_grouping_ids = %x", (uint32_t)ebp->ebp_grouping_ids);
        num_ebp_grouping_ids = vqarray_length(ebp->ebp_grouping_ids);
        LOG_INFO_ARGS ("   num_ebp_grouping_ids = %d", num_ebp_grouping_ids);
        for (int i=0; i<num_ebp_grouping_ids; i++)
        {
            LOG_INFO_ARGS ("       ebp_grouping_ids[%d] = %d", i, *((uint32_t*)vqarray_get(ebp->ebp_grouping_ids, i)));
        }
    }
    else
    {
        LOG_INFO_ARGS ("   num_ebp_grouping_ids = %d", num_ebp_grouping_ids);
    }

    LOG_INFO_ARGS ("   ebp_acquisition_time = %"PRId64"", ebp->ebp_acquisition_time);
    LOG_INFO_ARGS ("   ebp_ext_partitions = %d", ebp->ebp_ext_partitions);

}

static descriptor_t* ebp_descriptor_new(descriptor_t *desc)
{
   ebp_descriptor_t *ebp = NULL;
   ebp = (ebp_descriptor_t *)calloc(1, sizeof(ebp_descriptor_t));
   ebp->descriptor.tag = EBP_DESCRIPTOR;
   if (desc != NULL)
   {
      ebp->descriptor.length = desc->length;
      free(desc);
   }
   return (descriptor_t *)ebp;
}

int ebp_descriptor_free(descriptor_t *desc)
{
//   LOG_INFO_ARGS ("ebp_descriptor_free: %x", (unsigned int)desc);
   if (desc == NULL) return 0;
   if (desc->tag != EBP_DESCRIPTOR) return 0;

    ebp_descriptor_t *ebp = (ebp_descriptor_t *)desc;

   // walk the partition_data array and free all entries
   // then free partition_data using vqarray_free(vqarray_t* v)
   while (1)
   {
      ebp_partition_data_t * partitionData = vqarray_pop(ebp->partition_data);
      if (partitionData == NULL)
      {
         break;
      }
      free (partitionData);
   }
   vqarray_free(ebp->partition_data);

   free(ebp);

   return 1;
}

descriptor_t* ebp_descriptor_read(descriptor_t *desc, bs_t *b)
{
   if ((desc == NULL) || (b == NULL)) return NULL;

   
 /*  printf ("ebp_descriptor_read: %d: ", desc->length);
   uint8_t *temp = b->p;
   for (int i=0; i<desc->length; i++)
   {
      printf ("0x%x ", *temp);
      temp++;
   }
   printf ("\n\n");
   */
  

   ebp_descriptor_t *ebp =
         (ebp_descriptor_t *)ebp_descriptor_new(desc);
//   LOG_INFO_ARGS ("ebp_descriptor_read: %x", (unsigned int)ebp);

   ebp->num_partitions = bs_read_u(b, 5);
   ebp->timescale_flag = bs_read_u1(b);
   bs_skip_u(b, 2);

   ebp->ticks_per_second = (ebp->timescale_flag) ? bs_read_u(b, 21) : 1;
   ebp->ebp_distance_width_minus_1 = (ebp->timescale_flag) ? bs_read_u(b, 3) : 0;

   if (ebp->num_partitions > 0)
   {
      int i = ebp->num_partitions;
      ebp->partition_data = vqarray_new();
      while (i-- > 0)
      {
         ebp_partition_data_t *partition_data = calloc(1, sizeof(ebp_partition_data_t));
         partition_data->ebp_data_explicit_flag = bs_read_u1(b);
         partition_data->representation_id_flag = bs_read_u1(b);
         partition_data->partition_id = bs_read_u(b, 5);

         if (partition_data->ebp_data_explicit_flag == 0)
         {
            bs_skip_u1(b);
            partition_data->ebp_pid = bs_read_u(b, 13);
            bs_skip_u(b, 3);
         }
         else
         {
            int ebp_distance_width = (ebp->ebp_distance_width_minus_1 + 1) * 8;
            partition_data->boundary_flag = bs_read_u1(b);
            partition_data->ebp_distance = bs_read_u(b, ebp_distance_width);

            if (partition_data->boundary_flag == 1)
            {
               partition_data->sap_type_max = bs_read_u(b, 3);
               bs_skip_u(b, 4);
            }
            else
            {
               bs_skip_u(b, 7);
            }

            partition_data->acquisition_time_flag = bs_read_u1(b);
         }

         if (partition_data->representation_id_flag == 1)
         {
            partition_data->representation_id = bs_read_u64(b);
//            partition_data->representation_id = ntohll(partition_data->representation_id);
         }

         vqarray_add(ebp->partition_data, partition_data);
      }
   }

//   ebp_descriptor_print_stdout(ebp);

   return (descriptor_t *)ebp;
}

int ebp_descriptor_print(const descriptor_t *desc, int level, char *str, size_t str_len)
{
   int bytes = 0;
   if (desc == NULL) return 0;
   if (desc->tag != EBP_DESCRIPTOR) return 0;

   ebp_descriptor_t *maxbr = (ebp_descriptor_t *)desc;

   bytes += SKIT_LOG_UINT_VERBOSE(str + bytes, level, desc->tag, "ebp_descriptor", str_len - bytes);
   bytes += SKIT_LOG_UINT(str + bytes, level, desc->length, str_len - bytes);

   //bytes += SKIT_LOG_UINT(str + bytes, level, maxbr->max_bitrate, str_len - bytes);
   return bytes;
}

ebp_descriptor_t* ebp_descriptor_copy(const ebp_descriptor_t *ebp_in)
{
   if (ebp_in == NULL)
   {
      return NULL;
   }

   ebp_descriptor_t *ebp = (ebp_descriptor_t *)calloc(1, sizeof(ebp_descriptor_t));
   
   LOG_INFO_ARGS ("ebp_descriptor_copy: from %x to %x", (unsigned int)ebp_in, (unsigned int)ebp);
   ebp->descriptor.tag = EBP_DESCRIPTOR;
   ebp->descriptor.length = ebp_in->descriptor.length;


   ebp->num_partitions = ebp_in->num_partitions;
   ebp->timescale_flag = ebp_in->timescale_flag;
   ebp->ticks_per_second = ebp_in->ticks_per_second;
   ebp->ebp_distance_width_minus_1 = ebp_in->ebp_distance_width_minus_1;

   if (ebp->num_partitions > 0)
   {
      ebp->partition_data = vqarray_new();

      for (int i=0; i<ebp->num_partitions; i++)
      {
         ebp_partition_data_t *partition_data_in = (ebp_partition_data_t *) vqarray_get(ebp_in->partition_data, i);

         ebp_partition_data_t *partition_data = calloc(1, sizeof(ebp_partition_data_t));
         partition_data->ebp_data_explicit_flag = partition_data_in->ebp_data_explicit_flag;
         partition_data->representation_id_flag = partition_data_in->representation_id_flag;
         partition_data->partition_id = partition_data_in->partition_id;
         partition_data->ebp_pid = partition_data_in->ebp_pid;
         partition_data->boundary_flag = partition_data_in->boundary_flag;
         partition_data->ebp_distance = partition_data_in->ebp_distance;
         partition_data->sap_type_max = partition_data_in->sap_type_max;
         partition_data->acquisition_time_flag = partition_data_in->acquisition_time_flag;
         partition_data->representation_id = partition_data_in->representation_id;

         vqarray_add(ebp->partition_data, partition_data);
      }
   }

//   ebp_descriptor_print_stdout(ebp);

   return ebp;
}


void ebp_descriptor_print_stdout(const ebp_descriptor_t *ebp_desc)
{
   LOG_INFO ("EBP Descriptor:");
   LOG_INFO_ARGS ("   tag = %d, length = %d, timescale_flag = %d, ticks_per_second = %d, ebp_distance_width_minus_1 = %d", 
      ebp_desc->descriptor.tag, ebp_desc->descriptor.length, 
      ebp_desc->timescale_flag, ebp_desc->ticks_per_second, ebp_desc->ebp_distance_width_minus_1);

   int num_partitions = 0;
   if (ebp_desc->partition_data != NULL)
   {
      num_partitions = vqarray_length(ebp_desc->partition_data);
      LOG_INFO_ARGS ("   num_partitions = %d", num_partitions);

      for (int i=0; i<num_partitions; i++)
      {
         ebp_partition_data_t* partition = (ebp_partition_data_t*)vqarray_get(ebp_desc->partition_data, i);

         LOG_INFO_ARGS ("   partition[%d]:", i);
         LOG_INFO_ARGS ("      ebp_data_explicit_flag = %d, representation_id_flag = %d, partition_id = %d, ebp_pid = %d", 
            partition->ebp_data_explicit_flag, partition->representation_id_flag, partition->partition_id,
            partition->ebp_pid);
         LOG_INFO_ARGS ("      boundary_flag = %d, ebp_distance = %d, sap_type_max = %d, acquisition_time_flag = %d, representation_id = %"PRId64"", 
            partition->boundary_flag, partition->ebp_distance, partition->sap_type_max, 
            partition->acquisition_time_flag, partition->representation_id);
      }
   }
   else
   {
      LOG_INFO_ARGS ("   num_partitions = %d:", num_partitions);
   }
}

ebp_partition_data_t* get_fragment_partition (const ebp_descriptor_t *ebp_desc)
{
   return get_partition (ebp_desc, EBP_PARTITION_FRAGMENT);
}

ebp_partition_data_t* get_segment_partition (const ebp_descriptor_t *ebp_desc)
{
   return get_partition (ebp_desc, EBP_PARTITION_SEGMENT);
}

ebp_partition_data_t* get_partition (const ebp_descriptor_t *ebp_desc, int partitionId)
{
   for (int i=0; i<vqarray_length(ebp_desc->partition_data); i++)
   {
      ebp_partition_data_t* partition = (ebp_partition_data_t *) vqarray_get(ebp_desc->partition_data, i);
      if (partition->partition_id == partitionId)
      {
         return partition;
      }
   }

   return NULL;
}


int does_fragment_mark_boundary (const ebp_descriptor_t *ebp_desc)
{
   // walk partitions looking for partitionId == 2
   // then return boundary_flag

   for (int i=0; i<vqarray_length(ebp_desc->partition_data); i++)
   {
      ebp_partition_data_t* partition = (ebp_partition_data_t *) vqarray_get(ebp_desc->partition_data, i);
      if (partition->partition_id == EBP_PARTITION_FRAGMENT)
      {
         if (partition->ebp_data_explicit_flag)
         {
            return partition->boundary_flag;
         }
         else
         {
            return 0;
         }
      }
   }

   return 0;
}

int does_segment_mark_boundary (const ebp_descriptor_t *ebp_desc)
{
   // walk partitions looking for partitionId == 1
   // then return boundary_flag

   for (int i=0; i<vqarray_length(ebp_desc->partition_data); i++)
   {
      ebp_partition_data_t* partition = (ebp_partition_data_t *) vqarray_get(ebp_desc->partition_data, i);
      if (partition->partition_id == EBP_PARTITION_SEGMENT)
      {
         if (partition->ebp_data_explicit_flag)
         {
            return partition->boundary_flag;
         }
         else
         {
            return 0;
         }
      }
   }

   return 0;
}

uint8_t get_fragment_SAP_max (const ebp_descriptor_t *ebp_desc)
{
   // walk partitions looking for partitionId == 1
   for (int i=0; i<vqarray_length(ebp_desc->partition_data); i++)
   {
      ebp_partition_data_t* partition = (ebp_partition_data_t *) vqarray_get(ebp_desc->partition_data, i);
      if (partition->partition_id == EBP_PARTITION_FRAGMENT)
      {
         return partition->sap_type_max;
      }
   }

   return 0;
}

uint8_t get_segment_SAP_max (const ebp_descriptor_t *ebp_desc)
{
   // walk partitions looking for partitionId == 1
   for (int i=0; i<vqarray_length(ebp_desc->partition_data); i++)
   {
      ebp_partition_data_t* partition = (ebp_partition_data_t *) vqarray_get(ebp_desc->partition_data, i);
      if (partition->partition_id == EBP_PARTITION_SEGMENT)
      {
         return partition->sap_type_max;
      }
   }

   return 0;
}

uint64_t ntohll(uint64_t num)
{
    uint64_t num2 = (((uint64_t)ntohl((unsigned int)num)) << 32) + (uint64_t)ntohl((unsigned int)(num >> 32));
    return num2;
}

void parseNTPTimestamp(uint64_t ntpTime, uint32_t *numSeconds, float *fractionalSecond)
{
   *numSeconds = (uint32_t)(ntpTime >> 32);

   uint64_t twoToTheThirtytwo = 0x100000000;
   uint64_t lowerThirtytwoMask = 0x00000000FFFFFFFF;

   // careful with data sizes here
   double numerator = (double)(ntpTime & lowerThirtytwoMask);
   double denominator = (double)twoToTheThirtytwo;

   double fraction = (numerator / denominator);
   *fractionalSecond = fraction;

 //  LOG_INFO_ARGS ("fraction = %f", fraction);
}

