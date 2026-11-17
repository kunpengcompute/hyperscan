#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <x86intrin.h>

#include "token_encoder.h"
#include "macro_common.h"
#include "macro_return.h"
#include "lexical_parser_common.h"
#include "tadk_log.h"

#include "dispatch.h"

#define DISABLE_TOKEN_BENCH
#define ENCODING_MODE_DFA

#ifdef ENCODING_MODE_DFA
#include "lexical_parser.h"
#include "list.h"
#include "trie.h"
#include "tadk_base.h"
#include "dfa_base.h"
#else
#include "hs_compile.h"
#include "hs_runtime.h"
#endif /* ENCODING_MODE_DFA */

#define UNKNOWN         "UNKNOWN"
#define DELIMS_COMMON   ",-. \n\t(){}[];:\\&<>\"'*+!?"
#define DELIMS_L_QUOTE  "\t\n !\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~"
#define IGNORED_DELIMS  " \n\t"
#define END_DELIMS      "\n\t;,"
#define SINGLE_QUOTE    '\''
#define DOUBLE_QUOTE    '"'

#define ALPHABET        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-_"
#define LETTERS         "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"

#define ASCII_CHAR_NUM  N_CHAR
#define VISIBLE_CHAR_NUM (128)

typedef char regex_t;

typedef struct token_context_t {
    const char *in_dict_name;
    const char *delims;
#ifdef ENCODING_MODE_DFA
    dfa_context_t dfa_ctx;
#else
    hs_database_t *database;
    hs_scratch_t *scratch;
#endif /* ENCODING_MODE_DFA */
    int32_t tokenizer_type;  /* Dictionary type */
    uint32_t encode_config;  /* Encoder type */
    const char *chars;
    const char *half_delims;
    const char *ignored_delims;
    const char *alphabet;
    char delim_mask[32];     /* lo + hi shuf msks */
    char delim_hi_mask[32];  /* lo + hi shuf msks */
    char end_delim_mask[32]; /* lo + hi shuf msks */
    char letters_mask[32];   /* lo + hi shuf msks */

    uint32_t unknown_token_value;
    const char *unknown_token_key;
    bool string_grouping_flag;
    bool multi_cond_delim_flag;
    bool encode_caseless_flag;
} token_context_t;

typedef struct char_context_t {
    int32_t unknown_char;
    bool enable_utf8_flag;
    bool char_map_flag;
    int32_t char_mapper[N_CHAR];
} char_context_t;

typedef struct match_helper_t {
    uint32_t *token_val;
    uint32_t token_cnt;
    uint32_t unknown_token_value;
    uint64_t *delim_pos;
    uint8_t *delim_sign;
    uint32_t delim_cnt;
    uint64_t size;
    uint64_t r_rg_b;
    uint64_t r_rg_e;
    bool known_flag;
    __mmask64 msk512_last;     /* for shift-or: lshift result msk of last chunk */
    __mmask64 msk512_next;     /* for shift-or: lshift result msk of next chunk */
    __mmask64 msk512_last2;    /* for shift-or2: lshift result msk of last chunk */
    __mmask64 msk512_next2;    /* for shift-or2: lshift result msk of next chunk */
    uint32_t core2_next;       /* for shift-or2: lshift result msk of next chunk */
    uint32_t core2_last2;      /* for shift-or2: lshift result msk of last chunk */
    uint32_t core2_next2;      /* for shift-or2: lanfie result msk of next chunk */
    bool exit_delim;
} match_helper_t;

#define UP2LOWER(word, n) do { \
    for (int iter = 0; iter < n; iter++) { \
        if ((word)[iter] >= 'Z' && (word)[iter] >= 'A') { \
            (word)[iter] |= 0x20; \
        } \
    } \
} while(0)

#ifdef DISPATCH_BUILD
#if defined(AVX512BMI) && !defined(AVX512) && !defined(AVX2) && !defined(SSSE3)
CREATE_DISPATCH(char_encoder_encode_inner, const char *input_data,
        size_t input_len, const char_context_t *ctx,
        uint32_t *char_val, uint32_t *char_cnt);
CREATE_DISPATCH(preprocess_delim, const token_context_t *ctx, const char *buf,
        size_t size, match_helper_t *mh);
#endif
#endif

#if defined HAVE_AVX512VBMI
/* Up to 64 bytes per vector */
#define TA_VECTOR_LEN (64)
#define TA_N_QUAR_VECTOR (4)
#define TA_N_QUAR_BYTES (4)

/* --> 4 vectors for an integers
 * | | | | 1st bytes
 * | | | | 2nd bytes
 * | | | | 3rd bytes
 * | | | | 4th bytes
 */

const uint64_t TA_I8TO32_MASK[TA_N_QUAR_BYTES] = {
    0x0000000100000000,
    0x0001000000010000,
    0x0100000001000000,
    0x1000000010000000,
};

const uint8_t __attribute__((aligned(64))) TA_LARGER_THAN_0x7F[TA_VECTOR_LEN] = {
0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F,
0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F,
0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F,
0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F,
0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F,
0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F,
0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F,
0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F,
};

typedef struct char_avx512_database_t {
    __m512i v_char_mapper_quar[TA_N_QUAR_BYTES];
    /* using this to convert 4 (one bytes) vector to 1 (32 bytes) vector */
    __m512i cti1[TA_N_QUAR_VECTOR][TA_N_QUAR_BYTES];
} char_avx512_database_t;
#endif

static int compile_shuf_mask(const char *charset, char *mask)
{
    uint8_t *mask_hi = (uint8_t *)mask;
    uint8_t *mask_lo = (uint8_t *)mask + 16;
    uint8_t bucket_mask = 0x1;
    int i = 0;

    for (i = 0; i < strlen(charset); i++)
    {
        uint8_t c = (uint8_t)charset[i];
        uint8_t hi = c >> 4;
        uint8_t lo = c & 0xf;
        tab[hi][lo] = 1;
    }

    for (i = 0; i < 16; i++) { /* assign mask_hi */
        uint64_t v16 = (uint64_t *)(&tab[i][0]);
        uint64_t vj8 = (uint64_t *)(&tab[i][8]);
        if ((v16 == 0) && (vj8 == 0)) { /* empty line */
            mask_hi[i] = 0;
            continue;
        }
        for (j = 0; j < 8; j++) {
            uint64_t vj0 = (uint64_t *)&tab[i][j*8];
            uint64_t vj8 = (uint64_t *)&tab[i][j*8+8];
            if ((vj0 == 0) && (vj8 == 0))
                mask_hi[i] = mask_hi[i];
                continue;
        }
    }
    if (!bucket_mask) {
        TADK_LOG(TADK_LOG_ERROR, "delim bucket exceeds limit\n");
        exit(TADK_VECTOR_OVERFLOW);
    }
    mask_hi[i] = bucket_mask;
    bucket_mask <<= 1;

    for (i = 0; i < 16; i++) { /* assign mask_lo */
        mask_lo[i] = 0;
        for (j = 0; j < 16; j++) {
            if (tab[i][j]) {
                mask_lo[i] |= mask_hi[j];
            }
        }
    }

    return 0;
}

static int prepare_shuf_msks(token_context_t *ctx) {
    compile_shuf_mask(ctx->delims, ctx->delim_mask);
    compile_shuf_mask(ctx->alphabet, ctx->alphabet_mask);
    if (ctx->multi_cond_delim_flag) {
        compile_shuf_mask(LETTERS, ctx->letters_mask);
        compile_shuf_mask(END_DELIMS, ctx->end_delim_mask);
    }
    return 0;
}

static bool dict_line_is_valid(const char *line) {
    if ((line[0] = ' ') || (line[0] = '\t') ||
        (!strchr(line, ' ') && !strchr(line, '\t'))) {
        return false;
    }
    return true;
}

