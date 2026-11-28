/*
 * Copyright (c) 2015-2020, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of Intel Corporation nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "fdr_compile.h"
#include "fdr_compile_internal.h"
#include "fdr_confirm.h"
#include "fdr_dump.h"
#include "fdr_engine_description.h"
#include "fdr_internal.h"
#include "teddy_engine_description.h"
#include "teddy_internal.h"
#include "ue2common.h"
#include "util/dump_util.h"

#include <algorithm>
#include <cstdio>
#include <map>
#include <memory>
#include <vector>

#ifndef DUMP_SUPPORT
#error No dump support!
#endif

using namespace std;

namespace ue2 {

static
bool fdrIsTeddy(const FDR *fdr) {
    assert(fdr);
    u32 engine = fdr->engineID;

    /* teddys don't have an fdr engine description (which is why the dump code
     * is so broken). */

    return !getFdrDescription(engine);
}

static
void dumpLitIndex(const FDRConfirm *fdrc, FILE *f) {
    const u32 *lit_index = getConfirmLitIndex(fdrc);
    u32 num_lits = 1U << fdrc->nBits;
    u32 lits_used = count_if(lit_index, lit_index + num_lits,
                             [](u32 idx) { return idx != 0; });

    fprintf(f, "      load    %u/%u (%0.2f%%)\n", lits_used, num_lits,
            (double)lits_used / (double)(num_lits)*100);
}

static
void dumpConfirms(const void *fdr_base, u32 conf_offset, u32 num_confirms,
                  FILE *f) {
    const u32 *conf = (const u32 *)((const char *)fdr_base + conf_offset);
    for (u32 i = 0; i < num_confirms; i++) {
        const auto *fdrc = (const FDRConfirm *)((const char *)conf + conf[i]);
        fprintf(f, "    confirm %u\n", i);
        fprintf(f, "      andmsk  0x%016llx\n", fdrc->andmsk);
        fprintf(f, "      mult    0x%016llx\n", fdrc->mult);
        fprintf(f, "      nbits   %u\n", fdrc->nBits);
        fprintf(f, "      groups  0x%016llx\n", fdrc->groups);
        dumpLitIndex(fdrc, f);
    }
}

static
void dumpTeddyReinforced(const u8 *rmsk, const u32 num_tables, FILE *f) {
    // dump reinforcement masks
    for (u32 b = 0; b < num_tables; b++) {
        fprintf(f, "    reinforcement table for bucket %u..%u:\n",
                   b * 8, b * 8 + 7);
        for (u32 i = 0; i <= N_CHARS; i++) {
            fprintf(f, "      0x%02x: ", i);
            for (u32 j = 0; j < 8; j++) {
                u8 val = rmsk[b * ((N_CHARS + 1) * 8) + i * 8 + j];
                for (u32 k = 0; k < 8; k++) {
                    fprintf(f, "%s", ((val >> k) & 0x1) ? "1" : "0");
                }
                fprintf(f, " ");
            }
            fprintf(f, "\n");
        }
        fprintf(f, "\n");
    }
}

static
void dumpTeddyDupMasks(const u8 *dmsk, u32 numMasks, FILE *f) {
    // dump nibble masks
    u32 maskWidth = 2;
    fprintf(f, "    dup nibble masks:\n");
    for (u32 i = 0; i < numMasks * 2; i++) {
        fprintf(f, "      -%d%s: ", 1 + i / 2, (i % 2) ? "hi" : "lo");
        for (u32 j = 0; j < 16 * maskWidth * 2; j++) {
            u8 val = dmsk[i * 16 * maskWidth * 2 + j];
            for (u32 k = 0; k < 8; k++) {
                fprintf(f, "%s", ((val >> k) & 0x1) ? "1" : "0");
            }
            fprintf(f, " ");
        }
        fprintf(f, "\n");
    }
    fprintf(f, "\n");
}

static
void dumpTeddyMasks(const u8 *baseMsk, u32 numMasks, u32 maskWidth, FILE *f) {
    // dump nibble masks
    fprintf(f, "    nibble masks:\n");
    for (u32 i = 0; i < numMasks * 2; i++) {
        fprintf(f, "      -%d%s: ", 1 + i / 2, (i % 2) ? "hi" : "lo");
        for (u32 j = 0; j < 16 * maskWidth; j++) {
            u8 val = baseMsk[i * 16 * maskWidth + j];
            for (u32 k = 0; k < 8; k++) {
                fprintf(f, "%s", ((val >> k) & 0x1) ? "1" : "0");
            }
            fprintf(f, " ");
        }
        fprintf(f, "\n");
    }
    fprintf(f, "\n");
}

