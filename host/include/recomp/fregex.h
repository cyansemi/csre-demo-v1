/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright 2016 - 2017 Systems Group, ETH Zurich
 */
#ifndef F_REGEX
#define F_REGEX

#ifndef MAX_CHARS
#define MAX_CHARS 256
#endif

#ifndef MAX_STATES
#define MAX_STATES 128
#endif

//#define NODEBUG
#ifdef NODEBUG
#define DEBUG false
#else
#define DEBUG true
#endif

//#define bool short
#define true 1
#define false 0

#define SUPPORTS_CASE_INSENSITIVE false

#define KRED  "\x1B[31m"
#define RESET "\x1B[0m"

typedef struct Token {
    char characters[MAX_CHARS];
    bool is_range;
    bool is_choice;
    bool is_not;
    int size;
    int length;
    int real_offs;
    int char_pos;
} Token ;

typedef struct State {
    int id;
    int out_edge[MAX_STATES];
    int out_cnt;
    int in_edge[MAX_STATES];
    int in_cnt;
    bool is_sticky;
    bool has_sticky_src;
    int sticky_src_cnt;
    int sticky_src_edge[MAX_STATES];
    int tokens[MAX_CHARS];
    int token_cnt;
    bool is_accepting;
    int lower_count;
    int upper_count;
    bool is_count;
} State ;

typedef struct Match_Locat {
    bool match_at_head;
    bool match_at_tail;
} Match_Locat ;

#ifdef __cplusplus
extern "C" {
#endif
    int fregex_get_config (const char* regex_string,
                           int max_char_cnt,
                           int max_state_cnt,
                           int max_token_cnt,
                           int max_char_per_token,
                           int max_count_cnt,
                           unsigned char* config_bytes,
                           int* config_len,
                           int dump_state);
#ifdef __cplusplus
}
#endif

#endif
