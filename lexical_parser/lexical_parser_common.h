#ifndef LEXICAL_PARSER_COMMON_H
#define LEXICAL_PARSER_COMMON_H

#define N_CHAR                (256)
#define HEX_BASE              (16)
#define HEX_A                 (10)
#define MAX_OUTPUT_TOKEN      (16)
#define ASCII_BIT_NUM         (2)
#define PARSER_STR_LEN        (64)
#define MASK_LINE_NUM         (4)
#define BUCKETS_NUM           (8)
#define MAX_NUM_IN_BUCKET     (16)
#define TWO_CHAR_STR_LEN      (64)
#define MAX_BUCKET_NUM        (64)
#define MAX_SCORE             (512)

enum hex_code_pos_e {
    HEX_CODE_POS_0 = 0, /* the position of '%' */
    HEX_CODE_POS_1, /* the position of first char after '%' */
    HEX_CODE_POS_2, /* the position of second char after '%' */
    N_HEX_CODE
};

#endif /* LEXICAL_PARSER_COMMON_H */