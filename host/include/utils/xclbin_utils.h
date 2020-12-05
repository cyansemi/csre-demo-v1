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

#include "xclbin.h"
#include <boost/property_tree/json_parser.hpp>

static int
xclbinGetKeyValue (const char * xclbin_file, std::string key)
{
    std::ifstream stream (xclbin_file);
    stream.seekg (0, stream.end);
    int size = stream.tellg();
    stream.seekg (0, stream.beg);

    char * header = new char[size];
    stream.read (header, size);

    if (std::strncmp (header, "xclbin2", 8)) {
        throw std::runtime_error ("Invalid bitstream");
    }

    const axlf * top = (const axlf *)header;
    auto kv_hdr = xclbin::get_axlf_section (top, KEYVALUE_METADATA);

    if (NULL == kv_hdr) {
        printf ("ERROR: no key-value section available in xclbin!\n");
        return -1;
    }

    char * kv = (char *) (header + kv_hdr->m_sectionOffset);
    uint64_t kv_size = kv_hdr->m_sectionSize;

    std::unique_ptr<unsigned char> memBuffer (new unsigned char[kv_size + 1]);

    memcpy ((char *) memBuffer.get(), kv, kv_size);
    memBuffer.get()[kv_size] = '\0';

    std::stringstream ss;
    ss.write((char *) memBuffer.get(), kv_size);
    boost::property_tree::ptree tree;

    try {
        boost::property_tree::read_json (ss, tree); 
    } catch (const std::exception & e) {
        std::string msg ("ERROR: Bad JSON format detected while marshling keyvalue metadata (");
        msg += e.what();
        msg += ").";
        throw std::runtime_error (msg);
    }

    if (tree.find ("key_values") != tree.not_found()) {
        for (auto & item : tree.get_child ("key_values")) {
            if (key == item.second.get<std::string>("key")) {
                return item.second.get<int>("value");
            }
        }
    } else {
        printf ("ERROR: unable to find key %s in xclbin's KEYVALUE METADATA!\n", key.c_str());
    }

    return -1;
}
