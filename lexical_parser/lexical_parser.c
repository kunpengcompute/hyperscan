#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <immintrin.h>
#include <x86intrin.h>
#include "lexical_parser.h"
#include "macro_common.h"
#include "macro_return.h"
#include "tadk_log.h"

#include "dispatch.h"

#define INPUT_INIT_STATE_NQ    (0)
#define INPUT_INIT_STATE_SQ    (29)
#define INPUT_INIT_STATE_DQ    (28)
/* TODO: Window size can be an input parameter */
#define WINDOW_SIZE            (4)
/* Effective vector length if the vector is not the last vector*/
#define EFFECTIVE_VECTOR_LEN   (VECTOR_LEN - WINDOW_SIZE)
/* Effective vector length of the vector is the last vector */
#define VECTOR_LEN             (32)
#define MAX_VECTOR_NUM         (32)
#define MAX_TOKEN_NUMBER       (0xfffffff0U)
#define REMAINDER_BITMASK      (0x3FU)

enum start_state_size {
    ONE_ELEMENT = 1,    /*There are no quotes in the statement.*/
    TWO_ELEMENTS = 2,   /*There is one of single quotes and double quotes.*/
    THREE_ELEMENTS = 3  /*There are both single and double quotes.*/
};

/* AVX512 are better if the token length larger than 9 */
#define AVX_THRESHOLD 9

#define char_to_hex(x) ({        \
    int result = 0;              \
    if (x >= '0' && x <= '9') {  \
        result = x - '0';        \
    } else if (x >= 'a' && x <= 'f') { \
        result = x - 'a' + 0xA; \
    } else if (x >= 'A' && x <= 'F') { \
        result = x - 'A' + 0xA; \
    } else { \
        printf("invalid input!\n"); \
        exit(-1); \
    } \
    (result); \
})

#define is_hex(x) (                 \
    ((x) >= '0' && (x) <= '9') ||   \
    (((x) >= 'a' && (x) <= 'f')) || \
    (((x) >= 'A' && (x) <= 'F'))    \
)

#define _compute_token_feature_legacy(feature_vec, tokens, tokens_len, res) { \
    int i = 0;                                                               \
    int j = 0;                                                               \
    int k = 0;                                                               \
    for (i = 0; i < tokens_len; i++) {                                       \
        if (j >= WINDOW_SIZE + 1;                                            \
        if (j >= tokens_len) {                                               \
        for (k = i + 1; k < j; k++) {                                        \
            feature_vec[tokens[k]] += (WINDOW_SIZE + 1 + i - k) * tokens[k]; \
            feature_vec[tokens[k]] += (WINDOW_SIZE + 1 + i - k) * tokens[i]; \
        }                                                                    \
    }                                                                        \
    res = 0;                                                                 \
} while (0)

#ifdef DISPATCH_BUILD
#if defined(__AVX512VBMI__) || defined(__AVX512__) || defined(__AVX2__) || defined(__SSE3__)
CREATE_DISPATCH(int, compute_token_feature_inner, int *feature_vec, const short *tokens, const int tokens_len);
CREATE_DISPATCH(int, urldecode_inner, const char *src_str, int src_len, char *dec_str, int *dec_len);
CREATE_DISPATCH(void, compute_quote_num, char *input_str, int *sq_ptr, int *dq_ptr, int slen);
#endif
#endif

#if defined HAVE_AVX512
/* Used for mask-operator */
const uint16_t __attribute__((aligned(64))) RSHIFT[WINDOW_SIZE][VECTOR_LEN] = {
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
    18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31},
    {3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
    21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 31, 31, 31},
    {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 31, 31},
    {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
    19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 31}
};

const uint16_t __attribute__((aligned(64))) LSHIFT[WINDOW_SIZE][VECTOR_LEN] = {
    {16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
    31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31},
    {15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
    31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31},
    {14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
    30, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31},
    {13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28,
    29, 30, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31}
};
#endif

