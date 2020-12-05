/* 
 *  Copyright 2020 CyanSemi Semiconductor Co.
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *  
 *      http://www.apache.org/licenses/LICENSE-2.0
 *  
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *  
 *  Date: Sun May 03 2020
 */

#ifndef F_REGEX_REF
#define F_REGEX_REF

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif
void       regex_ref_push_pattern (const char * in_patt_file);
void       regex_ref_push_packet (const char * in_pkt_file);
void       regex_ref_run_match();
int        regex_ref_compare_result (uint32_t in_pkt_id, uint32_t in_patt_id, uint16_t in_offset);
int        regex_ref_get_num_matched_pkt();
#ifdef __cplusplus
}
#endif

#endif
