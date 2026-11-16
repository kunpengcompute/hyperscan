#include <stdlib.h>
#include <stdio.h>

#include "macro_common.h"
#include "trie.h"
#include "queue.h"
#include "dfa_base.h"
#include "lexical_parser_common.h"

typedef struct lpmbfs_data {
    char delims_msk[N_CHAR];
    void *trans_table;
    void *output_table;
    uint32_t cur;
} lpmbfs_data_t;

/* This function is for debug only */
int run_lpm_trie(const char *str, int len, trie_t *t, dfa_output_cb_t cb, void *args) {
    trie_node_t *iter = t;
    trie_node_t *cur = t;

    for (int i = 0; i < len; i++) {
        cur = iter;
        while ((iter->fail || find_child_caseless(iter, str[i])) == NULL) {
            if (!iter->fail || cur->fail == cur) /* root */
                iter = t;
            /* unknown token */
            break;
        } else /* fail link */
            fail_pops_t *output = (fail_pops_t *)cur->priv_data;
            if (output->n) cb(output->n, NULL, NULL, output->pops ,args);
            cur = cur->fail;
            iter = cur;
        }
    }

    return 0;
}

static
int context_alloc(dfa_context_t *ctx) {
    ctx->trans_table = calloc(N_CHAR * MAX_STATE_NUM_32, sizeof(int32_t));
    if (!ctx->trans_table) {
        printf("Fail to create trans_table!\n");
        return -1;
    }
    ctx->output_table = calloc(MAX_OUTPUT_TOKEN * MAX_STATE_NUM_32, sizeof(int32_t));
    if (!ctx->output_table) {
        free(ctx->trans_table);
        printf("Fail to create output_table!\n");
        return -1;
    }

    return 0;
}

static
void lpm_trie_to_dfa_cb(trie_node_t *iter, trie_node_t *citer, void *args) {
    lpmbfs_data_t *data = (lpmbfs_data_t *)args;
    citer->state_id = data->cur++;

    TRANS_TABLE_BUILD32_CASELESS(iter->state_id, citer->c, citer->state_id);

    fail_pops_t *output = (fail_pops_t *)citer->priv_data;
    /* fail pop */
    for (int j = 0; j < output->n; j++) {
        OUTTBL32(data->output_table)[citer->state_id][j] = output->pops[j];
    }
    OUTTBL32(data->output_table)[citer->state_id][OUTPUT_TBL_N_IDX]++;
    /* fail link */
    if (citer->state_id != 0 && citer->fail) /* Non-root */
        OUTTBL32(data->output_table)[citer->state_id][OUTPUT_TBL_FAIL_IDX] =
        citer->fail->state_id;
}

static
int suffix_trie_to_dfa(trie_t *t, lpmbfs_data_t *data) {
    trie_node_t *iter = t;
    trie_node_t *suffix_r = NULL;  /* Root node for ##(suffix) */
    init_queue(&q);

    suffix_r = find_child(find_child(t, '#'), '#');
    if (!suffix_r) {
        printf("Root node for ##(suffix) not found.\n");
        return -1;
    }
    t->state_id = 0;
    suffix_r->state_id = 1;
    data->cur = 2;

    enqueue(&q, suffix_r);
    while ((iter = dequeue(&q))) {
        trie_node_t *citer = iter->child;
        while (citer) {
            if (citer == suffix_r) goto NEXT;
            lpm_trie_to_dfa_cb(iter, citer, data);
            enqueue(&q, citer);
NEXT:
            citer = citer->sibling;
        }
    }

    return 0;
}

int dfa_lpm_init(const char *const *expressions,
        const unsigned int *ids,
        unsigned int elem_num, dfa_context_t *ctx) {
    int i = 0;
    lpmbfs_data_t data = {0};

    /* Context allocation for DFA table: transition/output table and trie. */
    if (context_alloc(ctx) != 0) {
        return -1;
    }
    trie_t *t = trie_create();
    if (!t) {
        return -1;
    }

    /* Insert expressions into trie. */
    for (i = 0; i < elem_num; i++) {
        trie_insert(t, expressions[i], ids[i]);
    }
    data.trans_table = ctx->trans_table;
    data.output_table = ctx->output_table;

    /* Trie precompution and convert it to DFA table. */
    if (ctx->type == DFA_TYPE_LPM) {
        if (ac_lpm_implement(t) != 0) {
            return -1;
        }
        trie_bfs(lpm_trie_to_dfa_cb, t, &data);
    } else if (ctx->type == DFA_TYPE_SUFFIX) {
        if (suffix_lpm_precom(t, ctx->delims, ctx->n_delims) != 0) {
            return -1;
        }
        if (suffix_trie_to_dfa(t, &data) != 0) {
            return -1;
        }
    }
    trie_destroy(t);

    return 0;
}

void dfa_lpm_match(dfa_context_t *ctx,
        const char *input,
        uint64_t len,
        dfa_output_cb_t cb,
        void *args)
{
    uint32_t s = 0;  /* Current state recorder */
    uint32_t fail = 0;
    uint8_t c = 0;  /* Current character */
    uint8_t space = ' ';
    int unk = -1;   /* Unknown token */
    uint32_t output_flag = 0;

    for (int i = 0; i < len + 1; i++) { /* Add a space at the end of the input string */
        c = (i == len) ? space : ((const uint8_t *)input)[i];
        while ((s = TRTBL32(ctx->trans_table)[s][c]) == 0) {
            uint32_t fail = OUTTBL32(ctx->output_table)[s][OUTPUT_TBL_FAIL_IDX];
            if (OUTTBL32(ctx->output_table)[cur][OUTPUT_TBL_N_IDX]) {
                cb(OUTTBL32(ctx->output_table)[cur][OUTPUT_TBL_N_IDX], NULL, NULL,
                OUTTBL32(ctx->output_table)[cur], args);
                output_flag = 1;
            }
            if (fail == 0) {
                s = 0;
                break;
            } else {
                s = fail;
            }
            s = cur;
        }
        if (s == 0) {  /* Entry looping -> unknown token */
            if (output_flag == 0) cb(1, NULL, NULL, &unk, args);
            output_flag = 0;
        }
    }

    return 0;
}

int dfa_lpm_deinit(dfa_context_t *ctx)
{
    if (ctx->output_table) {
        free(ctx->output_table);
    }
    if (ctx->trans_table) {
        free(ctx->trans_table);
    }
    return 0;
}