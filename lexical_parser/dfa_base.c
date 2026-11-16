#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <x86intrin.h>

#include "macro_common.h"
#include "macro_return.h"
#include "lexical_parser.h"
#include "trie.h"
#include "tadk_log.h"
#include "dfa_base.h"

typedef struct bfs_cb_data {
    void *trans_table;
    void *output_table;
    void *len_table;
    dfa_context_t *ctx;
    int32_t next;
    int32_t cur;
} bfs_data_t;

static inline
void gen_dfa_entry(trie_t *t, void *args) {
    int32_t index = 1;    /* 0 for entry */
    trie_node_t *tt = t->child;
    struct bfs_cb_data *data = (struct bfs_cb_data *)args;
    bool encode_caseless_flag = data->ctx->encode_caseless_flag;
    data->next = 0;
    while (tt) {
        if (data->ctx->width == DFA_WIDTH_16) {
            if (encode_caseless_flag == true) {
                TRANS_TABLE_BUILD16_CASELESS(0, (int16_t)tt->c, index);
            } else {
                TRANS_TABLE_BUILD16_CASENESS(0, (int16_t)tt->c, index);
            }
        } else {
            if (encode_caseless_flag == true) {
                TRANS_TABLE_BUILD32_CASELESS(0, (int16_t)tt->c, index);
            } else {
                TRANS_TABLE_BUILD32_CASENESS(0, (int16_t)tt->c, index);
            }
        }
        tt->state_id = index;
        index++;
        tt = tt->sibling;
    }
    data->next = index;
    data->cur = 1;    /* entry state finished, start from index 1 */
}

static
void bfs_start_cb(trie_node_t *iter, void *args) {
    struct bfs_cb_data *data = (struct bfs_cb_data *)args;
    int32_t i = iter->state_id;

    /* fill output and length table here
     * first level
     */
    if (iter->type) {
        if (data->ctx->width == DFA_WIDTH_16) {
            OUTPUT_TABLE_BUILD16(i, 0, iter->type);
            /* length table token length */
            OUTTBL16(data->len_table[i][0]) = strlen(iter->word);
        } else {
            OUTPUT_TABLE_BUILD32(i, 0, iter->type);
            /* length table token length */
            OUTTBL32(data->len_table[i][0]) = strlen(iter->word);
        }
    }
}

static
void bfs_main_cb(trie_node_t *iter, void *args) {
    struct bfs_cb_data *data = (struct bfs_cb_data *)args;
    bool encode_caseless_flag = data->ctx->encode_caseless_flag;
    /* fill trans table */
    if (data->ctx->width == DFA_WIDTH_16) {
        if (encode_caseless_flag == true) {
            TRANS_TABLE_BUILD16_CASELESS(data->cur, (int16_t)iter->c, data->next);
        } else {
            TRANS_TABLE_BUILD16_CASENESS(data->cur, (int16_t)iter->c, data->next);
        }
    } else {
        if (encode_caseless_flag == true) {
            TRANS_TABLE_BUILD32_CASELESS(data->cur, (int16_t)iter->c, data->next);
        } else {
            TRANS_TABLE_BUILD32_CASENESS(data->cur, (int16_t)iter->c, data->next);
        }
    }
    iter->state_id = data->next;
    data->next++;
}