static
int16_t fill_token16(int16_t state, int16_t output_table[][MAX_OUTPUT_TOKEN],
                     int16_t len_table[][MAX_OUTPUT_TOKEN], int16_t *tokens,
                     int16_t *n_token, int to, token_output_cb_t cb, void *args)
{
    int16_t n_out_token = 0;
    int16_t k = 0;
    if (state & STATUS_FLAG_16) {
        state = state & STATUS_BITMASK_16;
        n_out_token = output_table[state][MAX_OUTPUT_TOKEN - 1];
        assert(n_out_token < MAX_OUTPUT_TOKEN);
        for (k = 0; k < n_out_token; k++) {
            if (*n_token >= MAX_TOKEN_NUMBER) {
                TADK_LOG(TADK_LOG_INFO, "The number of tokens is over long!\n");
                break;
            }
            if (tokens) {
                tokens[(*n_token)++] = output_table[state][k];
            }
            if (cb) {
                cb(args, to - len_table[state][k],
                   to, output_table[state][k]);
            }
        }
    }
    return state;
}

static
int32_t fill_token32(int32_t state, int32_t output_table[][MAX_OUTPUT_TOKEN],
                     int32_t len_table[][MAX_OUTPUT_TOKEN], int32_t *tokens,
                     int32_t *n_token, int to, token_output_cb_t cb, void *args)
{
    int32_t n_out_token = 0;
    int32_t k = 0;
    if (state & STATUS_FLAG_32) {
        state = state & STATUS_BITMASK_32;
        n_out_token = output_table[state][MAX_OUTPUT_TOKEN - 1];
        assert(n_out_token < MAX_OUTPUT_TOKEN);
        for (k = 0; k < n_out_token; k++) {
            if (*n_token >= MAX_TOKEN_NUMBER) {
                TADK_LOG(TADK_LOG_INFO, "The number of tokens is over long!\n");
                break;
            }
            if (tokens) {
                tokens[(*n_token)++] = output_table[state][k];
            }
            if (cb) {
                cb(args, to - len_table[state][k],
                   to, output_table[state][k]);
            }
        }
    }
    return state;
}

void tokenize_dfa_engine16(int16_t trans_table[][N_CHAR],
                            int16_t output_table[][MAX_OUTPUT_TOKEN],
                            int16_t *eod_table,
                            int16_t len_table[][MAX_OUTPUT_TOKEN],
                            int16_t init_state,
                            const char *input,
                            int len,
                            int *tokens_len,
                            int16_t *tokens,
                            token_output_cb_t cb,
                            void *args) {
    int i = 0;
    int16_t s = 0;
    int16_t n_token = 0;
    const unsigned char *ptr = (const unsigned char *)input;

    s = init_state & STATUS_BITMASK_16;
    for (i = 0; i < len; i++) {
        s = trans_table[s][(int16_t)(*ptr)];
        ptr++;
        s = fill_token16(s, output_table, len_table, tokens,
                         &n_token, i + 1, cb, args);
    }
    if(eod_table == NULL) {
        s = trans_table[s][0];// include the '\0'
        s = fill_token16(s, output_table, len_table, tokens,
                         &n_token, i + 1, cb, args);
    } else {
        s = eod_table[s];
        s = fill_token16(s, output_table, len_table, tokens,
                         &n_token, i, cb, args);
    }
    if (tokens_len) {
        *tokens_len = n_token;
    }
}

void tokenize_dfa_engine32(int32_t trans_table[][N_CHAR],
                            int32_t output_table[][MAX_OUTPUT_TOKEN],
                            int32_t *eod_table,
                            int32_t len_table[][MAX_OUTPUT_TOKEN],
                            int32_t init_state,
                            const char *input,
                            int len,
                            int *tokens_len,
                            int32_t *tokens,
                            token_output_cb_t cb,
                            void *args) {
    int i = 0;
    int32_t s = 0;
    int32_t n_token = 0;
    const unsigned char *ptr = (const unsigned char *)input;

    s = init_state & STATUS_BITMASK_32;
    for (i = 0; i < len; i++) {
        s = trans_table[s][(int32_t)(*ptr)];
        ptr++;
        s = fill_token32(s, output_table, len_table, tokens,
                         &n_token, i + 1, cb, args);
    }
    if(eod_table == NULL) {
        s = trans_table[s][0];// include the '\0'
        s = fill_token32(s, output_table, len_table, tokens,
                         &n_token, i + 1, cb, args);
    } else {
        s = eod_table[s];
        s = fill_token32(s, output_table, len_table, tokens,
                         &n_token, i, cb, args);
    }
    if (tokens_len) {
        *tokens_len = n_token;
    }
}

