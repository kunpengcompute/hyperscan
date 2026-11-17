#ifndef DFA_BASE_H
#define DFA_BASE_H

#include "stdint.h"
#include "stdbool.h"

#define MAX_STATE_NUM_16 (uint16_t)(1 << 15)
/* can not malloc (1 << 31) size memory */
#define MAX_STATE_NUM_32 (uint32_t)(1 << 20)

#define OUTPUT_TBL_FAIL_IDX (MAX_OUTPUT_TOKEN - 2)
#define OUTPUT_TBL_N_IDX (MAX_OUTPUT_TOKEN - 1)

#define TRTBL16(tbl)    ((int16_t (*)[N_CHAR])tbl)
#define TRTBL32(tbl)    ((int32_t (*)[N_CHAR])tbl)
#define OUTTBL16(tbl)   ((int16_t (*)[MAX_OUTPUT_TOKEN])tbl)
#define OUTTBL32(tbl)   ((int32_t (*)[MAX_OUTPUT_TOKEN])tbl)

#define TRANS_TABLE_BUILD16_CASELESS(s, c, i)
    do {
        TRTBL16(data->trans_table)[(s)][(c)] = i;
        if ((c) <= 'Z' && (c) >= 'A')
            TRTBL16(data->trans_table)[(s)][(c)+0x20] = i;
        else if ((c) <= 'z' && (c) >= 'a')
            TRTBL16(data->trans_table)[(s)][(c)-0x20] = i;
    } while(0)

#define TRANS_TABLE_BUILD32_CASELESS(s, c, i)
    do {
        TRTBL32(data->trans_table)[(s)][(c)] = i;
        if ((c) <= 'Z' && (c) >= 'A')
            TRTBL32(data->trans_table)[(s)][(c)+0x20] = i;
        else if ((c) <= 'z' && (c) >= 'a')
            TRTBL32(data->trans_table)[(s)][(c)-0x20] = i;
    } while(0)

#define TRANS_TABLE_BUILD16_CASENESS(s, c, i)
    do {
        ((int16_t (*)[N_CHAR])data->trans_table)[(s)][(c)] = i;
    } while(0)

#define TRANS_TABLE_BUILD32_CASENESS(s, c, i)
    do {
        ((int32_t (*)[N_CHAR])data->trans_table)[(s)][(c)] = i;
    } while(0)

#define OUTPUT_TABLE_BUILD16(s, n, t)
    do {
        /* output table token type */
        OUTTBL16(data->output_table)[(s)][(n)] = t;
        /* output table token number */
        OUTTBL16(data->output_table)[(s)][OUTPUT_TBL_N_IDX]++;
    } while(0)

#define OUTPUT_TABLE_BUILD32(s, n, t)
    do {
        /* output table token type */
        OUTTBL32(data->output_table)[(s)][(n)] = t;
        /* output table token number */
        OUTTBL32(data->output_table)[(s)][OUTPUT_TBL_N_IDX]++;
    } while(0)

typedef int (*dfa_output_cb_t)(int n, int *from, int *to, int *id, void *args);

typedef enum dfa_width {
    DFA_WIDTH_16,
    DFA_WIDTH_32,
} dfa_width_t;

typedef enum dfa_type {
    DFA_TYPE_BASIC,
    DFA_TYPE_LPM,
    DFA_TYPE_SUFFIX,
} dfa_type_t;

typedef struct dfa_context {
    void *trans_table;
    void *output_table;
    void *len_table;
    char *delims;
    int n_delims;
    int32_t n_states;
    dfa_width_t width;
    dfa_type_t type;
    bool encode_caseless_flag;
} dfa_context_t;

/**
 * DFA table initializer.
 *
 * Initialize a DFA table context.
 *
 * @param expressions
 *     Input expressions in dictionary.
 *
 * @param ids
 *     IDs corresponding to the expressions.
 *
 * @param elem_num
 *     The number of input.
 *
 * @param ctx
 *     Pointer of DFA table context.
 *
 * @return
 *     0 on success, other on failure.
 */
int dfa_init(const char *const *expressions,
        const unsigned int *ids,
        unsigned int elem_num,
        dfa_context_t *ctx);

/**
 * DFA table engine.
 *
 * Run the DFA table and execute callback function
 * when current state accepted.
 *
 * @param ctx
 *     Pointer of DFA table context.
 *
 * @param input
 *     Input string.
 *
 * @param len
 *     The length of input.
 *
 * @param cb
 *     Callback function to execute.
 *
 * @param args
 *     Arguments for callback.
 */
void dfa_match(dfa_context_t *ctx,
        const char *input,
        uint64_t len,
        dfa_output_cb_t cb,
        void *args);

/**
 * DFA table destroyer.
 *
 * Destroy a DFA table context.
 *
 * @param ctx
 *     Pointer of DFA table context.
 *
 * @return
 *     0 on success, other on failure.
 */
int dfa_deinit(dfa_context_t *ctx);

#endif /* DFA_BASE_H */