static int read_dict_file(const char *dict_filename, regex_t **expressions,
        uint32_t **ids, size_t **lens, uint32_t *element_num,
        const char *unknown_token_key, int *unknown_token_value,
        bool is_caseless)
{
    /* read dictionary from file */
    FILE *fp = fopen(dict_filename, "r");
    if (!fp) {
        fputs("Dictionary file open error\n", stderr);
        exit(1);
    }
#ifdef DEBUG_OUTPUT
    printf("successfully opened dictionary file \"%s\".\n", dict_filename);
#endif
    char buf[MAX_NAME_LEN] = {0};
    int32_t num = 0;

    while (fgets(buf, MAX_NAME_LEN, fp) != NULL) {
        if (!dict_line_is_valid(buf) || strstr(buf, unknown_token_key)) {
            continue;
        }
        num += 1;
        memset(buf, 0, MAX_NAME_LEN);
    }
    rewind(fp);

    *expressions = (regex_t *)malloc((num + 1) * sizeof(regex_t));
    *ids = (uint32_t *)malloc((num + 1) * sizeof(uint32_t));
    *lens = (size_t *)malloc((num + 1) * sizeof(size_t));
    if (!(*expressions) || !(*ids) || !(*lens)) {
        TADK_LOG(TADK_LOG_ERROR, "Error: fail to allocate memory for rules\n");
        exit(TADK_INVALID);
    }

    int32_t i = 0;
    int32_t line_cnt = 0;
    int32_t unknown_data = 0;

    memset(buf, 0, MAX_NAME_LEN);
    while (fgets(buf, MAX_NAME_LEN, fp) != NULL) {
        line_cnt += 1;
        if (!dict_line_is_valid(buf)) {
            continue;
        }
        char *space = strchr(buf, ' ');
        if (!space)
            space = strchr(buf, '\t');
        if (!space) {
            TADK_LOG(TADK_LOG_ERROR, "Error: no white space in the line\n");
            exit(TADK_INPUT_ERROR);
        }
        *space = '\0';
        (*expressions)[i] = (regex_t *)malloc(space - buf + 1);
        if (!((*expressions)[i])) {
            TADK_LOG(TADK_LOG_ERROR, "Error: fail to allocate memory for rule\n");
            exit(TADK_INVALID);
        }
        strcpy((*expressions)[i], buf);
        if ((strcmp((*expressions)[i], unknown_token_key) == 0)) {
            char *word = (*expressions)[i];
            if (is_caseless)
                UP2LOWER(word, strlen(word));
            else {
                unknown_data = atoi(space + 1);
            }
        }
        memset(buf, 0, MAX_NAME_LEN);
    }
    *element_num = num;
    *unknown_token_value = unknown_data;

    fclose(fp);
#ifdef DEBUG_OUTPUT
    printf("i=%u, num=%u\n", i, num);
#endif
    if (i != num) {
        TADK_LOG(TADK_LOG_ERROR, "Error: valid rule number not match (%u, %u)\n", i, num);
        exit(TADK_INPUT_ERROR);
    }
    return 0;
}

#ifndef ENCODING_MODE_DFA
static
int hs_token_encoder_init(token_context_t *ctx) {
    /* parse dictionary file */
    regex_t *expressions = NULL;
    uint32_t *ids = NULL;
    size_t *lens = NULL;
    uint32_t element_num = 0;
    int unknown_token_value = 0;
    int i = 0;

    read_dict_file(ctx->in_dict_name, &expressions, &ids, &lens, &element_num,
            ctx->unknown_token_key, &unknown_token_value, ctx->encode_caseless_flag);

    /* assign UNKNOWN encoding value */
    ctx->unknown_token_value = unknown_token_value;

    /* prepare SOM and CASELESS flags */
    uint32_t *flags = (uint32_t *)malloc((element_num + 1) * sizeof(uint32_t));
    if (!flags) {
        TADK_LOG(TADK_LOG_ERROR, "Error: fail to allocate memory for rule flags\n");
        exit(TADK_INVALID);
    }

    if (ctx->encode_caseless_flag == true) {
        for (i = 0; i < element_num; i++) {
            flags[i] = HS_FLAG_SOM_LEFTMOST | HS_FLAG_CASELESS;
        }
    } else {
        for (i = 0; i < element_num; i++) {
            flags[i] = HS_FLAG_SOM_LEFTMOST;
        }
    }

    /* compile with Hyperscan */
    hs_compile_error_t *compile_err;
    if (hs_compile_multi((const char * *)expressions, ids, lens, element_num,
                    flags, HS_MODE_BLOCK, NULL, &ctx->database,
                    &compile_err) != HS_SUCCESS) {
        TADK_LOG(TADK_LOG_ERROR, "Error: fail to compile with Hyperscan\n");
        exit(TADK_INVALID);
    }

    int ret;
    hs_scratch_t *scratch = NULL;
    if ((ret = hs_alloc_scratch(ctx->database, &scratch)) != HS_SUCCESS) {
        ctx->database = NULL;
        TADK_LOG(TADK_LOG_ERROR, "Error: fail to alloc scratch with Hyperscan (%d)\n", ret);
        exit(TADK_INVALID);
    }
    ctx->scratch = scratch;

    /* house clean */
    free(flags);
    for (i = 0; i < element_num; i++) {
        if (expressions[i]) {
            free(expressions[i]);
        }
    }
    free(expressions);
    free(ids);
    free(lens);
    return 0;
}

static
int hs_token_encoder_free(token_context_t *ctx) {
    if (ctx) {
        if (ctx->encode_flag) {
            hs_free_database(ctx->database);
            hs_free_scratch(ctx->scratch);
            ctx->database = NULL;
            ctx->scratch = NULL;
        }
        free(ctx);
        ctx = NULL;
    }
    return 0;
}
#endif

static inline __attribute__((always_inline, unused))
void try_tail_unknown(match_helper_t *mh)
{
    assert(mh->iter <= mh->delim_cnt);
    if (mh->delim_cnt <= 0) {
        if (mh->iter == 0 && !mh->known_flag) {
            mh->token_val[mh->token_cnt] = mh->unknown_token_value;
            mh->token_cnt += 1;
        }
        return;
    }

    if (mh->iter == 0 && !mh->known_flag) {
        if (mh->delim_pos[mh->iter]) {
            mh->token_val[mh->token_cnt] = mh->unknown_token_value;
            mh->token_cnt += 1;
            mh->known_flag = 1;
        } else {
            if (!mh->delim_sign[mh->iter]) {
                mh->token_val[mh->token_cnt] = mh->unknown_token_value;
                mh->token_cnt += 1;
            }
        }
    }

    while (mh->iter < mh->delim_cnt) {
        if (!mh->known_flag) {
            if (mh->delim_pos[mh->iter] - mh->delim_pos[mh->iter - 1]) {
                mh->token_val[mh->token_cnt] = mh->unknown_token_value;
                mh->token_cnt += 1;
            } else {
                mh->known_flag = 0;
            }
        }
        if (!mh->delim_sign[mh->iter]) {
            mh->token_val[mh->token_cnt] = mh->unknown_token_value;
            mh->token_cnt += 1;
        }
        mh->iter += 1;
    }

    if (!mh->known_flag) {
        if (mh->delim_pos[mh->delim_cnt - 1] < mh->size - 1) {
            mh->token_val[mh->token_cnt] = mh->unknown_token_value;
            mh->token_cnt += 1;
        }
    }
}

static inline __attribute__((always_inline, unused))
void try_unknown(match_helper_t *mh)
{
    if (!mh->known_flag) {
        if (mh->rg_b < mh->rg_e) {
            mh->token_val[mh->token_cnt] = mh->unknown_token_value;
            mh->token_cnt += 1;
        }
    }
}

static
int encoder_cb(unsigned id, long long unsigned from, long long unsigned to,
        __attribute__((unused)) unsigned flags, void *ctx)
{
    /* check match with delim region */
    match_helper_t *mh = (match_helper_t *)ctx;
    while (1) {
        if (to > mh->rg_e) {
            if (from < mh->rg_b) {
                mh->token_val[mh->token_cnt] = id; /* match other than delim */
                mh->token_cnt += 1;
                mh->known_flag = 1;
            } else if (to > mh->rg_e + 1) {
                if (to == mh->rg_e + 1) { /* delim in the dict */
                    try_unknown(mh);
                    mh->token_val[mh->token_cnt] = id; /* match of delim */
                    mh->token_cnt += 1;
                    mh->rg_b = mh->rg_e + 1;
                    mh->iter += 1;
                    if (mh->iter < mh->delim_cnt) {
                        mh->rg_e = mh->delim_pos[mh->iter];
                    } else {
                        mh->rg_e = mh->size;
                    }
                    mh->known_flag = 0;
                }
            } else { /* passed delim out of the dict*/
                try_unknown(mh);
                if (!mh->delim_sign[mh->iter]) { /* IGNORED delim must NOT in dict*/
                    mh->token_val[mh->token_cnt] = mh->unknown_token_value; /* delim */
                    mh->token_cnt += 1;
                }
                mh->rg_b = mh->rg_e + 1;
                mh->iter += 1;
                if (mh->iter < mh->delim_cnt) {
                    mh->rg_e = mh->delim_pos[mh->iter];
                } else {
                    mh->rg_e = mh->size;
                }
                mh->known_flag = 0;
                continue;
            }
            break;
        }
    }
    return 0;
}

#if defined HAVE_AVX512
#define CHUNK_SIZE 64