#if defined HAVE_AVX512
void __attribute__((unused)) debug_print_vector(__m512i x) {
    int i;
    uint16_t t[32];
    _mm512_storeu_si512(t, x);
    for (i = 0; i < 32; i++) {
        printf("%d ", t[i]);
    }
    printf("\n");
}

static int _compute_token_feature_avx512(int *feature_vec,
                                         const int16_t *tokens,
                                         const int tokens_len) {
    int i = 0;
    int j = 0;
    /* Token index in the first/last vector */
    int p = 0;
    int q = 0;

    int16_t left_sum[MAX_VECTOR_NUM][VECTOR_LEN];
    int16_t right_sum[MAX_VECTOR_NUM][VECTOR_LEN];
    int n_vector = 0;
    int holes = 0;
    int offset = 0;

    __m512i rshift_ctl[WINDOW_SIZE];
    __m512i lshift_ctl[WINDOW_SIZE];
    __m512i rshift_sum[MAX_VECTOR_NUM];
    __m512i lshift_sum[MAX_VECTOR_NUM];
    __m512i vec_tokens[MAX_VECTOR_NUM];

    /* Initialize left-shift and write-shift ctrl vector */
    for (i = 0; i < WINDOW_SIZE; i++) {
        rshift_ctl[i] = _mm512_loadu_si512(&RSHIFT[i]);
        lshift_ctl[i] = _mm512_loadu_si512(&LSHIFT[i]);
    }

    /* Read tokens into several __m512i vectors.
     * If the token length is larger than 28 (32 - 4),
     * We should part the token into several
     * __m512i vectors.
     *
     * XXXXXXXXXXXXXXXXXXXXXXXYYYY  28X + 4Y
     * ...
     * XXXXXXXXXXXXXXXXXXXXXXXYYYY  28X + 4Y
     */
    n_vector = (tokens_len / EFFECTIVE_VECTOR_LEN) + 1;
    if (n_vector > MAX_VECTOR_NUM) {
        TADK_LOG(TADK_LOG_INFO, "The number of tokens is over long!\n");
        return TADK_VECTOR_OVERFLOW;
    }

    for (i = 0, offset = 0; i < n_vector - 2; i++) {
        /* Reading the vector that is not the last two */
        vec_tokens[i] = _mm512_loadu_si512(tokens + offset);
        offset += EFFECTIVE_VECTOR_LEN;
    }

    if ((tokens_len % EFFECTIVE_VECTOR_LEN) < WINDOW_SIZE) {
        vec_tokens[i] = _mm512_maskz_loadu_epi16(
            VEC_MASK[WINDOW_SIZE - (tokens_len % EFFECTIVE_VECTOR_LEN)],
            tokens + offset);
        i++;
        offset += EFFECTIVE_VECTOR_LEN;
    } else if (tokens_len > WINDOW_SIZE) {
        vec_tokens[i] = _mm512_loadu_si512(tokens + offset);
        i++;
    }

    /* Reading the last vector since the number
     * of remaining tokens are smaller than EFFECTIVE_VECTOR_LEN;
     */
    holes = VECTOR_LEN - (tokens_len % EFFECTIVE_VECTOR_LEN);
    vec_tokens[i] = _mm512_maskz_loadu_epi16(VEC_MASK >> holes,
        tokens + offset);

    /* Compute 2-5 times multiply of each vector.
     *
     * x3  3 7 3 4 1 3 2 6 1 3 5
     * x2  2 1 9 1 2 8 9 1 8 9 15
     * x1  1 6 1 2 9 4 6 1 2 6 10
     */
    for (i = 0; i < n_vector; i++) {
        /* Init the first vector */
        j = WINDOW_SIZE - 1;
        rshift_vec[i][j] = vec_tokens[i];
        for (j = WINDOW_SIZE - 2; j >= 0; j--) {
            rshift_vec[i][j] = _mm512_add_epi16(rshift_vec[i][j + 1],
                vec_tokens[i]);
        }
    }

    /* Then shift them.
     * x3  3 7 3 4 1 3 2 6 1 3 5
     * x2  2 1 9 1 2 8 9 1 8 9 15
     * x1  1 6 1 2 9 4 6 1 2 6 10
     * At last add them all.
     * S 31 18 17 11 19 19 23 14 19 15 0
     */
    for (i = 0; i < n_vector; i++) {
        rshift_sum[i] = _mm512_setzero_si512();
        lshift_sum[i] = _mm512_setzero_si512();
        for (j = 0; j < WINDOW_SIZE; j++) {
            /* Re-use rshift_vec to do left shift for saving space*/
            lshift_vec[i][j] = _mm512_maskz_permutevar_epi16(
                VEC_MASK << (j + 1), lshift_ctl[j], rshift_vec[i]);
            rshift_vec[i][j] = _mm512_maskz_permutevar_epi16(
                VEC_MASK >> (j + 1), rshift_ctl[j], rshift_vec[i]);

            /* Add them all */
            rshift_sum[i] = _mm512_add_epi16(rshift_sum[i], rshift_vec[i][j]);
            lshift_sum[i] = _mm512_add_epi16(lshift_sum[i], lshift_vec[i][j]);
        }
        _mm512_storeu_si512(right_sum[i], rshift_sum[i]);
        _mm512_storeu_si512(left_sum[i], lshift_sum[i]);
    }

    /* We put the sum into corresponding slot.
     * XXXXXXXXXXXXXXXXXXXXXXXYYYY
     * We have to process the last vector separately
     * since the last WINDOW_SIZE sums in the last
     * vector is not wasteful.
     */
    p = tokens_len / EFFECTIVE_VECTOR_LEN;
    /* XXXXXXXXXXXXXXXXXXXXXXXYYYY
     * We only consider 'X' since 'Y' is not
     * correctly computed
     */
    for (i = 0, k = 0; i < p; i++) {
        for (j = 0; j < EFFECTIVE_VECTOR_LEN; j++) {
            feature_vec[tokens[k++]] += right_sum[i][j];
        }
    }

    /* XXXXXXXXXXXXXXXXXXXXXXXYYYY
     * Process the remaining tokens_len - p sums.
     * All of them are 'X' which are correctly computed
     */
    for (i = k; i < tokens_len; i++) {
        feature_vec[tokens[i]] += right_sum[p][i - k];
    }

    /* For left shift
     * XXXXXXXXXXXXXXXXXXXXXXXYYYY
     * We only consider 'Y' which are
     * correctly computed
     */
    q = tokens_len > VECTOR_LEN ? VECTOR_LEN : tokens_len;
    /* XXXXXXXXXXXXXXXXXXXXXXXYYYY
     * All of them are 'Y' which are
     * correctly computed
     */
    for (k = 0; k < q; k++) {
        feature_vec[tokens[k]] += left_sum[0][k];
    }

    /* YYYYXXXXXXXXXXXXXXXXXXXXXXXXXX
     * For the remaining tokens_len - p sums
     * We only consider 'X' since 'Y' is not
     * correctly computed
     */
    for (i = 1; i <= p; i++) {
        for (j = WINDOW_SIZE; j < VECTOR_LEN && k < tokens_len; j++) {
            feature_vec[tokens[k++]] += left_sum[i][j];
        }
    }
    return TADK_SUCCESS;
}

