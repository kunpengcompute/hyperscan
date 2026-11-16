#ifndef DFA_LPM_H
#define DFA_LPM_H

#include "dfa_base.h"

int dfa_lpm_init(const char *const *expressions,
        const unsigned int *ids,
        unsigned int elem_num, dfa_context_t *ctx);

void dfa_lpm_match(dfa_context_t *ctx,
        const char *input,
        uint64_t len,
        dfa_output_cb_t cb,
        void *args);

int dfa_lpm_deinit(dfa_context_t *ctx);

#endif /* DFA_LPM_H */