// ****************************************************************
// (C) Copyright My Captains Corporation 2017
// Author: Gou Peng Fei (pengfeidaxia@163.com)
// ****************************************************************
#ifndef REGEX_CONFIG
#define REGEX_CONFIG

#include "fregex.h"

class RegexConfig
{
    public:
        RegexConfig(State* in_states,
                    Token* in_tokens,
                    Match_Locat match_locat,
                    int    in_max_char_num,
                    int    in_max_token_num,
                    int    in_max_token_len,
                    int    in_max_state_num,
                    int    in_max_count_num);

        ~RegexConfig();

        int  getConfig(int in_s_loc, int in_t_loc, unsigned char* out_config_bytes);
        void dumpConfig(int in_config_len);

    private:
        State* states;
        Token* tokens;
        Match_Locat match_locat;
        int maxCharNum;
        int maxTokenNum;
        int maxTokenLen;
        int maxStateNum;
        int maxCountNum;

        // Size in bytes of each section in config array
        int sizeTokenArray;
        int sizeStateLength;
        int sizeMatchAtHead;
        int sizeMatchAtTail;
        int sizeMatchStates;
        int sizetSCTable;
        int sizetSsSTable;
        int sizeSstickyTable;
        int sizeSstickyVector;
        int sizeCountVector;

        unsigned char* configArray;
        int sizeConfigArray;

        void printTableHeader(int in_width, char in_prefix);
};

#endif