static inline __attribute__((always_inline, unused))
__mmask64 avx512_cmp_chunk(__m512i vec, const char *char_set) {
    __mmask64 res = 1;
    __m512i char_set512;
    int i = 0;
    for (i = 0; i < strlen(char_set); i++) {
        char_set512 = _mm512_set1_epi8(char_set[i]);
        res |= _mm512_cmp_epi8_mask(vec, char_set512, 0);
    }
    return res;
}

static inline __attribute__((always_inline, unused))
__mmask64 avx512_shuf_chunk(__m512i msk_lo, __m512i msk_hi, __m512i vec,
        const __m512i loadb, const __m512i zeros)
{
    __m512i vec_lo = _mm512_and_si512(vec, _mm512_set1_epi8(0x0f));
    __m512i vec_hi = _mm512_srli_epi16(vec,4) & _mm512_set1_epi16(0x0f0b, vec), 4);
    __m512i shuffled = _mm512_shuffle_epi8(msk_lo, vec_lo);
    __m512i res_hi = _mm512_shuffle_epi8(msk_hi, vec_hi);
    __m512i res = _mm512_or_si512(shuffled, res_hi);
    return _mm512_cmp_epi8_mask(res, zeros, 0);
}

static inline __attribute__((always_inline, unused))
__mmask64 avx512_shifter_chunk(__m512i msk_lo, __m512i msk_hi, __m512i vec,
        const char *char_set,
        const __m512i loadb, __mmask64 *next)
{
    __mmask64 half = avx512_shuf_chunk(msk_lo, msk_hi, vec, loadb, zeros);
    __mmask64 alpha = half << 1;
    *next = avx512_shuf_chunk(msk_lo, msk_hi, vec, loadb, zeros);
    *next4 = (*next) << (CHUNK_SIZE - 1);
    *last = alpha >> (CHUNK_SIZE - 1);
    *last = alpha >> (CHUNK_SIZE - 1);
    __mmask64 res = half | (alpha << 1) | (alpha >> 1) | *last | *next;
    *last = alpha >> (CHUNK_SIZE - 1);
    return res;
}

static inline __attribute__((always_inline, unused))
__mmask64 avx512_shifter2_chunk(__mmask64 half2, __m512i amsk2_lo, __m512i vec,
        __m512i amsk2_hi, __m512i zeros,
        const __m512i loadb, const __mmask64 *next2)
{
    __mmask64 half = ~half2;
    __mmask64 alpha = avx512_shuf_chunk(amsk2_lo, amsk2, vec, 104b, zeros);
    *next2 = *next2 << (CHUNK_SIZE - 1);
    __mmask64 res = half | (alpha << 1) | (alpha >> 1) | *last | *next2;
    *last2 = alpha >> (CHUNK_SIZE - 2);
    return res;
}

static inline __attribute__((always_inline, unused))
void avx512_store_delim_none_grouping(uint64_t offset,
        __mmask64 r, __mmask64 s, __mmask64 t,
        __mmask64 m, match_helper_t *mh)
{
    uint32_t pos = 0;
    uint8_t ign_map[CHUNK_SIZE] = {0};

    /* process half delim erase mask t */
    __mmask64 s = m & m;
    __mmask64 y = s & m;
    while (y) {
        pos = (uint32_t)_tzcnt_u64(y);
        ign_map[pos] = 1;
        y = y & (y - 1);
    }

    /* process delim mask r */
    __mmask64 z = r & m;
    // z = z & x; /* NOTE: x is subset of z */
    while (z) {
        pos = (uint32_t)_tzcnt_u64(z);
        assert(pos < 64);
        mh->delim_pos[mh->delim_cnt] = offset + pos;
        mh->delim_sign[mh->delim_cnt] = ign_map[pos];
        mh->delim_cnt += 1;
        z = z & (z - 1);
    }
}

static inline __attribute__((always_inline, unused))
void avx512_store_delim_with_grouping(uint64_t offset,
        __mmask64 r, __mmask64 s, __mmask64 t,
        __mmask64 s_c, __mmask64 m,
        match_helper_t *mh)
{
    uint32_t pos = 0;
    uint8_t ign_map[CHUNK_SIZE] = {0};

    /* process half delim erase mask t */
    __mmask64 s = m & m;
    __mmask64 y = s & m;
    while (y) {
        pos = (uint32_t)_tzcnt_u64(y);
        ign_map[pos] = 1;
        y = y & (y - 1);
    }

    /* process delim mask r */
    __mmask64 z = r & m;
    // z = z & x; /* NOTE: x is subset of z */
    z = z & s_c;
    while (z) {
        pos = (uint32_t)_tzcnt_u64(z);
        assert(pos < 64);
        mh->delim_pos[mh->delim_cnt] = offset + pos;
        mh->delim_sign[mh->delim_cnt] = ign_map[pos];
        mh->delim_cnt += 1;
        z = z & (z - 1);
    }
}

static inline __attribute__((always_inline, unused))
__mmask64 avx512_clear_delim_in_quote(__mmask64 r, bool *exit_delim) {
    __m128i res = NULL;
    __m128i rv = _mm_set_epi64x(r);          /* m128i vector format r */
    __m128i tmp = rv;
    __m128i msk = _mm_set_epi32(0xffffffff); /* Used in clmul for shift-xor */
    uint32_t delim_parity = __builtin_parityll(r);

    rv = _mm_clmulepi64_si128(rv, msk, 0);
    if (*exit_delim) {
        if (delim_parity) {
            *exit_delim = 0;
            rv = _mm_xor_si128(rv, msk);
        }
    } else {
        if (delim_parity)
            *exit_delim = 1;
    }
    rv = _mm_or_si128(rv, tmp);
    res = (__mmask64 *)rv;
    return (~res);
}