static
void bfs_end_cb(trie_node_t *iter, void *args) {
    struct bfs_cb_data *data = (struct bfs_cb_data *)args;
    bool encode_caseless_flag = data->ctx->encode_caseless_flag;
    trie_node_t *tt = iter;

    if (tt->fail) {
        trie_node_t *fail = tt->fail;
        int fail_lv = 0;
        int first = iter->type ? 1 : 0; /* check if current state have output*/
        /* fill output table here from fail link */
        while (fail && fail->c) /* till root */
        {
            if (fail->type) {
                if (data->ctx->width == DFA_WIDTH_16) {
                    if (OUTTBL16(data->output_table[tt->state_id][OUTPUT_TBL_N_IDX] < OUTPUT_TBL_N_IDX)) {
                        OUTPUT_TABLE_BUILD16(tt->state_id, fail_lv + first, fail->type);
                        /* length table token length */
                        OUTTBL16(data->len_table[tt->state_id][fail_lv + first] = strlen(fail->word);
                    } else {
                        TADK_LOG(TADK_LOG_INFO, "Info: Exceed maximum number of output tokens.\n");
                    }
                } else {
                    if (OUTTBL32(data->output_table[tt->state_id][OUTPUT_TBL_N_IDX] < OUTPUT_TBL_N_IDX)) {
                        OUTPUT_TABLE_BUILD32(tt->state_id, fail_lv + first, fail->type);
                        /* length table token length */
                        OUTTBL32(data->len_table[tt->state_id][fail_lv + first] = strlen(fail->word);
                    } else {
                        TADK_LOG(TADK_LOG_INFO, "Info: Exceed maximum number of output tokens.\n");
                    }
                }
                fail_lv++;
            }
            fail = fail->fail;
        }
        /* fill trans table from fail link here */
        fail = tt->fail;
        while (fail) {
            trie_node_t *citer = fail->child;
            while (citer) {
                if (data->ctx->width == DFA_WIDTH_16) {
                    /* iterator first, fail link second */
                    if (TRTBL16(data->trans_table)[data->cur][(int16_t)citer->c] == 0) {
                        if (encode_caseless_flag == true) {
                            TRANS_TABLE_BUILD16_CASELESS(data->cur, (int16_t)citer->c, citer->state_id);
                        } else {
                            TRANS_TABLE_BUILD16_CASENESS(data->cur, (int16_t)citer->c, citer->state_id);
                        }
                    }
                } else {
                    /* iterator first, fail link second */
                    if (TRTBL32(data->trans_table)[data->cur][(int16_t)citer->c] == 0) {
                        if (encode_caseless_flag == true) {
                            TRANS_TABLE_BUILD32_CASELESS(data->cur, (int16_t)citer->c, citer->state_id);
                        } else {
                            TRANS_TABLE_BUILD32_CASENESS(data->cur, (int16_t)citer->c, citer->state_id);
                        }
                    }
                }
                citer = citer->sibling;
            }
            if (fail->c == 0) /* till root (include root)*/
                break;
            fail = fail->fail;
        }
    }

    data->cur++;
    if (((data->ctx->width == DFA_WIDTH_16) && data->cur > MAX_STATE_NUM_16) ||
        ((data->ctx->width == DFA_WIDTH_32) && data->cur > MAX_STATE_NUM_32)) {
        TADK_LOG(TADK_LOG_ERROR, "Exceed maximum number of states\n");
        exit(TADK_INPUT_ERROR);
    }
}

static
void set_output_state_flag(void *trans_table,
                           void *output_table,
                           int32_t n, dfa_width_t width) {
    int32_t i = 0;
    int32_t j = 0;

    if (width == DFA_WIDTH_16) {
        int16_t s = 0;
        for (i = 0; i < n; i++) {
            for (j = 0; j < M_CHAR; j++) {
                s = TRTBL16(trans_table)[i][j];
                if (OUTTBL16(output_table)[s][0] != 0) {
                    s |= STATUS_FLAG_16;
                    TRTBL16(trans_table)[i][j] = s;
                }
            }
        }
    }
    if (width == DFA_WIDTH_32) {
        int32_t s = 0;
        for (i = 0; i < n; i++) {
            for (j = 0; j < M_CHAR; j++) {
                s = TRTBL32(trans_table)[i][j];
                if (OUTTBL32(output_table)[s][0] != 0) {
                    s |= STATUS_FLAG_32;
                    TRTBL32(trans_table)[i][j] = s;
                }
            }
        }
    }
}

static
void dfa_context_init(dfa_context_t *ctx, trie_bfs_interface_t *tbi)
{
    if (ctx->width == DFA_WIDTH_16) {
        ctx->trans_table = calloc(M_CHAR * MAX_STATE_NUM_16,
                                  sizeof(int16_t));
        ctx->output_table = calloc(MAX_OUTPUT_TOKEN * MAX_STATE_NUM_16,
                                   sizeof(int16_t));
        ctx->len_table = calloc(MAX_OUTPUT_TOKEN * MAX_STATE_NUM_16,
                                sizeof(int16_t));
    }
    if (ctx->width == DFA_WIDTH_32) {
        ctx->trans_table = calloc(M_CHAR * MAX_STATE_NUM_32,
                                  sizeof(int32_t));
        ctx->output_table = calloc(MAX_OUTPUT_TOKEN * MAX_STATE_NUM_32,
                                   sizeof(int32_t));
        ctx->len_table = calloc(MAX_OUTPUT_TOKEN * MAX_STATE_NUM_32,
                                sizeof(int32_t));
    }

    data->trans_table = ctx->trans_table;
    data->output_table = ctx->output_table;
    data->len_table = ctx->len_table;

    tbi->bfs_start_cb = bfs_start_cb;
    tbi->bfs_main_cb = bfs_main_cb;
    tbi->bfs_end_cb = bfs_end_cb;

    data->cur = 0;
    data->next = 0;
    data->ctx = ctx;
}

static inline
trie_t *dfa_build_trie(const char *const *expressions,
                       const unsigned int *ids,
                       unsigned int elem_num, void *args) {
    unsigned int i;
    trie_t *t = trie_create();
    if (!t) {
        return NULL;
    }

    for (i = 0; i < elem_num; i++) {
        if (expressions[i])
            trie_insert(t, expressions[i], ids[i]);
    }
    ac_implement(t);
    return t;
}

static
void dfa_gen_trans_table(const char *const *expressions,
                         const unsigned int *ids,
                         unsigned int elem_num,
                         trie_bfs_interface_t *tbi, void *args) {
    trie_t *t = dfa_build_trie(expressions, ids, elem_num, tbi, args);
    if (!t) {
        TADK_LOG(TADK_LOG_ERROR, "build_trie failed\n");
        exit(TADK_INVALID);
    }
    gen_dfa_entry(t, args);
    trie_bfs_legacy(tbi, t, args);
    trie_destroy(t);
}

static
int dfa_basic_init(const char *const *expressions,
                   const unsigned int *ids,
                   unsigned int elem_num, dfa_context_t *ctx) {
    trie_bfs_interface_t tbi;
    bfs_data_t abfs_data;
    dfa_context_init(ctx, &tbi, &abfs_data);
    dfa_gen_trans_table(expressions, ids, elem_num, &tbi, &abfs_data);
    ctx->n_states = abfs_data.cur;
    set_output_state_flag(ctx->trans_table, ctx->output_table,
                          ctx->n_states, ctx->width);
#ifdef DEBUG_OUTPUT
    printf("n_states: %d\n", ctx->n_states);
#endif
    return 0;
}

int dfa_init(const char *const *expressions,
             const unsigned int *ids,
             unsigned int elem_num, dfa_context_t *ctx) {
    if (ctx->type == DFA_TYPE_BASIC) {
        return dfa_basic_init(expressions, ids, elem_num, ctx);
    }
    if (ctx->type == DFA_TYPE_LPM || ctx->type == DFA_TYPE_SUFFIX) {
        return dfa_lpm_init(expressions, ids, elem_num, ctx);
    }
    return -1;
}

int dfa_deinit(dfa_context_t *ctx) {
    if (!ctx) return -1;
    if (ctx->trans_table) {
        free(ctx->trans_table);
    }
    if (ctx->len_table) {
        free(ctx->len_table);
    }
    if (ctx->output_table) {
        free(ctx->output_table);
    }
    if (ctx->eod_table) {
        free(ctx->eod_table);
    }
    return 0;
}

static
int16_t fill_token16(int16_t state, int16_t output_table[][MAX_OUTPUT_TOKEN],
                     int16_t len_table[][MAX_OUTPUT_TOKEN], int to,
                     dfa_output_cb_t cb, void *args) {
    int16_t n_out_token = 0;
    int16_t k = 0;
    if (state & STATUS_BITMASK_16) {
        n_out_token = OUTTBL16(output_table[state][OUTPUT_TBL_N_IDX]);
        assert(n_out_token < MAX_OUTPUT_TOKEN);
        for (k = 0; k < n_out_token; k++) {
            if (cb) {
                int from = to - len_table[state][k];
                int output = output_table[state][k];
                cb(1, &from, &to, &output, args);
            }
        }
    }
    return state;
}

static
void dfa_engine16(dfa_context_t *ctx,
                  const char *input,
                  uint64_t len,
                  dfa_output_cb_t cb,
                  void *args) {
    int i = 0;
    int16_t s = 0;
    const unsigned char *ptr = (const unsigned char *)input;
    int16_t (*trans_table)[M_CHAR] = TRTBL16(ctx->trans_table);
    int16_t (*output_table)[MAX_OUTPUT_TOKEN] = OUTTBL16(ctx->output_table);
    int16_t (*len_table)[MAX_OUTPUT_TOKEN] = OUTTBL16(ctx->len_table);
    int16_t *eod_table = (int16_t *)ctx->eod_table;

    s = 0 & STATUS_BITMASK_16;
    for (i = 0; i < len; i++) {
        s = trans_table[s][(int16_t)(*ptr)];
        ptr++;
        s = fill_token16(s, output_table, len_table,
                         i + 1, cb, args);
    }
    if (eod_table == NULL) {
        s = trans_table[s][0];// include the '\0'
        s = fill_token16(s, output_table, len_table,
                         i + 1, cb, args);
    } else {
        s = eod_table[s];
        s = fill_token16(s, output_table, len_table,
                         i + 1, cb, args);
    }
}

static
int32_t fill_token32(int32_t state, int32_t output_table[][MAX_OUTPUT_TOKEN],
                     int32_t len_table[][MAX_OUTPUT_TOKEN], int to,
                     dfa_output_cb_t cb, void *args) {
    int32_t n_out_token = 0;
    int32_t k = 0;
    if (state & STATUS_FLAG_32) {
        state = state & STATUS_BITMASK_32;
        n_out_token = OUTTBL32(output_table[state][OUTPUT_TBL_N_IDX]);
        assert(n_out_token < MAX_OUTPUT_TOKEN);
        for (k = 0; k < n_out_token; k++) {
            if (cb) {
                int from = to - len_table[state][k];
                int output = output_table[state][k];
                cb(1, &from, &to, &output, args);
            }
        }
    }
    return state;
}

static
void dfa_engine32(dfa_context_t *ctx,
                  const char *input,
                  uint64_t len,
                  dfa_output_cb_t cb,
                  void *args) {
    int i = 0;
    int32_t s = 0;
    const unsigned char *ptr = (const unsigned char *)input;
    int32_t (*trans_table)[M_CHAR] = TRTBL32(ctx->trans_table);
    int32_t (*output_table)[MAX_OUTPUT_TOKEN] = OUTTBL32(ctx->output_table);
    int32_t (*len_table)[MAX_OUTPUT_TOKEN] = OUTTBL32(ctx->len_table);
    int32_t *eod_table = (int32_t *)ctx->eod_table;

    s = 0 & STATUS_BITMASK_32;
    for (i = 0; i < len; i++) {
        s = trans_table[s][(int32_t)(*ptr)];
        ptr++;
        s = fill_token32(s, output_table, len_table,
                         i + 1, cb, args);
    }
    if (eod_table == NULL) {
        s = trans_table[s][0];// include the '\0'
        s = fill_token32(s, output_table, len_table,
                         i + 1, cb, args);
    } else {
        s = eod_table[s];
        s = fill_token32(s, output_table, len_table,
                         i + 1, cb, args);
    }
}

void dfa_match(dfa_context_t *ctx,
               const char *input,
               uint64_t len,
               dfa_output_cb_t cb,
               void *args) {
    if (ctx->type == DFA_TYPE_BASIC) {
        if (ctx->width == DFA_WIDTH_16) {
            dfa_engine16(ctx, input, len, cb, args);
        } else if (ctx->width == DFA_WIDTH_32) {
            dfa_engine32(ctx, input, len, cb, args);
        }
    } else if (ctx->type == DFA_TYPE_LPM || ctx->type == DFA_TYPE_SUFFIX) {
        dfa_lpm_match(ctx, input, len, cb, args);
    }
}