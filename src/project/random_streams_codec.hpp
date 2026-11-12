#pragma once
#include "json.hpp"
#include "../core/random_streams.hpp"

namespace multra {
struct ProjectDocument; struct AuthoringDefinition;
// INPUT_STREAMS §3 and §8 slice 3 (D505 choice 1 b, D510): `definition.randomStreams`, the run's
// random-number mapping. Absent means `shared`; only `perInput` is written, which is what makes a
// file schema 74, so a project that keeps the single stream keeps its version and bytes. A file
// before 74 that names the key is refused at its path rather than read as something it never said.
bool ownsRandomStreams(const ProjectDocument&);
void rejectRandomStreamsBefore74(const Json& file);
// INVALID_RANDOM_STREAMS at definition.randomStreams for anything but "shared" or "perInput".
RandomStreams parseRandomStreams(const Json& definition);
void addRandomStreamsJson(const AuthoringDefinition&, Json& definition);
// The written names, also used by run, batch and comparison output (IS8).
const char* randomStreamsName(RandomStreams);
}
