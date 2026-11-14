// [glide fix 11] D2Glide shadow-draw field diagnostic — see ShadowDiag.cpp.
//
// Reconstructed (build round 2): the fork's port commit shipped ShadowDiag.cpp
// and its two call sites in Glide.cpp, but this header was never committed to
// either branch, so the D2Glide module could not compile. The API below is
// exactly the surface the call sites use: NoteWorldFrame() marks the first
// ground-tile frame; OnShadowQuad() captures the first kMaxRecords shadow
// quads after that marker into d2shadow-diag.txt (append-only file I/O,
// zero behavior change).
#pragma once

struct GlideTexInfoStrc;

namespace D2Glide::Renderer::ShadowDiag {

void NoteWorldFrame();
void OnShadowQuad(GlideTexInfoStrc * pTexInfo, const void * pFanVerts);

} // namespace D2Glide::Renderer::ShadowDiag