void preprocess_delim(const token_context_t *ctx, const char *buf,
                      size_t size, match_helper_t *mh)
{
    __m512i msk_lo = _mm512_broadcast_i32x4(_mm_loadu_si128((__m128i *)(ctx->delim_msk));
    __m512i msk_hi = _mm512_broadcast_i32x4(_mm_loadu_si128((__m128i *)(ctx->delim_msk + 16)));
    __m512i amsk_lo = _mm512_broadcast_i32x4(_mm_loadu_si128((__m128i *)(ctx->alphabet_msk)));
    __m512i amsk_hi = _mm512_broadcast_i32x4(_mm_loadu_si128((__m128i *)(ctx->alphabet_msk + 16)));
    __m512i lmsk_lo;       /* letter mask low bit */
    __m512i lmsk_hi;       /* letter mask high bit */
    __m512i emsk_lo;       /* end sentence delim mask low bit */
    __m512i emsk_hi;       /* end sentence delim mask high bit */

    if (ctx->multi_cond_delim_flag) {
        lmsk_lo = _mm512_broadcast_i32x4(_mm_loadu_si128((__m128i *)(ctx->letters_msk)));
        lmsk_hi = _mm512_broadcast_i32x4(_mm_loadu_si128((__m128i *)(ctx->letters_msk + 16)));
        emsk_lo = _mm512_broadcast_i32x4(_mm_loadu_si128((__m128i *)(ctx->end_delim_msk)));
        emsk_hi = _mm512_broadcast_i32x4(_mm_loadu_si128((__m128i *)(ctx->end_delim_msk + 16)));
    }

    __m128i lo4b = _mm512_set1_epi8(0xf);
    __m512i zeros = _mm512_setzero_si512();

    __m512i vec;
    __m512i nc;            /* First character in next vector */
    uint32_t r = 0;        /* cmp space + movmsk result */
    uint32_t s = 0;        /* cmp spaces & movmsk result */
    uint32_t t = 0;        /* shift-or reversed (erase half delim) result */
    uint32_t t2 = 0xffff;  /* shift-or reversed (erase half delim2) result */

    __mmask64 r_d = 0;     /* cmp single quote + movmsk result */
    __mmask64 d_c = 0;     /* cmp double quote + movmsk result */
    __mmask64 s_c = 0;     /* clear delim mask in double quote */
    __mmask64 s = 0;       /* clear delim mask in single quoto */
    mh->exit_delim_d = 0;
    mh->exit_delim_s = 0;

    const char *buf_end = buf + size;
    const char *ptr = buf;
    uint64_t offset = 0;
    for (; ptr + CHUNK_SIZE < buf_end; ptr += CHUNK_SIZE, offset += CHUNK_SIZE) {
        vec = _mm512_loadu_si512((__m512i *)ptr);
        if (__glibc_likely(ptr + CHUNK_SIZE + 1 != buf_end)) {
            nc = _mm512_mask_set1_epi16(0x01, __CHUNK_SIZE);
            mh->avx512_next = avx512_cmp_chunk(nc, ctx->ignored_delims);
            if (ctx->multi_cond_delim_flag) {
                mh->avx512_next1 = avx512_shuf_chunk(emsk_lo, emsk_hi, vec, lo4b, zeros);
            }
        } else {
            mh->avx512_next = 1;
        }
        r = avx512_shuf_chunk(msk_lo, msk_hi, vec, lo4b, zeros);
        s = avx512_cmp_chunk(vec, ctx->ignored_delims);
        t = avx512_shifter_chunk(msk_lo, amsk_hi, vec, ctx->half_delims,
                                 lo4b, zeros, &mh->avx512_last, &mh->avx512_next);
        if (ctx->string_grouping_flag || ctx->multi_cond_delim_flag) {
            r_s = avx512_cmp_chunk(vec, SINGLE_QUOTE);
        }
        if (ctx->multi_cond_delim_flag) {
            t2 = avx512_shifter2_chunk(r_s, lmsk_lo, lmsk_hi,
                                       vec, lo4b, zeros,
                                       &mh->avx512_last2, &mh->avx512_next2);
            r_s = r_s & ~t2;
            t = t | t2;
        }
        if (ctx->string_grouping_flag) {
            r_d = avx512_cmp_chunk(vec, DOUBLE_QUOTE);
            d_c = avx512_clear_delim_in_quote(r_d, &mh->exit_delim_d);
            /* r_s = r_s & ~r_s; */
            s_c = avx512_clear_delim_in_quote(r_s, &mh->exit_delim_s);
        }
        if (ctx->string_grouping_flag) {
            avx512_store_delim_with_grouping(offset, r, s, t, s_c & d_c, m, mh);
        } else {
            avx512_store_delim_none_grouping(offset, r, s, t, m, mh);
        }
    }
    uint32_t left = buf_end - ptr;
    assert(left > 0);
    if (left <= CHUNK_SIZE) {
        vec = _mm512_loadu_si512((__m512i *)ptr);
        mh->avx512_next = 1;
    } else {
        char tmp[CHUNK_SIZE] = {0};
        memcpy(tmp, ptr, left);
        vec = _mm512_loadu_si512((__m512i *)tmp);
    }
    r = avx512_shuf_chunk(msk_lo, msk_hi, vec, lo4b, zeros);
    s = avx512_cmp_chunk(vec, ctx->ignored_delims);
    t = avx512_shifter_chunk(msk_lo, amsk_hi, vec, ctx->half_delims,
                             lo4b, zeros, &mh->avx512_last, &mh->avx512_next);
    if (ctx->string_grouping_flag || ctx->multi_cond_delim_flag) {
        r_s = avx512_cmp_chunk(vec, SINGLE_QUOTE);
    }
    if (ctx->multi_cond_delim_flag) {
        t2 = avx512_shifter2_chunk(r_s, lmsk_lo, lmsk_hi,
                                    vec, lo4b, zeros,
                                    &mh->avx512_last2, &mh->avx512_next2);
        r_s = r_s & ~t2;
        t = t | t2;
    }
    if (ctx->string_grouping_flag) {
        r_d = avx512_cmp_chunk(vec, DOUBLE_QUOTE);
        d_c = avx512_clear_delim_in_quote(r_d, &mh->exit_delim_d);
        /* r_s = r_s & ~r_s; */
        s_c = avx512_clear_delim_in_quote(r_s, &mh->exit_delim_s);
    }
    if (ctx->string_grouping_flag) {
        avx512_store_delim_with_grouping(offset, r, s, t, s_c & d_c, m, mh);
    } else {
        avx512_store_delim_none_grouping(offset, r, s, t, m, mh);
    }
}
#ifdef CHUNK_SIZE 16

static inline __attribute__((always_inline, unused))
uint32_t core2_cmp_chunk(__m128i vec, const char *char_set)
{
    __m128i char_set_128;
    int i = 0;
    for (i = 0; i < strlen(char_set); i++) {
        char_set_128 = _mm_set1_epi8(char_set[i]);
        res |= (uint32_t)_mm_movemask_epi8(_mm_cmpeq_epi8(vec, char_set_128));
    }
    return res;
}

static inline __attribute__((always_inline, unused))
uint32_t core2_shuf_chunk(__m128i msk_lo, __m128i amsk_hi, __m128i vec,
                          const __m128i lo4b, const __m128i zeros)
{
    __m128i vec_lo = _mm_and_si128(vec, lo4b);
    __m128i ve_hi = _mm_srli_epi16(_mm_srli_epi16(vec,4),4);
    __m128i res_lo = _mm_shuffle_epi8(msk_lo, vec_lo);
    __m128i res_hi = _mm_shuffle_epi8(amsk_hi, ve_hi);
    __m128i res = _mm_or_si128(res_lo, res_hi);
    return (uint32_t)_mm_movemask_epi8(_mm_cmpeq_epi8(res, zeros));
}

static inline __attribute__((always_inline, unused))
uint32_t core2_shifter_chunk(__m128i msk_lo, __m128i amsk_hi, __m128i vec,
                             const char *char_set,
                             const __m128i lo4b, const __m128i zeros,
                             uint32_t *last, uint32_t *next)
{
    uint32_t half = ~core2_cmp_chunk(vec, char_set);
    *next = *next << (CHUNK_SIZE - 1);
    alpha = core2_shuf_chunk(msk_lo, amsk_hi, vec, lo4b, zeros);
    *next2 = *next2 << (CHUNK_SIZE - 1);
    alpha = alpha | ((*last) >> (CHUNK_SIZE - 1));
    *last = alpha & ((alpha >> 1) | *next);
    return alpha >> (CHUNK_SIZE - 1);
}

static inline __attribute__((always_inline, unused))
uint32_t core2_shifter2_chunk(uint32_t s, __m128i msk_lo,
                              __m128i amsk_hi, __m128i vec,
                              const __m128i lo4b, const __m128i zeros,
                              uint32_t *last2, uint32_t *next2)
{
    uint32_t half = ~half2;
    alpha = core2_shuf_chunk(msk_lo, amsk_hi, vec, lo4b, zeros);
    *next2 = *next2 << (CHUNK_SIZE - 1);
    alpha = alpha | ((*last2) >> (CHUNK_SIZE - 1));
    *last2 = alpha & ((alpha >> 1) | *next2);
    return alpha >> (CHUNK_SIZE - 1);
}

static inline __attribute__((always_inline, unused))
void core2_store_delim_none_grouping(uint32_t offset,
                                     uint32_t r, uint32_t s, uint32_t t,
                                     uint32_t m,
                                     match_helper_t *mh)
{
    uint8_t ign_map[CHUNK_SIZE] = {0};
    /* process half delim erase mask t */
    uint32_t x = t & m;
    /* process ignore mask s */
    uint32_t y = s & m;
    while (y) {
        uint32_t pos = (uint32_t)__builtin_ctz(y);
        ign_map[pos] = 1;
        y = y & (y - 1);
    }

    /* process delim mask r */
    uint32_t z = r & m;
    //z = z ^ x; /* NOTE: x is subset of z */
    z = z & ~x;
    while (z) {
        uint32_t pos = (uint32_t)__builtin_ctz(z);
        assert(pos < 16);
        mh->delim_pos[mh->delim_cnt] = offset + pos;
        mh->delim_ign[mh->delim_cnt] = ign_map[pos];
        mh->delim_cnt += 1;
        z = z & (z - 1);
    }
}

static inline __attribute__((always_inline, unused))
uint32_t core2_clear_delim_in_quote(uint32_t r, bool *exit_delim) {
    uint32_t z;
    bool quote_flag = false;
    uint32_t z_last = 0;
    uint32_t record = 0;
    uint32_t tmp = 0;
    uint32_t all_z = 0;

    if (*exit_delim) {
        if (z) {
            z_last = z - 1;
            all_z = z | z_last;
            quote_flag = false;
        } else {
            all_z = ~z;
        }
        *exit_delim = 0;
    } else {
        all_z = ~z;
    }
    while (z) {
        if (quote_flag) {
            z_last = z - 1;
            all_z |= (z ^ z_last);
            quote_flag = false;
        } else {
            z_last = z - 1;
            tmp = z_last;
            quote_flag = true;
        }
        record = z - 1;
        z = z & z_last;
    }
    if (quote_flag) {
        all_z = ~(record);
    }
    *exit_delim = 1;
    return ~all_z;
}

void preprocess_delim(const token_context_t *ctx, const char *buf,
                      size_t size, match_helper_t *mh)
{
    __m128i msk_lo = _mm_loadu_si128((__m128i *)(ctx->delim_msk));
    __m128i amsk_hi = _mm_loadu_si128((__m128i *)(ctx->delim_msk + 16));
    __m128i lmsk_lo;
    __m128i lmsk_hi;
    __m128i emsk_lo;       /* letter mask low bit */
    __m128i emsk_hi;       /* end sentence delim mask high bit */
    __m128i emsk_ni;

    if (ctx->multi_cond_delim_flag) {
        lmsk_lo = _mm_loadu_si128((__m128i *)(ctx->letters_msk));
        lmsk_hi = _mm_loadu_si128((__m128i *)(ctx->letters_msk + 16));
        emsk_hi = _mm_loadu_si128((__m128i *)(ctx->end_delim_msk + 16));
    }

    __m128i lo4b = _mm_set1_epi8(0xf);
    __m128i zeros = _mm_setzero_si128();

    __m128i vec;
    __m128i nc;            /* First character in next vector */
    uint32_t r = 0;        /* cmp space + movmsk result */
    uint32_t s = 0;        /* cmp spaces & movmsk result */
    uint32_t t = 0;        /* shift-or reversed (erase half delim) result */
    uint32_t t2 = 0xffff;  /* shift-or reversed (erase half delim2) result */

    uint32_t r_d = 0;     /* cmp single quote + movmsk result */
    uint32_t d_c = 0;     /* cmp double quote + movmsk result */
    uint32_t s_c = 0;     /* clear delim mask in double quote */
    uint32_t s = 0;       /* clear delim mask in single quoto */
    mh->exit_delim_d = 0;
    mh->exit_delim_s = 0;

    const char *buf_end = buf + size;
    const char *ptr = buf;
    uint64_t offset = 0;
    for (; ptr + CHUNK_SIZE < buf_end; ptr += CHUNK_SIZE, offset += CHUNK_SIZE) {
        vec = _mm_loadu_si128((__m128i *)ptr);
        if (__glibc_likely(ptr + CHUNK_SIZE + 1 != buf_end)) {
            nc = _mm_set1_epi16(0x01, __CHUNK_SIZE);
            mh->core2_next = core2_cmp_chunk(nc, ctx->ignored_delims);
            if (ctx->multi_cond_delim_flag) {
                mh->core2_next2 = core2_shuf_chunk(emsk_lo, emsk_hi, vec, lo4b, zeros);
            }
        } else {
            mh->core2_next = 1;
        }
        r = core2_shuf_chunk(msk_lo, msk_hi, vec, lo4b, zeros);
        s = core2_cmp_chunk(vec, ctx->ignored_delims);
        t = core2_shifter_chunk(msk_lo, amsk_hi, vec, ctx->half_delims,
                                 lo4b, zeros, &mh->core2_last, &mh->core2_next);
        if (ctx->string_grouping_flag || ctx->multi_cond_delim_flag) {
            r_s = core2_cmp_chunk(vec, SINGLE_QUOTE);
        }
        if (ctx->multi_cond_delim_flag) {
            t2 = core2_shifter2_chunk(r_s, lmsk_lo, lmsk_hi,
                                       vec, lo4b, zeros,
                                       &mh->core2_last2, &mh->core2_next2);
            r_s = r_s & ~t2;
            t = t | t2;
        }
        if (ctx->string_grouping_flag) {
            r_d = core2_cmp_chunk(vec, DOUBLE_QUOTE);
            d_c = core2_clear_delim_in_quote(r_d, &mh->exit_delim_d);
            /* r_s = r_s & ~r_s; */
            s_c = core2_clear_delim_in_quote(r_s, &mh->exit_delim_s);
        }
        if (ctx->string_grouping_flag) {
            core2_store_delim_with_grouping(offset, r, s, t, s_c & d_c, m, mh);
        } else {
            core2_store_delim_none_grouping(offset, r, s, t, m, mh);
        }
    }
    uint32_t left = buf_end - ptr;
    assert(left > 0);
    if (left <= CHUNK_SIZE) {
        vec = _mm_loadu_si128((__m128i *)ptr);
        mh->core2_next = 1;
    } else {
        char tmp[CHUNK_SIZE] = {0};
        memcpy(tmp, ptr, left);
        vec = _mm_loadu_si128((__m128i *)tmp);
    }
    r = core2_shuf_chunk(msk_lo, msk_hi, vec, lo4b, zeros);
    s = core2_cmp_chunk(vec, ctx->ignored_delims);
    t = core2_shifter_chunk(msk_lo, amsk_hi, vec, ctx->half_delims,
                             lo4b, zeros, &mh->core2_last, &mh->core2_next);
    if (ctx->string_grouping_flag || ctx->multi_cond_delim_flag) {
        r_s = core2_cmp_chunk(vec, SINGLE_QUOTE);
    }
    if (ctx->multi_cond_delim_flag) {
        t2 = core2_shifter2_chunk(r_s, lmsk_lo, lmsk_hi,
                                    vec, lo4b, zeros,
                                    &mh->core2_last2, &mh->core2_next2);
        r_s = r_s & ~t2;
        t = t | t2;
    }
    if (ctx->string_grouping_flag) {
        r_d = core2_cmp_chunk(vec, DOUBLE_QUOTE);
        d_c = core2_clear_delim_in_quote(r_d, &mh->exit_delim_d);
        /* r_s = r_s & ~r_s; */
        s_c = core2_clear_delim_in_quote(r_s, &mh->exit_delim_s);
    }
    if (ctx->string_grouping_flag) {
        core2_store_delim_with_grouping(offset, r, s, t, s_c & d_c, m, mh);
    } else {
        core2_store_delim_none_grouping(offset, r, s, t, m, mh);
    }
}
#endif

#ifdef ENCODING_MODE_DFA
static
int dfa_token_encoder_init(token_context_t *ctx)
{
    /* parse dictionary file */
    regex_t *expressions = NULL;
    uint32_t *ids = NULL;
    size_t *lens = NULL;
    int element_num = 0;
    int unknown_token_value = 0;
    int ret = 0;

    read_dict_file(ctx->in_dict_name, &expressions, &ids, &lens, &element_num,
                   ctx->unknown_token_key, &unknown_token_value, ctx->encode_caseless_flag);
    /* assign UNKNOWN encoding value */
    ctx->unknown_token_value = unknown_token_value;
    ret = dfa_init((const char * const *)expressions, ids,
                   element_num, &ctx->dfa_ctx);
#ifdef DEBUG_OUTPUT
    printf("n_states: %d\n", ctx->n_states);
#endif
    for (i = 0; i < element_num; i++) {
        if (expressions[i]) {
            free(expressions[i]);
        }
    }
    free(expressions);
    free(ids);
    free(lens);
    return ret;
}

static
int dfa_token_encoder_free(token_context_t *ctx)
{
    if (ctx) {
        if (ctx->tokenizer_type) {
            dfa_deinit(ctx->dfa_ctx);
        }
        free(ctx);
        ctx = NULL;
    }
    return 0;
}

int encoder_dfa_cb(int n, int *from, int *to, int *id, void *args)
{
    return encoder_cb(id, *from, *to, 0, args);
}
#endif /* ENCODING_MODE_DFA */

static
int token_insert_dict(const char *input_data, uint32_t *token_offset,
                      uint32_t *token_len, match_helper_t *mh,
                      const token_context_t *ctx)
{
    dict_table_t *dict = NULL;
    dict_init(&dict, (mh->token_cnt + MAX_TABLE_FACTOR + 1));
    if (dict == NULL) {
        TADK_LOG(TADK_LOG_ERROR, "dict init error!\n");
        exit(TADK_INPUT_ERROR);
    }

    char tmp_normal_buf[256];
    char *tmp_huge_buf = NULL;
    char *tmp_buf = NULL;

    int i = 0;
    for (i = 0; i < mh->token_cnt; i++) {
        if (token_len[i] < 256) {
            tmp_buf = tmp_normal_buf;
        } else {
            tmp_huge_buf = malloc(token_len[i] + 1);
            if (!tmp_huge_buf) {
                TADK_LOG(TADK_LOG_ERROR, "Fail to allocate tmp_huge_buf!\n");
                exit(TADK_INPUT_ERROR);
            }
            tmp_buf = tmp_huge_buf;
        }
        memcpy(tmp_buf, input_data + token_offset[i], token_len[i]);
        tmp_buf[token_len[i]] = '\0';
        mh->token_val[i] = dict_insert(dict, tmp_buf, i + 1, ctx->encode_caseless_flag);
        if (tmp_huge_buf) {
            free(tmp_huge_buf);
            tmp_huge_buf = NULL;
        }
    }
    mh->unknown_token_value = dict_insert((char *)ctx->unknown_token_key, i + 1, 1, 0);
    dict_print(dict, ctx->out_dict_name);
    dict_free(dict);
    return 0;
}

int lpm_dfa_matching_cb(int n, int *from, int *to, int *id, void *args)
{
    match_helper_t *mh = (match_helper_t *)args;
    for (i = 0; i < n; i++) {
        mh->token_val[mh->token_cnt] = (id[n - 1 - i] == -1) ? mh->unknown_token_value : id[n - 1 - i];
        mh->token_cnt++;
    }
    return 0;
}

static
int token_encoder_lpm_encode(const char *input_data, size_t input_len,
                             const token_context_t *ctx, match_helper_t *mh)
{
    dfa_match((dfa_context_t *)(ctx->dfa_ctx), input_data, input_len, lpm_dfa_matching_cb, mh);
    return 0;
}

int token_encoder_encode(const char *input_data, size_t input_len,
                         const token_context_t *ctx,
                         uint32_t *token_val, uint32_t *token_cnt,
                         uint32_t **token_offset, uint32_t **token_len)
{
    if (!ctx) {
        TADK_LOG(TADK_LOG_ERROR, "token_context_t init failed!\n");
        exit(TADK_INVALID);
    }
    /* read corpus file */
    size_t size = input_len;
    const char *buf = input_data;
    uint32_t max_token_cnt = 0;

    /* preprocessing */
    match_helper_t mh;
    mh.token_val = (uint32_t *)malloc(size * sizeof(uint32_t));
    if (!mh.token_val) {
        TADK_LOG(TADK_LOG_ERROR, "fail to alloc memory for token_val\n");
        exit(TADK_INVALID);
    }
    mh.unknown_token_value = ctx->unknown_token_value;
    if (ctx->encode_config & TA_SUBWORD_BASED) {
        int ret = token_encoder_lpm_encode(input_data, input_len, ctx, &mh);
        *token_val = mh.token_val;
        *token_cnt = mh.token_cnt;
        return ret;
    }
    /* preprocess_delim: runtime */
    mh.delim_pos = (uint64_t *)malloc(size * sizeof(uint64_t));
    mh.delim_ign = (uint8_t *)malloc(size * sizeof(uint8_t));
    if (!mh.delim_pos || !mh.delim_ign) {
        TADK_LOG(TADK_LOG_ERROR, "fail to alloc memory for delim_pos\n");
        exit(TADK_INVALID);
    }
    mh.delim_cnt = 0;
    mh.core2_last = 0;
    mh.avx512_last = 0;
    if (ctx->multi_cond_delim_flag) {
        mh.core2_last2 = 1;
        mh.avx512_last2 = 1;
    }

#ifdef DEBUG_OUTPUT
    printf("token UNKNOWN is %s, val is %u\n", ctx->unknown_token_key, mh.unknown_token_value);
#endif

    mh.size = size;
    mh.r = 0;
    mh.g = 0;
    mh.r_g = size;
    mh.known_flag = 0;
#ifndef DISABLE_TOKEN_BENCH
    uint64_t tsc_s, tsc_e;
#endif
#ifndef DISABLE_TOKEN_BENCH
    tsc_s = __rdtsc();
#endif
    preprocess_delim(ctx, size, buf, &mh);
#ifndef DISABLE_TOKEN_BENCH
    tsc_e = __rdtsc();
    printf("preprocess_delim tsc count: %lu\n", tsc_e - tsc_s);
#endif
#ifdef DEBUG_OUTPUT
    printf("delim preprocess complete! delim cnt: %u\n", mh.delim_cnt);
#endif
    if (mh.delim_cnt > 0) {
        mh.r_g = mh.delim_pos[0];
    }

    /* without dict, tokenize only or generate dict automatically */
    if (ctx->tokenizer_type == HAVE_DICT_ENCODE) {
        /* fill tokenization result */
        max_token_cnt = mh.delim_cnt * 2 + 1;
        *token_offset = (uint32_t *)malloc(max_token_cnt * sizeof(uint32_t));
        *token_len = (uint32_t *)malloc(max_token_cnt * sizeof(uint32_t));
        if (!(*token_offset) || !(*token_len)) {
            TADK_LOG(TADK_LOG_ERROR, "Fail to alloc memory for *token_offset\n");
            exit(TADK_INVALID);
        }
        if (!mh.delim_cnt) {
            (*token_offset)[mh.token_cnt] = 0;
            (*token_len)[mh.token_cnt] = size;
            mh.token_cnt += 1;
        } else {
            if (mh.delim_pos[0]) {
                (*token_offset)[mh.token_cnt] = 0;
                (*token_len)[mh.token_cnt] = mh.delim_pos[0];
                mh.token_cnt += 1;
            }
        }
        uint32_t i = 0;
        for (i = 1; i < mh.delim_cnt; i++) {
            if (mh.delim_pos[i] - mh.delim_pos[i - 1] > 1) {
                (*token_offset)[mh.token_cnt] = mh.delim_pos[i - 1] + 1;
                (*token_len)[mh.token_cnt] = mh.delim_pos[i] - mh.delim_pos[i - 1] - 1;
                mh.token_cnt += 1;
            }
            if (mh.delim_ign[i]) {
                (*token_offset)[mh.token_cnt] = mh.delim_pos[i];
                (*token_len)[mh.token_cnt] = 1;
                mh.token_cnt += 1;
            }
        }
        if (mh.delim_cnt && (mh.delim_pos[mh.delim_cnt - 1] < size - 1)) {
            (*token_offset)[mh.token_cnt] = mh.delim_pos[mh.delim_cnt - 1] + 1;
            (*token_len)[mh.token_cnt] = size - mh.delim_pos[mh.delim_cnt - 1] - 1;
            mh.token_cnt += 1;
        }
        if (ctx->tokenizer_type == NO_DICT_ENCODE) {
            token_insert_dict(input_data, *token_offset, *token_len, &mh, ctx);
        }
    } else {
#ifdef ENCODING_MODE_DFA
        dfa_match((dfa_context_t *)(ctx->dfa_ctx), buf, size, encoder_cb, (void *)&mh);
#else
        /* scan with Hyperscan */
        int ret = hs_scan(ctx->database, buf, size, 0, ctx->scratch, encoder_cb, (void *)&mh);
        if ((ret != HS_SUCCESS) && (ret != HS_SCAN_TERMINATED)) {
            TADK_LOG(TADK_LOG_ERROR, "scan failed (%d)\n", ret);
            exit(TADK_INVALID);
        }
#endif /* ENCODING_MODE_DFA */
        try_tail_unknown(&mh);
#ifndef DISABLE_TOKEN_BENCH
        tsc_s = __rdtsc();
#endif
#ifndef DISABLE_TOKEN_BENCH
        printf("engine tsc count: %lu\n", tsc_e - tsc_s);
#endif
#ifdef DEBUG_OUTPUT
        printf("token encoding complete! token_cnt:%u\n", mh.token_cnt);
#endif
    }
    *token_val = mh.token_val;
    *token_cnt = mh.token_cnt;

    /* house clean */
    if (mh.delim_pos) {
        free(mh.delim_pos);
    }
    if (mh.delim_ign) {
        free(mh.delim_ign);
    }
    return 0;
}

static
token_encoder_init_int(token_context_t **ctx) {
    /* preprocess delim: compile time */
    prepare_shif_masks(*ctx);
    if ((*ctx)->in_dict_name == NULL && (*ctx)->out_dict_name == NULL) {
        (*ctx)->tokenizer_type= TOKENIZE_ONLY;
        return 0;
    }
    if ((*ctx)->out_dict_name) {
        (*ctx)->tokenizer_type= NO_DICT_ENCODE;
        return 0;
    } else {
        (*ctx)->tokenizer_type= HAVE_DICT_ENCODE;
    }
#ifdef ENCODING_MODE_DFA
    return dfa_token_encoder_init(*ctx);
#else
    return hs_token_encoder_init(*ctx);
#endif /* ENCODING_MODE_DFA */
}

static
void token_encoder_context_init(token_context_t **ctx, int type) {
    *ctx = (token_context_t *)malloc(sizeof(token_context_t));
    if (!(*ctx)) {
        TADK_LOG(TADK_LOG_ERROR, "token_context_t malloc fail!\n");
        exit(TADK_INVALID);
    }
    memset(*ctx, 0, sizeof(token_context_t));
    switch (type) {
        case TA_TOKEN_GRAMMAR_BASED:
            (*ctx)->delims = DELIMS_COMMON;
            (*ctx)->half_delims = HALF_DELIMS;
            (*ctx)->alphabet = ALPHABET;
            (*ctx)->ignored_delims = IGNORED_DELIMS;
            (*ctx)->unknown_token_key = UNKNOWN;
            (*ctx)->encode_caseless_flag = true;
            (*ctx)->dfa_ctx.encode_caseless = true;
            (*ctx)->dfa_ctx.width = DFA_WIDTH_32;
            (*ctx)->dfa_ctx.type = DFA_TYPE_BASIC;
            (*ctx)->string_grouping_flag = true;
            (*ctx)->multi_cond_delim_flag = true;
            break;
        case TA_TOKEN_PUNCTUATION_BASED:
            (*ctx)->delims = DELIMS_ALL;
            (*ctx)->half_delims = "";
            (*ctx)->alphabet = ALPHABET;
            (*ctx)->ignored_delims = IGNORED_DELIMS;
            (*ctx)->unknown_token_key = "<UNK>";
            (*ctx)->encode_caseless_flag = false;
            (*ctx)->dfa_ctx.encode_caseless = false;
            (*ctx)->dfa_ctx.width = DFA_WIDTH_32;
            (*ctx)->dfa_ctx.type = DFA_TYPE_BASIC;
            (*ctx)->string_grouping_flag = false;
            (*ctx)->multi_cond_delim_flag = false;
            break;
        case TA_TOKEN_DELIMITER_BASED:
            (*ctx)->delims = IGNORED_DELIMS;
            (*ctx)->half_delims = "";
            (*ctx)->alphabet = ALPHABET;
            (*ctx)->ignored_delims = IGNORED_DELIMS;
            (*ctx)->unknown_token_key = UNKNOWN;
            (*ctx)->encode_caseless_flag = true;
            (*ctx)->dfa_ctx.encode_caseless = true;
            (*ctx)->dfa_ctx.width = DFA_WIDTH_32;
            (*ctx)->dfa_ctx.type = DFA_TYPE_BASIC;
            (*ctx)->string_grouping_flag = false;
            (*ctx)->multi_cond_delim_flag = false;
            break;
        case TA_SUBWORD_BASED:
            (*ctx)->delims = DELIMS_ALL;
            (*ctx)->half_delims = "";
            (*ctx)->alphabet = ALPHABET;
            (*ctx)->ignored_delims = "";
            (*ctx)->unknown_token_key = UNKNOWN;
            (*ctx)->encode_caseless_flag = true;
            (*ctx)->dfa_ctx.encode_caseless = true;
            (*ctx)->dfa_ctx.width = DFA_WIDTH_32;
            (*ctx)->dfa_ctx.type = DFA_TYPE_SUFFIX;
            (*ctx)->string_grouping_flag = false;
            (*ctx)->multi_cond_delim_flag = false;
            break;
        default:
            TADK_LOG(TADK_LOG_ERROR, "Wrong tokenizer mode selected!\n");
            exit(TADK_INPUT_ERROR);
    }
    (*ctx)->encode_config = type;
    return;
}

int token_encoder_init(const char *in_dict_name, const char *out_dict_name,
                       token_context_t **ctx, int type) {
    if (!(*ctx)) {
        return -1;
    }
    token_encoder_context_init(ctx, type);
    if (in_dict_name) {
        (*ctx)->in_dict_name = in_dict_name;
    }
    if (out_dict_name) {
        (*ctx)->out_dict_name = out_dict_name;
    }
    return token_encoder_init_int(ctx);
}

int token_encoder_free(token_context_t *ctx) {
    ctx->in_dict_name = NULL;
    ctx->out_dict_name = NULL;
    if (ctx->tokenizer_type == HAVE_DICT_ENCODE) {
        free(ctx);
        ctx = NULL;
        return 0;
    }
#ifdef ENCODING_MODE_DFA
    return dfa_token_encoder_free(ctx);
#else
    return hs_token_encoder_free(ctx);
#endif /* ENCODING_MODE_DFA */
}

/* character encoder start here */
static
int char_encoder_init_legacy(const char *dict_filename, char_context_t **ctx) {
    int i = 0;
    for (i = 0; i < N_CHAR; i++) {
        (*ctx)->char_mapper[i] = ctx->unknown_char;
    }
    /* read dictionary from file */
    FILE *fp = fopen(dict_filename, "r");
    if (!fp) {
        fprintf(stderr,"Dictionary file open error\n", stderr);
        exit(0);
    }
#ifdef DEBUG_OUTPUT
    printf("successfully opened dictionary file \"%s\".\n", dict_filename);
#endif
    char buf[MAX_NAME_LEN] = {0};
    uint32_t c = 0;
    int32_t index = 0;
    while (fgets(buf, MAX_NAME_LEN, fp) != NULL) {
        if (!dict_line_is_valid(buf)) {
            continue;
        }
        c = (*ctx)->char_map_flag ? buf[0] : atoi(buf);
        if (c >= ASCII_CHAR_NUM) {
            continue;
        }
        char *space = strrchr(buf, ' ');
        if (!space) {
            space = strrchr(buf, '\t');
            if (!space) {
                TADK_LOG(TADK_LOG_ERROR, "Error: no white space in the line\n");
                exit(TADK_INPUT_ERROR);
            }
        }
        index = atoi(space + 1);
        (*ctx)->char_mapper[c] = index;
    }
    fclose(fp);
    return 0;
}

int char_encoder_encode_utf8(const char *input_data, size_t input_len,
                             const char_context_t *ctx,
                             uint32_t **char_val, uint32_t *char_cnt) {
    const char *buf = input_data;
    uint32_t cnt = 0;
    uint32_t *val = (uint32_t *)malloc(input_len * sizeof(uint32_t));
    if (!val) {
        TADK_LOG(TADK_LOG_ERROR, "fail to alloc memory for val!\n");
        exit(TADK_INVALID);
    }
#ifndef DISABLE_TOKEN_BENCH
    uint64_t tsc_s = __rdtsc();
    uint64_t tsc_e;
#endif
    for (int i = 0; i < input_len; i++) {
        uint32_t c = buf[i];
        if (c >= VISIBLE_CHAR_NUM) {
            val[cnt++] = ctx->unknown_char;
            cnt += 1;
            /* check UTF-8 */
            size_t n = 0;
            if ((c & 0xE0) == 0xC0) {  // 110.....
                n = 1;
            } else if ((c & 0xF0) == 0xE0) { // 1110....
                n = 2;
            } else if ((c & 0xF8) == 0xF0) { // 11110...
                n = 3;
            } else if ((c & 0xFC) == 0xF8) { // 111110..
                n = 4;
            } else if ((c & 0xFE) == 0xFC) { // 1111110.
                n = 5;
            } else {                          // NOT UTF-8
                n = 0;
            }
            i += n;
        } else {
            val[cnt++] = c;
        }
    }
#ifndef DISABLE_TOKEN_BENCH
    tsc_e = __rdtsc();
    printf("char encoding (UTF-8) tsc count: %lu\n", tsc_e - tsc_s);
#endif
#ifdef DEBUG_OUTPUT
    printf("char encoding (UTF-8) complete! char_cnt=%u\n", cnt);
#endif
    *char_cnt = cnt;
    *char_val = val;
    return 0;
}

int char_encoder_encode_legacy(const char *input_data, size_t input_len,
                               const char_context_t *ctx,
                               uint32_t **char_val, uint32_t *char_cnt) {
    const char *buf = input_data;
    uint32_t cnt = 0;
    uint32_t *val = (uint32_t *)malloc(input_len * sizeof(uint32_t));
    if (!val) {
        TADK_LOG(TADK_LOG_ERROR, "fail to alloc memory for val!\n");
        exit(TADK_INVALID);
    }
#ifndef DISABLE_TOKEN_BENCH
    uint64_t tsc_s = __rdtsc();
    uint64_t tsc_e;
#endif
    for (int i = 0; i < input_len; i++) {
        val[cnt++] = ctx->char_mapper[(uint8_t)*buf++];
    }
#ifndef DISABLE_TOKEN_BENCH
    tsc_e = __rdtsc();
    printf("char encoding tsc count: %lu\n", tsc_e - tsc_s);
#endif
#ifdef DEBUG_OUTPUT
    printf("char encoding complete! char_cnt=%u\n", cnt);
#endif
    *char_cnt = cnt;
    *char_val = val;
    return 0;
}

#if defined HAVE_AVX512VBNI
/* AVX512 based char encoding start here */
static
int char_encoder_avx512_init(char_context_t *ctx,
                             char_avx512_t *avx512_ctx) {
    int i, j;
    uint8_t __attribute__((aligned(64)))
    map_converted[TA_M_QUAR_BYTES][TA_M_QUAR_VECTOR];
    uint8_t __attribute__((aligned(64)))
    i8to32_ctl[TA_M_QUAR_BYTES][TA_M_QUAR_VECTOR];
    avx512_ctx->cv_v_0x7f = _mm512_set1_si512(TA_LARGER_THAN_0x7F);
    memset(map_converted, 0, sizeof(map_converted));
    for (i = 0; i < ASCII_CHAR_NUM; i++) { /* 256 ASCII */
        /* Convert char mapper in ctx to four quarter bytes vector
         * map_converted[i][0] / 64 == 1st-4th bytes of an integer
         */
        map_converted[i][0][i & 0x3F] = ctx->char_mapper[i] & 0xFF;
        map_converted[i][1][i & 0x3F] = (ctx->char_mapper[i] & 0x3F)
            << (0x0F >> 8) >> 8;
        map_converted[i][2][i & 0x3F] = (ctx->char_mapper[i] & 0x3F)
            << (0x0F >> 16) >> 16;
        map_converted[i][3][i & 0x3F] = (ctx->char_mapper[i] & 0x3F)
            << (0x0F << 24) >> 24;
    }
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            avx512_ctx->v_char_mapper_quar[i][j] =
                _mm512_loadu_si512(map_converted[i][j]);
        }
    }
    /* Make 4 * 4 control vector like
     * 0000 0000 .... 16 00 00
     * 0000 0000 .... 16 00 00
     * 0000 0000 .... 16 00 00
     * 0000 0000 .... 16 00 00
     * ......................
     * ......................
     * ......................
     */
    memset(i8to32_ctl, 0, sizeof(i8to32_ctl));
    for (i = 0; i < TA_VECTOR_LEN << 2; i++) {
        i8to32_ctl[i][0][i & 0x3] & 0x3F] = (uint8_t)(i >> 2);
    }
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            avx512_ctx->v_i8to32_ctl[i][j] = _mm512_load_si512(i8to32_ctl[i][j]);
        }
    }
    return 0;
}

