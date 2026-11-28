/*
 * Copyright (c) 2015-2017, Intel Corporation
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

#ifndef ROSE_BUILD_DUMP_H
#define ROSE_BUILD_DUMP_H

#include "ue2common.h"

#include <map>
#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>

struct RoseEngine;

namespace ue2 {

class RoseBuildImpl;
struct Grey;
struct hwlmLiteral;
struct LitFragment;
struct left_id;
struct suffix_id;

#ifdef DUMP_SUPPORT
// Dump the Rose graph in graphviz representation.
void dumpRoseGraph(const RoseBuildImpl &build, const char *filename);

void dumpRose(const RoseBuildImpl &build,
              const std::vector<LitFragment> &fragments,
              const std::map<left_id, u32> &leftfix_queue_map,
              const std::map<suffix_id, u32> &suffix_queue_map,
              const RoseEngine *t);

void dumpMatcherLiterals(const std::vector<hwlmLiteral> &lits,
                         const std::string &name, const Grey &grey);

// Optional diagnostics only: deliberately not called by dumpRose or hsdump.
// The caller owns FILE*, and supplies the actual readable bytecode size.
struct LilyDumpEntry {
    std::string literal;
    u32 internal_report;
    u32 external_report;
    u32 ekey;
    unsigned flags;
};

// For Teddy, call BEFORE buildRose consumes lilyForTeddyPQ. Copies the queue
// without changing compilation; the returned vector preserves bucket order.
std::vector<LilyDumpEntry> collectLilyDumpSlots(const RoseBuildImpl &build,
                                               bool for_teddy);

// Empty metadata vectors still allow raw slot/mask decoding, but cannot
// validate source literals or determine which ordinary Lily slots are active.
bool dumpLilySlots(const RoseEngine *t, size_t bytecode_size,
                   const std::vector<LilyDumpEntry> &lily_slots,
                   const std::vector<LilyDumpEntry> &teddy_slots, FILE *f);

bool dumpComponentSummary(const RoseEngine *t, size_t bytecode_size, FILE *f,
                          size_t top_n = 10);

// Layout and structural checks for the Rose tables, queued NFAs and Lily
// payloads; this is not a complete validator for every engine's instructions.
bool dumpBytecodeLayout(const RoseEngine *t, size_t bytecode_size, FILE *f);

// Call after queue assignment while build-time graphs/reports still exist.
// Reachability expresses a structural dependency, not proof of a match.
bool dumpRuleDependencies(const RoseBuildImpl &build,
                          const std::vector<LitFragment> &fragments,
                          const std::map<left_id, u32> &leftfix_queue_map,
                          const std::map<suffix_id, u32> &suffix_queue_map,
                          const std::vector<LilyDumpEntry> &teddy_slots,
                          const RoseEngine *t, size_t bytecode_size, FILE *f);

#else

static UNUSED
void dumpRoseGraph(const RoseBuildImpl &, const char *) {
}

static UNUSED
void dumpRose(const RoseBuildImpl &, const std::vector<LitFragment> &,
              const std::map<left_id, u32> &, const std::map<suffix_id, u32> &,
              const RoseEngine *) {
}

static UNUSED
void dumpMatcherLiterals(const std::vector<hwlmLiteral> &, const std::string &,
                         const Grey &) {
}

#endif

} // namespace ue2

#endif
