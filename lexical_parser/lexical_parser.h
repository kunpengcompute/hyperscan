#ifndef LEXICAL_PARSER_H
#define LEXICAL_PARSER_H

/**
 * @file
 * @brief The Lexical Parser Library API definition.
 *
 * Lexical Parser is a high speed language (SQL/HTML5) parsing library.
 *
 * This header contains functions for tokenizing and computing feature
 * vector that can be used by AI/ML model.
 */

#if defined(__SSSE3__)
#define HAVE_SSSE3
#endif

#if defined(__AVX512BW__)
#define HAVE_AVX512
#endif

#if defined(__AVX512VBMI__)
#define HAVE_AVX512VBMI
#define HAVE_AVX512VPOPCNTDQ
#endif

#include <stdio.h>
#include <stdint.h>
#include "lexical_parser_common.h"

#define START_STATE_ARRAY_LEN    (3)
#define ACCEPT_BIT_16            (15)
#define STATUS_FLAG_16           (1 << ACCEPT_BIT_16)
#define STATUS_BITMASK_16        (~STATUS_FLAG_16)
#define ACCEPT_BIT_32            (31)
#define STATUS_FLAG_32           (1 << ACCEPT_BIT_32)
#define STATUS_BITMASK_32        (~STATUS_FLAG_32)

typedef int (*token_output_cb_t)(void *args, int from, int id);

/**
 * Lexical Parser library 16-bit DFA engine runtime.
 *
 * Encode an input string to a sequence of tokens according to the given
 * transition tables.
 *
 * @param trans_table
 *     State transition table of ordinary characters (0..255).
 *
 * @param output_table
 *     Table storing output tokens of accept states.
 *
 * @param eod_table
 *     Pointer to state transition table at end of data.
 *     The DFA will use '\0' instead of EOD in case of NULL.
 *
 * @param len_table
 *     Table storing lengths of output tokens of accept states.
 *
 * @param init_state
 *     Initial state ID.
 *
 * @param input
 *     The input string to be encoded.
 *
 * @param tokens
 *     Output token array returned. User should allocate the memory and pass
 *     in a valid pointer here.
 *
 * @param tokens_len
 *     Number of output tokens returned.
 *
 * @param len
 *     Length of input string.
 *
 * @param cb
 *     User defined callback function called at accept states
 *
 * @param args
 *     User defined callback args.
 *
 * @return
 *     VOID.
 */
void tokenize_dfa_engine16(int16_t trans_table[][N_CHAR],
                           int16_t output_table[][MAX_OUTPUT_TOKEN],
                           int16_t *eod_table,
                           int16_t len_table[][MAX_OUTPUT_TOKEN],
                           int16_t init_state,
                           const char *input,
                           int16_t *tokens,
                           int *tokens_len,
                           int len,
                           token_output_cb_t cb,
                           void *args);

/**
 * Lexical Parser library 32-bit DFA engine runtime.
 *
 * Encode an input string to a sequence of tokens according to the given
 * transition tables.
 *
 * @param trans_table
 *     State transition table of ordinary characters (0..255).
 *
 * @param output_table
 *     Table storing output tokens of accept states.
 *
 * @param eod_table
 *     Pointer to state transition table at end of data.
 *     The DFA will use '\0' instead of EOD in case of NULL.
 *
 * @param len_table
 *     Table storing lengths of output tokens of accept states.
 *
 * @param init_state
 *     Initial state ID.
 *
 * @param input
 *     The input string to be encoded.
 *
 * @param tokens
 *     Output token array returned. User should allocate the memory and pass
 *     in a valid pointer here.
 *
 * @param tokens_len
 *     Number of output tokens returned.
 *
 * @param len
 *     Length of input string.
 *
 * @param cb
 *     User defined callback function called at accept states
 *
 * @param args
 *     User defined callback args.
 *
 * @return
 *     VOID.
 */
void tokenize_dfa_engine32(int32_t trans_table[][N_CHAR],
                           int32_t output_table[][MAX_OUTPUT_TOKEN],
                           int32_t *eod_table,
                           int32_t len_table[][MAX_OUTPUT_TOKEN],
                           int32_t init_state,
                           const char *input,
                           int32_t *tokens,
                           int *tokens_len,
                           int len,
                           token_output_cb_t cb,
                           void *args);

/**
 * Lexical Parser library token feature vector calculator.
 *
 * Compute token histogram by sliding window algorithm according to the given
 * sequence of tokens.
 *
 * @param feature_vec
 *     Output feature vector returned.
 *     User should allocate the memory and pass in a valid pointer here.
 *
 * @param tokens
 *     Input sequence of tokens.
 *
 * @param tokens_len
 *     The number of input tokens.
 *
 * @return
 *     0 on success, other values on failure.
 */
int compute_token_feature(int *feature_vec, const int16_t *tokens,
                          const int tokens_len);

/**
 * Lexical Parser library DFA start state calculator.
 *
 * Compute DFA start state according to the quotes' status.
 *
 * @param input_str
 *     Input string to be tokenized.
 *
 * @param len
 *     The length of input string.
 *
 * @return
 *     The ID of DFA start state.
 */
int16_t compute_start_state(char *input_str, int len);

/**
 * Lexical Parser library DFA start states calculator.
 *
 * Compute DFA start states according to the quotes' status.
 * It will be used in anomaly_detector to improve the accuracy.
 *
 * @param input_str
 *     Input string to be tokenized.
 *
 * @return
 *     The number of valid elements in start_state_array.
 */
uint32_t compute_start_state_array(char *input_str);

/**
 * Lexical Parser library URI decoder.
 *
 * Perform HEX decoding for a string.
 *
 * @param src_str
 *     Input string to be decoded.
 *
 * @param src_len
 *     The length of input string.
 *
 * @param dec_str
 *     Output buffer storing the decoded string.
 *
 * @param dec_len
 *     The length of decoded string.
 *
 * @return
 *     TADK_SUCCESS on success, TADK_DECODE_ERROR on failure.
 */
int uri_decode(const char *src_str, int src_len, char *dec_str, int *dec_len);

#endif /* LEXICAL_PARSER_H */