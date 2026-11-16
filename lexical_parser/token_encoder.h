#ifndef TOKEN_ENCODER_H
#define TOKEN_ENCODER_H
/**
 * @file
 * @brief The Token Encoder Library API definition.
 *
 * Token Encoder is a generic high speed tokenization library.
 * This header contains functions for tokenizing, word encoding and char
 * encoding. The encoding value of text or network packet can be used in
 * anomaly detection by AI/DL model.
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

#include "dict.h"

typedef struct token_context_t token_context_t;
typedef struct char_context_t char_context_t;

/**
 * @brief
 * An enum describing the available tokenizer and encoder types.
 * One of the types should be selected by the initialization calls (@ref
 * token_encoder_init()) and @ref char_encoder_init()).
 */
typedef enum encode_config_e {
    /**
     * Readable text tokenizer, identifying abbreviations, content within
     * quotation marks, etc.
     */
    TA_TOKEN_GRAMMAR_BASED,
    /**
     * Machine-generated text tokenizer (e.g. payload), Take all punctuation
     * as tokenizer's basis.
     */
    TA_TOKEN_PUNCTUATION_BASED,
    /**
     * Use transparent delimiters (tab, space, newline) as tokenizer's basis.
     */
    TA_TOKEN_DELIMITER_BASED,
    /**
     * UTF-8 char encode.
     */
    TA_CHAR_ENCODE_UTF8_BASED,
    /**
     * Dict based char encode.
     */
    TA_CHAR_ENCODE_DICT_BASED,
    /**
     * Sub-word encoder.
     */
    TA_SUBWORD_BASED,
    /**
     * Invalid tokenizer type.
     */
    TA_TOKEN_TYPE_INVALID
} encode_config_e;

/**
 * @brief
 * An enum describing the available word encoding modes.
 */
typedef enum encode_mode_e {
    /**
     * Do tokenizing and encoding according to a given dictionary.
     */
    HAVE_DICT_ENCODE,
    /**
     * Do tokenizing and encoding without dictionary, and generate a
     * dictionary during encoding process automatically.
     */
    NO_DICT_ENCODE,
    /**
     * Do tokenizing only, no dictionary is given.
     */
    TOKENIZE_ONLY
} encode_mode_e;

/**
 * @brief
 * Token Encoder library word encoder initializer.
 * Initialize a tokenizer with dictionary and delimiter configurations.
 *
 * @param in_dict_name
 * Pathname of input dictionary.
 *
 * @param out_dict_name
 * Pathname of output dictionary.
 *
 * @param ctx
 * On success, a pointer to a newly allocated token_context_t will be
 * returned here. This context can then be used for tokenizing or encoding,
 * and eventually freed by the caller using @ref token_encoder_free().
 *
 * @param type
 * Type of tokenizer. There're 3 types can be selected: "GRAMMAR",
 * "PUNCTUATION" and "DELIMITER".
 *
 * @return
 * 0 on success, other values on failure.
 */
int token_encoder_init(const char *in_dict_name, const char *out_dict_name,
                       token_context_t **ctx, int type);

/**
 * @brief
 * Token Encoder library word encoder destroyer.
 * Destroy a tokenizer previously initialized by @ref token_encoder_init().
 *
 * @param ctx
 * The tokenizer context to be freed which is previously allocated by
 * @ref token_encoder_init().
 *
 * @return
 * 0 on success, other values on failure.
 */
int token_encoder_free(token_context_t *ctx);

/**
 * @brief
 * Token Encoder library word encoder tokenizer.
 * Separate an input string into token fragments, or a sequence of encoding
 * values. The encoding can use a given dictionary, or generate a new dictionary
 * automatically.
 *
 * @param input_data
 * The input string to be encoded.
 *
 * @param input_len
 * Length of input string.
 *
 * @param ctx
 * The tokenizer context which is previously allocated by
 * @ref token_encoder_init().
 *
 * @param token_val
 * Output array of token encoding values.
 *
 * @param token_cnt
 * Number of output tokens returned.
 *
 * @param token_offset
 * Output array of token starting offsets in input buffer.
 *
 * @param token_len
 * Output array of token word lengths.
 *
 * @return
 * 0 on success, other values on failure.
 */
int token_encoder_encode(const char *input_data, size_t input_len,
                         const token_context_t *ctx,
                         uint32_t *token_val, uint32_t *token_cnt,
                         uint32_t *token_offset, uint32_t *token_len);

/**
 * @brief
 * Token Encoder library char encoder initializer.
 * Initialize a tokenizer with dictionary and delimiter configurations.
 *
 * @param dict_filename
 * Pathname of input dictionary.
 *
 * @param ctx
 * On success, a pointer to a newly allocated char_context_t will be
 * returned here. This context can then be used for char encoding,
 * and eventually freed by the caller using @ref char_encoder_free().
 *
 * @param type
 * Type of char encoder. There're 2 types can be selected: "UTF8" and
 * "DICT".
 *
 * @return
 * 0 on success, other values on failure.
 */
int char_encoder_init(const char *dict_filename, char_context_t **ctx, int type);

/**
 * @brief
 * Token Encoder library char encoder destroyer.
 * Destroy a char encoder previously initialized by @ref char_encoder_init().
 *
 * @param ctx
 * The char encoder context to be freed which is previously allocated by
 * @ref char_encoder_init().
 *
 * @return
 * 0 on success, other values on failure.
 */
int char_encoder_free(char_context_t *ctx);

#endif