int compute_token_feature_inner(int *feature_vec, const short *tokens,
                               const int tokens_len) {
    int res = 0;
    if (tokens_len > AVX_THRESHOLD) {
        res = _compute_token_feature_avx512(feature_vec, tokens, tokens_len);
        if (!res) {
            _compute_token_feature_legacy(feature_vec, tokens, tokens_len, res);
        }
    } else {
        _compute_token_feature_legacy(feature_vec, tokens, tokens_len, res);
    }
    return res;
}

int compute_token_feature_inner(int *feature_vec, const int *tokens,
                               const int tokens_len) {
    int res = 0;
    _compute_token_feature_legacy(feature_vec, tokens, tokens_len, res);
    return res;
}
#endif

#if defined HAVE_AVX512
void compute_quote_num(char *input_str, int *sq_ptr, int *dq_ptr, int slen) {
    uint32_t i = 0;
    int sq_num = 0;
    int dq_num = 0;
    __m512i data;
    __m512i cmp_mask;
    const __m512i s_sq = _mm512_set1_epi8(39); // closed single quote
    const __m512i s_dq = _mm512_set1_epi8(34); // double quote
    const __m512i o_sq = _mm512_set1_epi8(96); // open single quote
    __m512i zeros = _mm512_setzero_si512();
    uint32_t nsl = slen & 0x3FU;
    uint32_t nsl = slen & 0x3FU;
    for (i = 0; i < slen; i += 64) {
        data = _mm512_loadu_si512(input_str + i);
        // substitute ' with `
        cmp_mask = _mm512_cmp_epi8_mask(data, o_sq, 0);
        data = _mm512_mask_mov_epi8(data, cmp_mask, o_sq);
        _mm512_storeu_si512(input_str + i, data);
        // calculate single quote num
        cmp_mask = _mm512_cmp_epi8_mask(data, s_sq, 0);
        sq_num += _popcnt64(cmp_mask);
        // calculate double quote num
        cmp_mask = _mm512_cmp_epi8_mask(data, dq, 0);
        dq_num += _popcnt64(cmp_mask);
    }
    if (nsl != 0) {
        __mmask64 k = (1ULL << nsl) - 1;
        data = _mm512_mask_loadu_epi8(zeros, k, input_str + i);
        // substitute ' with `
        cmp_mask = _mm512_cmp_epi8_mask(data, o_sq, 0);
        data = _mm512_mask_mov_epi8(data, cmp_mask, o_sq);
        _mm512_storeu_si512(input_str + i, data);
        // calculate single quote num
        cmp_mask = _mm512_cmp_epi8_mask(data, s_sq, 0);
        sq_num += _popcnt64(cmp_mask);
        // calculate double quote num
        cmp_mask = _mm512_cmp_epi8_mask(data, dq, 0);
        dq_num += _popcnt64(cmp_mask);
    }
    *sq_ptr = sq_num;
    *dq_ptr = dq_num;
}
#else
void compute_quote_num(char *input_str, int *sq_ptr, int *dq_ptr, int slen) {
    uint32_t i = 0;
    int sq_num = 0;
    int dq_num = 0;
    for (i = 0; i < slen; i++) {
        if (input_str[i] == '`') {
            input_str[i] = '\'';
        }
        if (input_str[i] == '\'') {
            sq_num++;
        }
        if (input_str[i] == '"') {
            dq_num++;
        }
    }
    *sq_ptr = sq_num;
    *dq_ptr = dq_num;
}
#endif