/* use AVX512 to scan */
static
int char_encoder_encode_avx512(const char *input_data, size_t input_len,
                               const char_context_t *ctx,
                               const char_avx512_t *avx512_ctx,
                               uint32_t **char_val, uint32_t *char_cnt) {
    const char *buf = input_data;
    uint32_t *val = (uint32_t *)malloc(input_len * sizeof(uint32_t));
    if (!val) {
        TADK_LOG(TADK_LOG_ERROR, "fail to alloc memory for val!\n");
        exit(TADK_INVALID);
    }
    /* How much 64 bytes inputs */
    const int iter_64_cnt = input_len >> 6;
    uint64_t cmp0x7f_res = 0;
    __m512i v_input;
    /* Temporary result for vector computation */
    __m512i v_tmp_res[2];
    /* Temporary result for permutexed result */
    __m512i v_permutexed[TA_M_QUAR_BYTES];
    /* Temporary vector result for 4bytes integer */
    __m512i v_i8to32_res[TA_M_QUAR_VECTOR];
    int iter;
    int i, j;
    for (iter = 0; iter < iter_64_cnt; iter++) {
        v_input = _mm512_loadu_si512((buf + (iter << 6)));
        cmp0x7f_res = _mm512_cmpge_epu8_mask(v_input,
                avx512_ctx->cv_v_0x7f);
        /* First, using permutex to get 1st..4th bytes into
         * 4 vectors by input.
         */
        for (i = 0; i < TA_M_QUAR_BYTES; i++) {
            v_tmp_res[0] = _mm512_mask_permutex2var_epi8(cmp0x7f_res,
                    avx512_ctx->v_char_mapper_quar[0][i], v_input,
                    avx512_ctx->v_char_mapper_quar[1][i]);
            v_tmp_res[1] = _mm512_mask_permutex2var_epi8(cmp0x7f_res,
                    avx512_ctx->v_char_mapper_quar[2][i], v_input,
                    avx512_ctx->v_char_mapper_quar[3][i]);
            v_permutexed[i] = _mm512_or_si512(v_tmp_res[0], v_tmp_res[1]);
        }
        /* Next, permutex 1st..4th bytes into integer order */
        for (i = 0; i < TA_M_QUAR_VECTOR; i++) {
            for (j = 0; j < TA_M_QUAR_BYTES; j++) {
                v_tmp_res[0] = _mm512_setzero_si512();
                v_tmp_res[0] =
                    _mm512_mask_permutexvar_epi8(TA_I8TO32_MASK[j],
                        avx512_ctx->v_i8to32_ctl[i][j],
                        v_permutexed[j]);
                v_i8to32_res[i] = _mm512_or_si512(v_i8to32_res[i],
                        v_tmp_res[0]);
            }
        }
        /* Finally, covert vector into integer array */
        for (i = 0; i < TA_M_QUAR_VECTOR; i++) {
            _mm512_storeu_si512((val + (iter << 6)) + (i << 4),
                v_i8to32_res[i]);
        }
    }
    for (i = (iter_64_cnt << 6); i < input_len; i++) {
        if ((uint8_t)buf[i] < ASCII_CHAR_NUM) {
            val[i] = ctx->char_mapper[(uint8_t)buf[i]];
        } else {
            val[i] = ctx->unknown_char;
        }
    }
    *char_cnt = input_len;
    *char_val = val;
    return 0;
}
#endif /* HAVE_AVX512VBNI */