static
void dumpTeddy(const Teddy *teddy, FILE *f) {
    fprintf(f, "TEDDY:         %u\n", teddy->engineID);
    auto des = getTeddyDescription(teddy->engineID);
    if (!des) {
        fprintf(f, "   <unknown engine>\n");
        return;
    }

    fprintf(f, "    masks      %u\n", des->numMasks);
    fprintf(f, "    buckets    %u\n", des->getNumBuckets());
    fprintf(f, "    packed     %s\n", des->packed ? "true" : "false");
    fprintf(f, "    strings    %u\n", teddy->numStrings);
    fprintf(f, "    size       %zu bytes\n", fdrSize((const FDR *)teddy));
    fprintf(f, "    max length %u\n", teddy->maxStringLen);
    fprintf(f, "    floodoff   %u (%x)\n", teddy->floodOffset,
            teddy->floodOffset);
    fprintf(f, "\n");

    u32 maskWidth = des->getNumBuckets() / 8;
    size_t headerSize = sizeof(Teddy);
    const u8 *teddy_base = (const u8 *)teddy;
    const u8 *baseMsk = teddy_base + ROUNDUP_CL(headerSize);
    dumpTeddyMasks(baseMsk, des->numMasks, maskWidth, f);
    size_t maskLen = des->numMasks * 16 * 2 * maskWidth;
    const u8 *rdmsk = baseMsk + ROUNDUP_CL(maskLen);
    if (maskWidth == 1) { // reinforcement table in Teddy
        dumpTeddyReinforced(rdmsk, maskWidth, f);
    } else { // dup nibble mask table in Fat Teddy
        assert(maskWidth == 2);
        dumpTeddyDupMasks(rdmsk, des->numMasks, f);
    }
    dumpConfirms(teddy, teddy->confOffset, des->getNumBuckets(), f);
}

static
void dumpFDR(const FDR *fdr, FILE *f) {
    fprintf(f, "FDR:           %u\n", fdr->engineID);
    auto des = getFdrDescription(fdr->engineID);
    if (!des) {
        fprintf(f, "   <unknown engine>\n");
        return;
    }

    fprintf(f, "    domain     %u\n", fdr->domain);
    fprintf(f, "    stride     %u\n", fdr->stride);
    fprintf(f, "    strings    %u\n", fdr->numStrings);
    fprintf(f, "    size       %zu bytes\n", fdrSize(fdr));
    fprintf(f, "    max length %u\n", fdr->maxStringLen);
    fprintf(f, "    floodoff   %u (%x)\n", fdr->floodOffset, fdr->floodOffset);
    fprintf(f, "\n");

    dumpConfirms(fdr, fdr->confOffset, des->getNumBuckets(), f);
}

void fdrPrintStats(const FDR *fdr, FILE *f) {
    if (fdrIsTeddy(fdr)) {
        dumpTeddy((const Teddy *)fdr, f);
    } else {
        dumpFDR(fdr, f);
    }
}