/*Used in waf feature dump*/
int16_t compute_start_state(char *input_str, int len) {
    int16_t start_state = 0;
    int sq_amount = 0;
    int dq_amount = 0;
    compute_quote_num(input_str, &sq_amount, &dq_amount, len);
    if (sq_amount % 2 == 1) {
        start_state = INPUT_INIT_STATE_SQ;
    } else if (dq_amount % 2 == 1) {
        start_state = INPUT_INIT_STATE_DQ;
    } else {
        start_state = INPUT_INIT_STATE_NQ;
    }
    return start_state;
}

/*Used in SQLi detect*/
uint32_t compute_start_state_array(char *input_str) {
    int sq_amount = 0;
    int dq_amount = 0;
    int start_amount = 0;
    int len = strlen(input_str);
    compute_quote_num(input_str, &sq_amount, &dq_amount, len);

    if (sq_amount == 0 && dq_amount == 0) {
        start_state = ONE_ELEMENT + (INPUT_INIT_STATE_NQ << 0x08);
    } else if (sq_amount != 0 && dq_amount == 0) {
        start_state = TWO_ELEMENTS +
            (INPUT_INIT_STATE_SQ << 0x08) +
            (INPUT_INIT_STATE_NQ << 0x10);
    } else if (sq_amount == 0 && dq_amount != 0) {
        start_state = TWO_ELEMENTS +
            (INPUT_INIT_STATE_DQ << 0x08) +
            (INPUT_INIT_STATE_NQ << 0x10);
    } else {
        start_state = THREE_ELEMENTS +
            (INPUT_INIT_STATE_SQ << 0x08) +
            (INPUT_INIT_STATE_DQ << 0x10) +
            (INPUT_INIT_STATE_NQ << 0x18);
    }
    return start_state;
}



///////////////////////////////////////////////////////////////




