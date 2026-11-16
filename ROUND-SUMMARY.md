# Round summary — R160 (rev 176, 2026-11-16): shared-stash data safety (ROB-01 + ROB-02)

## The ask

No owner ask — this round lands the two **High** findings from the code audit
(`audit.md`, Priority Matrix near-term line), both in the shared-stash TU
`Source/D2Game/PLAYER/VanillaPlusStash.cpp`. R159 already retired the audit's
quick wins; R160 retires the only two findings that could destroy player data.

## ROB-01 — a transient read failure no longer pins the empty state

Before: `VPSHARED_EnsureLoaded` set `gbSharedLoaded = 1` before knowing the read
outcome, so if the first toggle landed while `_d2vp_shared.sss` was transiently
locked (an AV scan, a backup or a sync client holding it), the session ran on an
all-empty in-memory state — and the next save serialized that empty state over
the good on-disk file (the one-generation `.bak` kept the previous file, but
pages and gold beyond what the current session touched were lost until a manual
restore nobody knows to do).

Now the read path classifies its verdicts:

- `VPSHARED_TryReadFile` distinguishes the open failure: file-not-found /
  path-not-found = the benign first-run case (pin empty, saves fine — unchanged),
  a corrupt file = benign (pin; the `.bak` chase unchanged — a re-read cannot
  help), **any other open error = transient, returns -1**.
- `VPSHARED_EnsureLoaded` defers on a transient verdict: `gbSharedLoaded` stays 0,
  so the next toggle retries the read from scratch; `gbSharedLoadFailed = 1` gates
  everything downstream.
- `VPSHARED_ActivatePage` refuses the toggle outright while `gbSharedLoadFailed`
  is set (mark: `toggle refused - stash file unreadable (will retry; disk
  untouched)`) — the personal set is never parked against a failed load.
- `VPSHARED_WriteFileLocked` refuses the save while it is set (mark: `save
  refused - last load failed (disk untouched)`) — the second lock, covering the
  flush-before-save path.

A locked file can now only ever cost the toggle attempt itself; it can never
flush an empty session over the real stash.

## ROB-02 — a partial materialization no longer erases records

Before: if a record failed to decode during materialization
(`PLRSAVE_DispatchItemLoadByFormat` returned an error — the pre-existing
"load error %d - partial page" mark), the page's live page-4 set was a subset of
the stored block, and the next page flip or toggle-to-personal re-serialized that
subset over the full stored block — the undecoded records were dropped
permanently, silently, on the next save.

Now `VPSHARED_MaterializePage4` returns materialization health (1 = complete,
including the legitimately-empty block; 0 = a record failed to decode), stored in
`gbActivePagePartial` on every materialize. While it is set:

- the page→page switch skips the serialize-into-block and keeps page A's stored
  block authoritative (mark: `page N was partial - stored copy kept, session
  edits on it discarded`);
- the toggle-to-personal skips it the same way.

The trade is deliberate: session edits made on a partially-materialized page are
discarded (the user played on a degraded view), but records that exist on disk
are never erased by a decode failure. A decode failure now degrades one session
instead of destroying the page.

## Guarantees

- **Zero v2-format changes** — the `.sss` shape, checksums, `.bak` rotation and
  header layout are untouched.
- **Zero healthy-path behavior change** — when every read and every decode
  succeeds, the load → toggle → page flip → toggle back → save chain is
  byte-identical to R158/R159.
- New state fields (`gbSharedLoadFailed`, `gbActivePagePartial`) are BSS-zero
  like the rest of `VPStashState` — no init path, no ownership-pin interaction.

## Verification

Formal ask: SMOKE **§X 97–99** — 97 = the transient-lock drill (exclusive
`CreateFile` hold from PowerShell, toggle refused, retry after release), 98 = the
no-file benign path regression (rename away → first-run shape, file recreated on
save), 99 = the healthy round-trip regression (no `was partial` mark, `.bak`
rotates). Gate: `tools/docs_gate.py` green at round R160.