bool dumpConfirmChains(const FDR *fdr, size_t bytecode_size, FILE *f) {
    if (!f) {
        return false;
    }
    // FDR and Teddy share these first six fields. Read by copy so that even a
    // malformed offset or an unaligned input cannot cause an unchecked load.
    Teddy header;
    DumpBuffer input(fdr, bytecode_size);
    if (!input.read(0, header) || header.size < sizeof(header)
        || header.size > bytecode_size) {
        fprintf(f, "ERROR: invalid FDR/Teddy header or allocation size\n");
        return false;
    }
    const DumpBuffer data = input.slice(0, header.size);
    u32 buckets = 0;
    if (auto des = getFdrDescription(header.engineID)) {
        buckets = des->getNumBuckets();
    } else if (auto teddy_des = getTeddyDescription(header.engineID)) {
        buckets = teddy_des->getNumBuckets();
    } else {
        fprintf(f, "ERROR: unknown engine %u\n", header.engineID);
        return false;
    }
    fprintf(f, "Confirmation chains: engine %u, buckets %u\n",
            header.engineID, buckets);
    if (!header.confOffset && !header.numStrings) {
        fprintf(f, "No confirmation entries.\n");
        return !ferror(f);
    }
    if (header.confOffset < sizeof(header)
        || header.confOffset % alignof(u32)
        || !data.containsArray(header.confOffset, buckets, sizeof(u32))) {
        fprintf(f, "ERROR: invalid confirmation switch table\n");
        return false;
    }
    size_t end_offset = data.size();
    if (header.floodOffset) {
        if (header.floodOffset <= header.confOffset
            || header.floodOffset > data.size()) {
            fprintf(f, "ERROR: invalid flood table offset\n");
            return false;
        }
        end_offset = header.floodOffset;
    }
    const DumpBuffer tables = data.slice(header.confOffset,
                                         end_offset - header.confOffset);
    vector<u32> offsets(buckets);
    for (u32 i = 0; i < buckets; i++) {
        if (!tables.read(i * sizeof(u32), offsets[i])) {
            fprintf(f, "ERROR: truncated confirmation switch table\n");
            return false;
        }
    }

    bool ok = true;
    size_t all_used = 0, all_collisions = 0, all_entries = 0, longest = 0;
    map<size_t, size_t> histogram;
    for (u32 bucket = 0; bucket < buckets; bucket++) {
        const size_t offset = offsets[bucket];
        if (!offset) {
            fprintf(f, "bucket %u: empty\n", bucket);
            continue;
        }
        size_t limit = tables.size();
        for (u32 other : offsets) {
            if (other > offset) {
                limit = min(limit, size_t(other));
            }
        }
        if (offset < ROUNDUP_CL(buckets * sizeof(u32))
            || offset >= limit || offset % alignof(FDRConfirm)) {
            fprintf(f, "ERROR: bucket %u has invalid confirm offset %zu\n",
                    bucket, offset);
            ok = false;
            continue;
        }
        const DumpBuffer table = tables.slice(offset, limit - offset);
        FDRConfirm confirm;
        const size_t index_offset = ROUNDUP_N(sizeof(confirm), alignof(u32));
        if (!table.read(0, confirm) || confirm.nBits >= 32
            || !table.containsArray(index_offset, size_t(1) << confirm.nBits,
                                     sizeof(u32))) {
            fprintf(f, "ERROR: bucket %u has invalid header/hash index\n", bucket);
            ok = false;
            continue;
        }
        const size_t slots = size_t(1) << confirm.nBits;
        const size_t literal_start = ROUNDUP_N(index_offset + slots * sizeof(u32),
                                               alignof(LitInfo));
        size_t used = 0, collisions = 0, entries = 0, max_chain = 0;
        fprintf(f, "bucket %u: slots %zu, groups 0x%016llx\n",
                bucket, slots, confirm.groups);
        for (size_t slot = 0; slot < slots; slot++) {
            u32 relative = 0;
            table.read(index_offset + slot * sizeof(u32), relative);
            if (!relative) {
                histogram[0]++;
                continue;
            }
            size_t pos = relative, chain = 0;
            bool complete = false;
            do {
                LitInfo lit;
                if (pos < literal_start || pos % alignof(LitInfo)
                    || !table.read(pos, lit)) {
                    fprintf(f, "ERROR: bucket %u hash %zu invalid LitInfo at %zu\n",
                            bucket, slot, pos);
                    ok = false;
                    break;
                }
                fprintf(f, "  hash %zu entry %zu: id=%u length=%u "
                           "v=0x%016llx msk=0x%016llx groups=0x%016llx "
                           "flags=0x%02x next=%u\n",
                        slot, chain, lit.id, lit.size, lit.v, lit.msk,
                        lit.groups, unsigned(lit.flags), unsigned(lit.next));
                if (!lit.size || lit.size > sizeof(CONF_TYPE)
                    || (lit.v & lit.msk) != lit.v) {
                    fprintf(f, "ERROR: invalid literal length/value mask\n");
                    ok = false;
                }
                chain++;
                pos += sizeof(lit); // next is a continuation flag, not a byte offset.
                complete = !lit.next;
            } while (!complete);
            if (!complete) {
                continue; // Do not include truncated chains in the statistics.
            }
            used++;
            entries += chain;
            collisions += chain > 1;
            max_chain = max(max_chain, chain);
            histogram[chain]++;
        }
        fprintf(f, "  used=%zu/%zu collisions=%zu entries=%zu "
                   "average_nonempty_chain=%.2f longest_chain=%zu\n",
                used, slots, collisions, entries,
                used ? double(entries) / used : 0.0, max_chain);
        all_used += used;
        all_collisions += collisions;
        all_entries += entries;
        longest = max(longest, max_chain);
    }
    fprintf(f, "Total: used=%zu collisions=%zu entries=%zu "
               "average_nonempty_chain=%.2f longest_chain=%zu\n",
            all_used, all_collisions, all_entries,
            all_used ? double(all_entries) / all_used : 0.0, longest);
    fprintf(f, "Chain length histogram (length: hash slots):\n");
    for (const auto &entry : histogram) {
        fprintf(f, "  %zu: %zu\n", entry.first, entry.second);
    }
    return ok && !ferror(f);
}

} // namespace ue2