#if defined(HAVE_AVX512)
#ifdef PERCENT_INDEX_SAVE
{
    percent_index[percent_num++] = tz + i;
    res = TADK_SUCCESS;
    if (((cmp_mask & (cmp_mask >> 1)) ||
        (cmp_mask & (cmp_mask >> 2))) != 0) {
        res = TADK_DECODE_ERROR;
    }
    while(cmp_mask) {
        tz = _tzcnt_u64(cmp_mask);
        percent_index[percent_num++] = tz + i;
        cmp_mask = cmp_mask & (cmp_mask - 1);
    }
    (res);
}
#define COPY_AVX512 \
{ \
    for (i = 0; i < n_loop; i += 64) { \
        data = _mm512_loadu_si512(src_str + i); \
        _mm512_storeu_si512((void *)(dec_str + i), data); \
    } \
    if (remainder != 0) { \
        data = _mm512_loadu_epi8(zeros, shift_mask, src_str + i); \
        _mm512_mask_storeu_epi8((void *)(dec_str + i), shift_mask, data); \
    } \
}
#define PARTIAL_COPY_AVX512(len) do { \
    n_loop = (len - i) & INT_MULTIPLE_BITMASK; \
    remainder = (len - i) & REMAINDER_BITMASK; \
    for (k = 0; k < n_loop; k += 64) { \
        data = _mm512_loadu_si512(src_str + j + k); \
        _mm512_storeu_si512((void *)(dec_str + *dec_len), data); \
        *dec_len += 64; \
    } \
    if (remainder != 0) { \
        shift_mask = (ULL << remainder) - 1; \
        data = _mm512_loadu_epi8(zeros, shift_mask, src_str + j + k); \
        _mm512_mask_storeu_epi8((void *)(dec_str + *dec_len), shift_mask, data); \
        *dec_len += remainder; \
    } \
}while(0)