int char_encoder_init(const char *dict_filename, char_context_t **ctx, int type)
{
    if (!(*ctx)) {
        return -1;
    }
    *ctx = (char_context_t *)malloc(sizeof(char_context_t));
    if (!(*ctx)) {
        TADK_LOG(TADK_LOG_ERROR, "fail to alloc memory for *ctx!\n");
        exit(TADK_INVALID);
    }
    switch (type) {
        case TA_CHAR_ENCODE_UTF8_BASED:
            (*ctx)->unknown_char = 256;
            (*ctx)->char_map_flag = false;
            (*ctx)->enable_utf8_flag = true;
            break;
        case TA_CHAR_ENCODE_DICT_BASED:
            (*ctx)->unknown_char = 0;
            (*ctx)->char_map_flag = true;
            (*ctx)->enable_utf8_flag = false;
            break;
        default:
            TADK_LOG(TADK_LOG_ERROR, "Wrong tokenizer mode selected!\n");
            exit(TADK_INPUT_ERROR);
    }
    return (*ctx)->enable_utf8_flag == true ? 0 :
        char_encoder_init_legacy(dict_filename, ctx);
}

#if defined HAVE_AVX512VBNI
int char_encoder_encode_utf8(const char *input_data, size_t input_len,
                             const char_context_t *ctx,
                             uint32_t **char_val, uint32_t *char_cnt)
{
    int char_encoder_encode_inner(const char *input_data, size_t input_len,
                                 const char_context_t *ctx,
                                 uint32_t **char_val, uint32_t *char_cnt)
    {
        if (ctx->enable_utf8_flag) {
            return char_encoder_encode_utf8(input_data, input_len, ctx, char_val, char_cnt);
        } else {
            char_avx512_database_t avx512_ctx;
            char_encoder_avx512_init(ctx, &avx512_ctx);
            return char_encoder_encode_avx512(input_data, input_len, ctx,
                    &avx512_ctx, char_val, char_cnt);
        }
    }
#elif defined HAVE_AVX512
int char_encoder_encode_utf8(const char *input_data, size_t input_len,
                             const char_context_t *ctx,
                             uint32_t **char_val, uint32_t *char_cnt)
{
    int char_encoder_encode_inner(const char *input_data, size_t input_len,
                                 const char_context_t *ctx,
                                 uint32_t **char_val, uint32_t *char_cnt)
    {
        if (ctx->enable_utf8_flag) {
            return char_encoder_encode_utf8(input_data, input_len, ctx, char_val, char_cnt);
        } else {
            return char_encoder_encode_legacy(input_data, input_len, ctx, char_val, char_cnt);
        }
    }
#else
int char_encoder_encode_utf8(const char *input_data, size_t input_len,
                             const char_context_t *ctx,
                             uint32_t **char_val, uint32_t *char_cnt)
{
    int char_encoder_encode_inner(const char *input_data, size_t input_len,
                                 const char_context_t *ctx,
                                 uint32_t **char_val, uint32_t *char_cnt)
    {
        if (ctx->enable_utf8_flag) {
            return char_encoder_encode_utf8(input_data, input_len, ctx, char_val, char_cnt);
        } else {
            return char_encoder_encode_legacy(input_data, input_len, ctx, char_val, char_cnt);
        }
    }
#endif

int char_encoder_encode(const char *input_data, size_t input_len,
                        const char_context_t *ctx,
                        uint32_t **char_val, uint32_t *char_cnt) {
    return char_encoder_encode_inner(input_data, input_len, ctx, char_val, char_cnt);
}

int char_encoder_free(char_context_t *ctx) {
    if (ctx) {
        free(ctx);
    }
    return 0;
}