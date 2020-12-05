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

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include "ref/regex_ref.h"
#include "core/constants.h"
#include "recomp/re_match.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "core/core.h"
#ifdef __cplusplus
}
#endif

using namespace std;

class RegexRef
{
public:
    RegexRef()
    {
        patterns.clear();
        stats.clear();
        num_matched_packets = 0;
    }
    ~RegexRef() {}

    void push_pattern (string & in_patt)
    {
        patterns.push_back (in_patt);
    }

    void run_match()
    {
        if (patterns.size() == 0) {
            cout << "WARNING! No patterns in regex_ref" << endl;
        }

        if (packets.size() == 0) {
            cout << "WARNING! No packets in regex_ref" << endl;
        }

        for (uint32_t i = 0; i < packets.size(); i++) {
            for (uint32_t j = 0; j < patterns.size(); j++) {
                // PATTERN ID and PACKET ID start from 1
                if (gen_result (packets[i], i + 1, patterns[j], j + 1)) {
                    cout << "WARNING! Software reference model failed to generate results" << endl;
                    break;
                }
            }
        }
    }

    void push_packet (string & in_pkt)
    {
        packets.push_back (in_pkt);
    }

    int compare_result (uint32_t in_pkt_id, uint32_t in_patt_id, uint16_t in_offset)
    {
        if (stats.find (in_pkt_id) == stats.end()) {
            return -1;
        } else if (stats[in_pkt_id].find (in_patt_id) == stats[in_pkt_id].end()) {
            return -1;
        } else {
            for (int i = 0; i < (int) stats[in_pkt_id][in_patt_id].size(); i++) {
                if (in_offset == stats[in_pkt_id][in_patt_id][i]) {
                    // Matched results!
                    return 0;
                }
            }
        }

        return -1;
    }

    int get_num_matched_pkt()
    {
        return num_matched_packets;
    }

private:
    vector<string> patterns;
    vector<string> packets;

    // <key = PKT ID, value = <key = pattern_id, value = offset vector > >
    map<uint32_t, map<uint32_t, vector<uint16_t> > > stats;

    int num_matched_packets;

    int gen_result (string & in_pkt, uint32_t in_pkt_id,
                    string & in_patt, uint32_t in_patt_id)
    {
        int sub_str[8192];
        int result = re_match (in_patt.c_str(), in_pkt.c_str(), sub_str);

        if (result == -2) {
            cout << "WARNING! Pattern[ " << dec << in_patt_id << "] "
                 << in_patt << " compiled error" << endl;
            return -1;
        }

        if ((result > 0) && (in_pkt != "")) {
            if (stats[in_pkt_id].find (in_patt_id) == stats[in_pkt_id].end()) {
                for (int i = 0; i < result; i++) {
                    num_matched_packets++;
                    stats[in_pkt_id][in_patt_id].push_back (sub_str[2 * i + 1]);
                }
            } else {
                cout << "WARNING! Packet [" << dec << in_pkt_id << "]"
                     << " already had results for Pattern [" << dec << in_patt_id << "]!" << endl;
                return -1;
            }
        }

        return 0;
    }
};

RegexRef regex_ref;

void regex_ref_push_pattern (const char * in_patt_file)
{
    FILE * fp = fopen (in_patt_file, "r");
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    while ((read = getline (&line, &len, fp)) != -1) {
        remove_newline (line);
        read--;
        string patt (line);
        // Need to push in the patt id order
        regex_ref.push_pattern (patt);
    }

    fclose (fp);
}

void regex_ref_push_packet (const char * in_pkt_file)
{
    FILE * fp = fopen (in_pkt_file, "r");
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    while ((read = getline (&line, &len, fp)) != -1) {
        remove_newline (line);
        string pkt (line);
        regex_ref.push_packet (pkt);
    }
    fclose (fp);
}

void regex_ref_run_match()
{
    regex_ref.run_match();
}

int regex_ref_compare_result (uint32_t in_pkt_id, uint32_t in_patt_id, uint16_t in_offset)
{
    return regex_ref.compare_result (in_pkt_id, in_patt_id, in_offset);
}

int regex_ref_get_num_matched_pkt()
{
    return regex_ref.get_num_matched_pkt();
}