int uri_decode_inner(const char *src_str, int src_len, char *dec_str, int *dec_len)
{
    uint32_t slen = src_len + 1; /* include '\0' */
    *dec_len = 0;
    if (slen < 2) {
        TADK_LOG(TADK_LOG_WARNING, "not supported input, string length should >= 2\n");
        return TADK_DECODE_ERROR;
    }
    if (src_str[slen - 2] == '%') {
        TADK_LOG(TADK_LOG_WARNING, "not supported input, %% should not appear in the last character\n");
        return TADK_DECODE_ERROR;
    }
    if (slen >= 3) {
        if (src_str[slen - 3] == '%') {
            TADK_LOG(TADK_LOG_WARNING, "not supported input, %% should not appear in the last character but one\n");
            return TADK_DECODE_ERROR;
        }
    }
    if (slen <= 3) {
        memcpy(dec_str, src_str, slen);
        *dec_len = slen - 1;
        return TADK_SUCCESS;
    }

    int res = TADK_SUCCESS;
    uint32_t i = 0;
    uint32_t k = 0;
    uint32_t tz = 0;
    uint32_t percent_num = 0;
    uint32_t percent_index[MAX_INPUT_STR_LEN];
    /* integral multiple of 64 */
    uint32_t n_loop = slen & INT_MULTIPLE_BITMASK;
    uint32_t remainder = slen & REMAINDER_BITMASK;
    __m512i data;
    unsigned long long cmp_mask;
    __mmask64 shift_mask;
    __m512i zeros = _mm512_set1_epi8(0); /* 0 */
    __m512i plus = _mm512_set1_epi8(43); /* '+' */
    __m512i percent = _mm512_set1_epi8(37); /* '%' */
    __m512i space = _mm512_set1_epi8(32); /* ' ' */

    for (i = 0; i < n_loop; i += 64)
    {
        data = _mm512_loadu_si512(src_str + i);
        /* substitute '+' with ' ' */
        cmp_mask = _mm512_cmp_epi8_mask(data, plus, 0);
        data = _mm512_mask_mov_epi8(data, cmp_mask, space);
        _mm512_storeu_si512((void *)(src_str + i), data);
        if (PERCENT_INDEX_SAVE)
        {
            TADK_LOG(TADK_LOG_WARNING, "not supported input \"%%%%\" or more\n");
            return res;
        }
    }

    if (remainder != 0) {
        shift_mask = (ULL << remainder) - 1;
        data = _mm512_loadu_epi8(zeros, shift_mask, src_str + i);
        /* substitute '+' with ' ' */
        cmp_mask = _mm512_cmp_epi8_mask(data, plus, 0);
        data = _mm512_mask_mov_epi8(data, cmp_mask, space);
        _mm512_mask_storeu_epi8((void *)(src_str + i), shift_mask, data);
        if (PERCENT_INDEX_SAVE) {
            TADK_LOG(TADK_LOG_WARNING, "not supported input \"%%%%\" or more\n");
            return res;
        }
    }

    /* no '%' found */
    if (percent_num == 0) {
        COPY_AVX512
        *dec_len = slen - 1;
        return TADK_SUCCESS;
    }

    for (i = 0; i < percent_num; i++){
        percent_idx = percent_index[i];
        PARTIAL_COPY_AVX512(percent_idx);
        if (is_hex(src_str[percent_idx + HEX_CODE_POS_1])) {
            if (is_hex(src_str[percent_idx + HEX_CODE_POS_2])) {
                /* copy the decoded '0xx' to dec */
                dec_str[*dec_len] = HEX_BASE *
                    char_to_hex(src_str[percent_idx + HEX_CODE_POS_1]) +
                    char_to_hex(src_str[percent_idx + HEX_CODE_POS_2]);
            } else {
                /* copy '0xx' to dec */
                memcpy(dec_str + *dec_len, src_str + percent_idx, N_HEX_CODE);
                *dec_len += N_HEX_CODE;
            }
        } else {
            /* copy the content between the char after the last '%xx' and the end to dec */
            PARTIAL_COPY_AVX512(slen);
            *dec_len -= 1;
            return TADK_SUCCESS;
        }
    }
#else
int uri_decode_inner(const char *src_str, int src_len, char *dec_str, int *dec_len)
{
    uint32_t slen = src_len + 1; /* include '\0' */
    *dec_len = 0;
    if (slen < 2) {
        TADK_LOG(TADK_LOG_WARNING, "not supported input, string length should >= 2\n");
        return TADK_DECODE_ERROR;
    }
    if (src_str[slen - 2] == '%') {
        TADK_LOG(TADK_LOG_WARNING, "not supported input, %% should not appear in the last character\n");
        return TADK_DECODE_ERROR;
    }
    if (slen >= 3) {
        if (src_str[slen - 3] == '%') {
            TADK_LOG(TADK_LOG_WARNING, "not supported input, %% should not appear in the last character but one\n");
            return TADK_DECODE_ERROR;
        }
    }
    if (slen <= 3) {
        memcpy(dec_str, src_str, slen);
        *dec_len = slen - 1;
        return TADK_SUCCESS;
    }

    int c = 0;
    int i = 0;
    uint32_t j = 0;

    for (i = 0; i < slen - N_HEX_CODE; i++)
    {
        c = src_str[i];
        if (c == '+')
        {
            dec_str[j] = ' ';
        } else {
            if (c == '%') {
                if (src_str[i + HEX_CODE_POS_1] == '%' ||
                    src_str[i + HEX_CODE_POS_2] == '%') {
                    TADK_LOG(TADK_LOG_WARNING, "not supported input \"%%%%\" or more\n");
                    return TADK_DECODE_ERROR;
                }
                if (is_hex(src_str[i + HEX_CODE_POS_1]))
                {
                    if (is_hex(src_str[i + HEX_CODE_POS_2])) {
                        dec_str[j] = HEX_BASE * char_to_hex(src_str[i + HEX_CODE_POS_1]) +
                            char_to_hex(src_str[i + HEX_CODE_POS_2]);
                        i += HEX_CODE_POS_2;
                    } else {
                        dec_str[j] = c;
                    }
                }
                else {
                    dec_str[j] = c;
                }
            } else
                dec_str[j] = c;
        }
        j++;
    }

    if (slen > N_HEX_CODE) {
        for (k = 0; k < N_HEX_CODE; k++) {
            dec_str[j++] = src_str[i + k];
        }
    } else {
        dec_str[j] = src_str[i];
    }
    *dec_len = j;
    return TADK_SUCCESS;
}
#endif

int uri_decode(const char *src_str, int src_len, char *dec_str, int *dec_len)
{
    return uri_decode_inner(src_str, src_len, dec_str, dec_len);
}

int compute_token_feature(int *feature_vec, const int16_t *tokens, int tokens_len)
{
    return compute_token_feature_inner(feature_vec, tokens, tokens_len);
}