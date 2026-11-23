// vanilla-plus shared stash: server-side container (track N, round 1).
//
// Fork-owned TU (picked up by CMake's GLOB_RECURSE). Do not move this code
// into upstream files; upstream files only ever call the entry points
// declared in VanillaPlusStash.h from thin VP-marked sites
// (PlayerTrade::ClickButton, PlayerSave::SaveGameAllGameTypes + tail).
//
// ---------------------------------------------------------------------------
// How the shared page works (round 1 - one page, retail-shaped; two shared
// pages since R75, TEN since R82, A HUNDRED since R146 - the toggle's param
// word carries the ABSOLUTE desired page 0 personal / 1..100 shared-N, and
// the shared sets live as per-page 'JM' blocks swapped into page 4 one at a
// time):
//
// While a shared page is ACTIVE, the player's INVPAGE_STASH (page 4)
// inventory holds that page's shared set - real item units materialized from
// the .sss file by the retail save-stream loader - and the personal set waits
// in a memory snapshot. Every retail stash flow works on it unchanged:
// grid draw, C->S 0x19 pick, C->S 0x18 place, swap 0x1F, gold buttons.
// The fork adds no packets, no item fields, no container code.
//
// Switching pages (client tab/arrows -> C->S 0x4F button 0x7D):
//   1. serialize the current page-4 set with the retail per-item encoder
//      (D2Common::ITEMS::Items::SerializeItem) into a 'JM' block - the
//      exact bytes the .d2s item block uses;
//   2. dematerialize those units (ITEM_RemoveItemFromCurrentLocation,
//      which also pushes the client 0x9D remove packets);
//   3. materialize the other set (PlayerSave::PLRSAVE_DispatchItemLoadByFormat,
//      format 0x60 - the same call the trade-cancel restore uses);
//   4. push the client 0x9C put-in-container packets for the new set;
//   5. swap the goldbank borrow (see below); on shared->personal also
//      write the .sss file.
//
// Shared gold borrows the retail UNITSTAT_goldbank stat while the shared
// page is active: the retail stash-gold deposit (0x4F/0x14) and withdraw
// (0x4F/0x13) buttons and the stash gold display keep working verbatim,
// but move shared gold. The toggle swaps goldbank between the personal
// snapshot and the shared pool; the save flush absorbs whatever the retail
// buttons moved back into the .sss.
//
// .d2s safety (owner constraint 2, invariant I1): the shared set never
// survives into a character save. VPSHARED_FlushBeforeSave runs at the top
// of PlayerSave::SaveGameAllGameTypes - the single dispatcher every save
// path (SP file, LAN buffer, BNet) goes through - and parks the shared
// page (write .sss, restore the personal set and the personal goldbank)
// BEFORE MakeSaveFile serializes. When the page is inactive the flush is a
// zero-op: no file IO, no marks, vanilla byte-for-byte. R204 adds the tail
// hook VPSHARED_AfterSave (thin VP site at the end of SaveGameAllGameTypes):
// realm saves fire per-player on intervals/waypoints, not just at exit, and
// without the re-activation every such save would kick the holder back to
// their personal page.
//
// .sss format (<save dir>\_d2vp_shared.sss, own file, owner constraint 1):
//   v2 (written since R75 - "add a 2nd page and buttons/arrows to navigate";
//   widened to ten pages by R82 - "increase the shared stash pages to 10" -
//   the exact extension the header reserved room for: SAME magic + version,
//   only the page count and the pair table grow):
//     0   4   magic 'V','P','S','2'
//     4   2   version (u16, 2)
//     6   2   pageCount (u16, 100 since R146 - the "Then 100" leg; ten
//             since R82; pre-R82 files carry 2 and read back with pages
//             3..100 empty - the next save rewrites a hundred)
//     8   4   shared gold (u32 - ONE pool across all pages)
//     12  8*N page records, N = pageCount: blockSize page-i (u32) at
//             12+8*i, checksum page-i (u32, additive byte-sum) at 16+8*i
//             (header is 12 + 8*N bytes: 28 at two pages, 92 at ten, 812 at
//             a hundred; the first page's block starts right behind it)
//   v1 (pre-R75 files, read back): 20-byte header (magic 'V','P','S','1',
//   version 1, pageCount 1, gold), the single page block right behind.
//   R204: on realms the file is PER ACCOUNT - "_d2vp_shared_<account>.sss"
//   (each account shares its pool across its own characters only, per the
//   owner's "each player to have their own shared stash, not shared with
//   other players, just shared for that one account"); single-player keeps
//   the legacy accountless "_d2vp_shared.sss" so existing SP pools carry
//   over untouched. Same bytes, same format, one file per key.
//
// ---------------------------------------------------------------------------
// R204 - realm semantics (replaces the v1 single-player-only scope guard):
//   The server half now handles the toggle for EVERY game type (single-
//   player local server, BNet closed realm, Open Battle.net, LAN - all are
//   D2Game in this build). Pools are PER ACCOUNT: the pool key is the
//   player's account name (empty key = the legacy SP global file), so two
//   players online hold two different pools and can use the shared stash
//   simultaneously without ever touching each other's items - no exclusivity
//   needed. Each CHARACTER materializes its account's page into its own
//   page 4 with its own personal snapshot, tracked per player.
//   Deployment note: on realms this module runs on the D2VP-D2GS server -
//   the server build must include this D2Game (a vanilla d2gs silently
//   ignores button 0x7D; harmless, no toggle).
//   Known v1-online limits, both documented not accidental: the pool file is
//   per account, not per realm-wide-account-lock, so the SAME account logged
//   into two games at once would race its own file (classic realms allow one
//   character per account, so this needs an unusual setup); and the pool
//   cache lives per D2Game instance, so two concurrent games of the same
//   account last-writer-win on the file.

#include "Source/D2Game/PLAYER/VanillaPlusStash.h"

#include "Source/D2Client/D2ClientStrc.h"
#include "Source/D2Client/GAME/Game/SGAMEDATA.h"
#include "Source/D2Client/GAME/SCmd.h"
#include "Source/D2Common/D2InventoryStrc.h"
#include "Source/D2Common/D2StatListExStrc.h"
#include "Source/D2Common/D2UnitStrc.h"
#include "Source/D2Common/INVENTORY/Inventory.h"
#include "Source/D2Common/ITEMS/Items.h"
#include "Source/D2Client/UNIT/Item.h" // [R211] D2UnitDataItemDetailsStrc (Route B materialise)
#include "Source/D2Common/Stats/StatsEx.h"
#include "Source/D2Common/Unit.h"
#include "Source/D2Game/GAME/Clients.h" // GetAccountName (the R204 per-account pool key)
#include "Source/D2Game/GAME/SCmd.h"
#include "Source/D2Game/ITEMS/ItemMode.h"
#include "Source/D2Game/PLAYER/PlayerSave.h"
#include "Source/D2Game/PLAYER/PlayerSave2.h"
#include "Source/D2Game/UNIT/Party.h"
#include "Source/D2Game/UNIT/SUnit.h"
#include "Source/Fog/D2PoolManagerStrc.h"
#include "Source/Fog/Memory.h"
#include "Source/Fog/Source/SFile.h"
#include "Source/globals/globals.h"
#include "Redivivus/d2_boot.h" // [d2boot]

#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#if defined(VPSTASH_113D_DLL)
// [R218] The standalone d2vpstash.dll must not link the rest of the tree, so the
// boot-log marks are satisfied locally: append to d2vpstash.log next to the DLL.
// (The in-exe build keeps the real D2BOOT_* writers.)
static void VPSHARED_Log113d(const char * szFmt, ...) {
    char szPath[260];
    DWORD dwLen = GetModuleFileNameA((HMODULE)&VPSHARED_Log113d, szPath, 240);
    if (dwLen && dwLen < 240) {
        char * pSlash = strrchr(szPath, '\\');
        if (pSlash) {
            pSlash[1] = '\0';
            strcat(szPath, "d2vpstash.log");
            FILE * f = fopen(szPath, "a");
            if (f) {
                va_list ap;
                va_start(ap, szFmt);
                vfprintf(f, szFmt, ap);
                va_end(ap);
                fputc('\n', f);
                fclose(f);
            }
        }
    }
}
#define D2BOOT_Mark(...)    VPSHARED_Log113d(__VA_ARGS__)
#define D2BOOT_MarkFmt(...) VPSHARED_Log113d(__VA_ARGS__)
#endif // VPSTASH_113D_DLL

// ---------------------------------------------------------------------------
// Tunables

// Per-page serialization cap. The retail stash grid cannot hold more than
// 64 items even in pathological layouts (v1 page = the retail stash grid);
// a load claiming more is treated as a corrupt count (clamped, marked).
constexpr int VPSTASH_PAGE_ITEM_CAP = 64;
// 'JM' block buffer cap. The retail trade copy uses 0x2000; 0x4000 gives
// 64 worst-case item records generous headroom.
constexpr uint32_t VPSTASH_BLOCK_CAP = 0x4000;
// .sss header sizes (see format sketch above): v1 = one page (20 bytes);
// v2 = 12 bytes of common fields + one size/checksum pair per page.
constexpr uint32_t VPSTASH_HEADER_SIZE_V1 = 20;
// Shared pages this build writes (R146: a hundred - the owner's "Good job,
// works. Let's increase the Shared Stash pages to 100", completing the "If
// that works we'll do 10 pages. Then 100." roadmap; R82 wrote ten, pre-R82
// two).
constexpr uint16_t VPSTASH_PAGE_COUNT = 100;
constexpr uint32_t VPSTASH_HEADER_SIZE_V2 = 12 + 8 * VPSTASH_PAGE_COUNT;
constexpr uint16_t VPSTASH_FILE_VERSION = 2;      // the shape this build writes
constexpr uint16_t VPSTASH_FILE_VERSION_V1 = 1;   // pre-R75 field files (read back)
constexpr uint16_t VPSTASH_PAGE_COUNT_V1 = 1;
// The shared pages (internal indices 0..99 = the UI's "page 1".."page 100"
// in marks and UI - the personal page is the retail page-4 set itself, not
// stored here).
constexpr int VPSTASH_PAGES = 100;
// [R204] Per-account pool cache + per-character activation records. Retail
// games hold at most 8 players, so 8 entries each cover every player; the
// entries live for the game (ownership pin drops everything at teardown).
constexpr int VPSTASH_MAX_POOLS = 8;
constexpr int VPSTASH_MAX_ACTIVE = 8;
constexpr size_t VPSTASH_KEY_MAX = 16; // d2 account-name width

// ---------------------------------------------------------------------------
// Per-game state. Ownership is pinned to the SGAMEDATA pointer - the first
// touch from a different game resets the state (the old game's memory pool
// is gone by then; the stale pointers are dropped, never dereferenced).
//
// [R204] Pools are PER ACCOUNT (key = account name, "" = the legacy SP
// global file), and activations are PER CHARACTER: each player materializes
// their own account's page into their own page 4 with their own personal
// snapshot, so any number of players can use the shared stash at once.

struct VPPool {
    int gbUsed;                 // entry in use (key assigned)
    char szKey[VPSTASH_KEY_MAX + 1]; // account name, "" = the legacy SP global file
    int gbLoaded;               // 1 once this pool's .sss was read with a FINAL verdict (success, or benign no-file/corrupt degrade); a transient failure leaves it 0 so the next toggle retries
    int gbLoadFailed;           // [ROB-01] 1 = the last load attempt hit a TRANSIENT error (file exists but locked/unreadable): the memory state may be an empty shell, so toggles and saves are refused until a read succeeds
    uint32_t dwSharedGold;      // shared gold pool - ONE pool across all pages (.sss header)
    uint8_t * apSharedBlock[VPSTASH_PAGES]; // 'JM' block per shared page (loaded / last serialized)
    uint32_t anSharedSize[VPSTASH_PAGES];
};

struct VPActive {
    int gbUsed;                 // entry in use (a player holds or has parked a shared page)
    D2UnitStrc * pUnit;         // the character (pin; pointer-compare only, never dereferenced after leave)
    VPPool * pPool;             // this character's account pool
    int gnPage;                 // 0 = personal live (entry parked / pre-activation); 1..100 = shared-N materialized in this character's page 4
    int gbActivePagePartial;    // [ROB-02] 1 = the active shared page materialized PARTIALLY (a record failed to decode): page 4 is a subset of the stored block, which stays authoritative - the park skips the re-serialize
    uint8_t * pPersonalBlock;   // 'JM' block of the personal set while a shared page is active
    uint32_t nPersonalSize;
    uint32_t dwPersonalGoldSnapshot; // goldbank value at toggle-to-shared
    int gnPostSavePage;         // [R204] 0 = none; 1..100 = re-activate this shared page after the current save builds (consumed by VPSHARED_AfterSave)
};

static struct {
    ::SGAMEDATA * pOwnerGame;   // owning game (ownership pin / lazy reset)
    VPPool aPools[VPSTASH_MAX_POOLS];
    VPActive aActive[VPSTASH_MAX_ACTIVE];
} s_VpStash;

// Plain BSS static (zero-init by definition): no guard macro needed - the
// state holds no code pointers and the ownership pin keeps stale pools from
// ever being dereferenced.

// [R207] Fork-private heap. Every buffer this module allocates is OUR data
// (serialized 'JM' page blocks, the personal-set snapshot, the .sss write
// buffer) - none of it is ever handed to the game as a pooled object, so it
// does not belong in D2's memory pool. It used to be allocated from
// pGame->pMemoryPool, which made SGAMEDATA::pMemoryPool an ABI dependency;
// when the D2GS agent's 1.13d evidence failed to corroborate the 0xAC offset
// (the site it cited resolved to KERNEL32!IsBadCodePtr operating on a
// callback field, not an allocation), that dependency became the single
// unverified risk in the whole port. Owning our own heap removes it: the
// module now touches NO game-struct field except D2UnitStrc::pInventory /
// eUnitType (both confirmed 0x60 / 0x00 on 1.13d) and the confirmed numeric
// constants (goldbank 15, INVPAGE_STASH 4).
static void * VPSHARED_Alloc(uint32_t nSize) {
    return HeapAlloc(GetProcessHeap(), 0, (SIZE_T)nSize);
}

static void VPSHARED_Free(void * pBlock) {
    if (pBlock) {
        HeapFree(GetProcessHeap(), 0, pBlock);
    }
}

// Release every block we own (pool pages + parked personal sets). Called when
// the owning game changes - the old game's teardown no longer reclaims our
// memory for us now that we own it.
static void VPSHARED_FreeAllState() {
    for (int i = 0; i < VPSTASH_MAX_POOLS; ++i) {
        for (int p = 0; p < VPSTASH_PAGES; ++p) {
            VPSHARED_Free(s_VpStash.aPools[i].apSharedBlock[p]);
            s_VpStash.aPools[i].apSharedBlock[p] = nullptr;
            s_VpStash.aPools[i].anSharedSize[p] = 0;
        }
    }
    for (int i = 0; i < VPSTASH_MAX_ACTIVE; ++i) {
        VPSHARED_Free(s_VpStash.aActive[i].pPersonalBlock);
        s_VpStash.aActive[i].pPersonalBlock = nullptr;
        s_VpStash.aActive[i].nPersonalSize = 0;
    }
}

// [R206] Server-local ini path: this module must not depend on D2Client
// (D2_MODULE_D2Game_SEES has no D2Client - correct for a standalone server
// DLL build, and on a realm D2Client does not exist at all). The old code
// borrowed ALTTOGGLE_BuildIniPath from D2Client/UI/VanillaPlusCore.h, which
// only linked because this build whole-archives every module into one exe.
// Same semantics: "<host exe dir>\d2vp.ini" - in single-player that is the
// client next to d2vp.exe, on a realm host it is the server's own d2vp.ini
// next to the D2GS host executable.
static BOOL VPSHARED_BuildIniPath(char * szIniPath) {
    szIniPath[0] = '\0';
    DWORD dwLen = GetModuleFileNameA(nullptr, szIniPath, 240);
    if (!dwLen || dwLen >= 240) {
        return FALSE;
    }
    char* pSlash = strrchr(szIniPath, '\\');
    if (!pSlash) {
        return FALSE;
    }
    pSlash[1] = '\0';
    lstrcatA(szIniPath, "d2vp.ini");
    return TRUE;
}

// [vanilla-plus shared-stash] ini gate, server side. Same file, same key,
// same 2 s hot-read cadence as the client gate (the SP server runs in the
// same process, so the exe dir is identical). Missing/garbage = 0 = inert.
static int s_nVpStashGateCache = 0;
static DWORD s_dwVpStashGateTick = 0;
// [R205] Wire encoding, read on the same 2 s cadence as the gate. Must match
// the client's [features] shared_stash_button / shared_stash_pad_magic.
static int s_nVpStashButtonCache = VPSHARED_BUTTON_TOGGLE;
static int s_nVpStashPadMagicCache = VPSHARED_PAD_MAGIC_DEFAULT;

static BOOL VPSHARED_FeatureActive() {
    DWORD dwNow = GetTickCount();
    if (!s_dwVpStashGateTick || dwNow - s_dwVpStashGateTick >= 2000) {
        char szIniPath[260];
        s_dwVpStashGateTick = dwNow;
        if (VPSHARED_BuildIniPath(szIniPath)) {
            // Missing file/section/key or a non-numeric value = 0 = inert.
            s_nVpStashGateCache = GetPrivateProfileIntA("features", "shared_stash", 0, szIniPath) ? 1 : 0;
            s_nVpStashButtonCache = GetPrivateProfileIntA("features", "shared_stash_button",
                                                          VPSHARED_BUTTON_TOGGLE, szIniPath);
            s_nVpStashPadMagicCache = GetPrivateProfileIntA("features", "shared_stash_pad_magic",
                                                            VPSHARED_PAD_MAGIC_DEFAULT, szIniPath);
        } else {
            s_nVpStashGateCache = 0;
            s_nVpStashButtonCache = VPSHARED_BUTTON_TOGGLE;
            s_nVpStashPadMagicCache = VPSHARED_PAD_MAGIC_DEFAULT;
        }
    }
    return s_nVpStashGateCache;
}

// Drop the state if it belongs to a previous game (its memory pool was
// freed at teardown; the pointers are stale and must never be touched).
static void VPSHARED_EnsureOwner(::SGAMEDATA * pGame) {
    if (s_VpStash.pOwnerGame == pGame) {
        return;
    }
    // [R207] We own our buffers now - the old game's teardown no longer frees
    // them, so release them before dropping the pointers.
    if (s_VpStash.pOwnerGame) {
        VPSHARED_FreeAllState();
    }
    memset(&s_VpStash, 0, sizeof(s_VpStash));
    s_VpStash.pOwnerGame = pGame;
}

// ---------------------------------------------------------------------------
// [R204] Account key + pool/activation lookup

// The pool key for a character: the account name (sanitized to the file-name
// safe subset d2 account names already use), or "" when the session has no
// account - single-player, whose legacy global file must keep working for
// existing installs. The client struct is resolved through the same helper
// the save dispatcher uses; no account = empty key, never an error.
static void VPSHARED_GetAccountKey(D2UnitStrc * pUnit, char * szOut, size_t nSize) {
    szOut[0] = '\0';
    if (!pUnit || nSize == 0) {
        return;
    }
#if defined(VPSTASH_D2GS_113D)
    // no client access in the realm build (import-free); interim fixed key below
#else
    D2ClientStrc * pClient = D2Game::UNIT::SUnit::SUNIT_GetClient(pUnit, ".\\PLAYER\\VanillaPlusStash.cpp", 0x204);
    if (!pClient) {
        return;
    }
#endif
    char szAccount[52];
    memset(szAccount, 0, sizeof(szAccount));
#if defined(VPSTASH_D2GS_113D)
    // [R214] INTERIM: Clients::GetAccountName (#10) is not pinned yet, so the
    // realm build keys its pool on a fixed smoke-test name. Per-account naming
    // resumes as soon as #10 lands (one line: replace this with the resolved
    // accessor). Per-character/per-account isolation is absent until then - the
    // pool is shared per game instance, which is acceptable ONLY for a private
    // smoke test.
    strcpy(szAccount, "smoketest");
    D2BOOT_Mark("shared stash [1.13d]: interim pool key 'smoketest' (#10 GetAccountName not pinned)"); // [d2boot]
#else
    D2Game::GAME::Clients::GetAccountName(pClient, szAccount);
#endif
    // File-name sanitize: d2 account names are alphanumeric already; keep
    // that subset and cap at the d2 account width. Anything else (or an
    // empty name - single-player) collapses to the legacy "" key.
    size_t nOut = 0;
    for (size_t i = 0; szAccount[i] && nOut + 1 < nSize && nOut < VPSTASH_KEY_MAX; ++i) {
        char c = szAccount[i];
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-') {
            szOut[nOut++] = c;
        }
    }
    szOut[nOut] = '\0';
}

static VPPool * VPSHARED_FindPool(const char * szKey) {
    for (int i = 0; i < VPSTASH_MAX_POOLS; ++i) {
        if (s_VpStash.aPools[i].gbUsed && strcmp(s_VpStash.aPools[i].szKey, szKey) == 0) {
            return &s_VpStash.aPools[i];
        }
    }
    return nullptr;
}

static VPPool * VPSHARED_GetPool(const char * szKey) {
    VPPool * pPool = VPSHARED_FindPool(szKey);
    if (pPool) {
        return pPool;
    }
    for (int i = 0; i < VPSTASH_MAX_POOLS; ++i) {
        if (!s_VpStash.aPools[i].gbUsed) {
            memset(&s_VpStash.aPools[i], 0, sizeof(VPPool));
            s_VpStash.aPools[i].gbUsed = 1;
            strcpy(s_VpStash.aPools[i].szKey, szKey);
            D2BOOT_MarkFmt("shared stash: pool opened (account '%s')", szKey[0] ? szKey : "(legacy single-player)"); // [d2boot]
            return &s_VpStash.aPools[i];
        }
    }
    D2BOOT_Mark("shared stash: pool cache full - toggle refused"); // [d2boot]
    return nullptr;
}

static VPActive * VPSHARED_FindActive(D2UnitStrc * pUnit) {
    for (int i = 0; i < VPSTASH_MAX_ACTIVE; ++i) {
        if (s_VpStash.aActive[i].gbUsed && s_VpStash.aActive[i].pUnit == pUnit) {
            return &s_VpStash.aActive[i];
        }
    }
    return nullptr;
}

static VPActive * VPSHARED_GetActive(D2UnitStrc * pUnit, VPPool * pPool) {
    VPActive * pActive = VPSHARED_FindActive(pUnit);
    if (pActive) {
        return pActive;
    }
    for (int i = 0; i < VPSTASH_MAX_ACTIVE; ++i) {
        if (!s_VpStash.aActive[i].gbUsed) {
            memset(&s_VpStash.aActive[i], 0, sizeof(VPActive));
            s_VpStash.aActive[i].gbUsed = 1;
            s_VpStash.aActive[i].pUnit = pUnit;
            s_VpStash.aActive[i].pPool = pPool;
            return &s_VpStash.aActive[i];
        }
    }
    D2BOOT_Mark("shared stash: activation cache full - toggle refused"); // [d2boot]
    return nullptr;
}

// ---------------------------------------------------------------------------
// [R210] 1.13d D2GS resolution surface. Every number here is pinned against the
// shipping stock binaries - never guessed. Anything still marked TODO is
// guarded with #error so the 1.13d branch cannot compile until it is filled.
// Confirmed so far:
//   D2Common 10518 = STATSEX_GetCurrentStat      (3-arg, ret 0xC)
//   D2Common 10665 = SerializeItem               (6-arg, ret 0x18)
//   Fog      10115 = SFile::GetAndCreateSavePath (exactly 2 callers in the
//            whole DLL: 0x39834 and 0x3bbbb, each a thiscall with
//            (lea ecx,[esp+0x1fc], mov edx,0x400) feeding the "%s%s.d2s"
//            builder - our exact signature)
//   D2Game   RVA 0x0DB780 = SCMD_QueuePacketForClient (pClient in EAX, ret 8)
#if defined(VPSTASH_D2GS_113D)
#ifndef VPSTASH_ORD_STATSEX
#define VPSTASH_ORD_STATSEX          10518
#endif
#ifndef VPSTASH_ORD_SERIALIZE_ITEM
#define VPSTASH_ORD_SERIALIZE_ITEM   10665
#endif
#ifndef VPSTASH_ORD_SAVE_PATH
#define VPSTASH_ORD_SAVE_PATH        10115
#endif
// [R215] The four values below are used ONLY by the S->C 0x9C resync (client
// repaint of the stash grid). Define VPSTASH_113D_NO_0X9C to build a state-only
// realm DLL: items move server-side and persist to the .sss, but the client does
// not repaint until the stash is reopened. That is a legitimate smoke-test mode
// and keeps the build honest while those ordinals are still unpinned.
// CONFIRMED (D2GS agent; byte-verified in D2Common.dll): the item-flag pair sits
// adjacent in the flag API cluster - 10120 @0x421a0 (getter, ret 4) and
// 10876 @0x421c0 (setter, ret 0xC); both open with the item-type assert
// `cmp dword ptr [eax], 4`, so the identification is semantic, not positional.
#ifndef VPSTASH_ORD_GET_ITEM_FLAG
#define VPSTASH_ORD_GET_ITEM_FLAG   10120
#endif
#ifndef VPSTASH_ORD_EDIT_ITEM_FLAG
#define VPSTASH_ORD_EDIT_ITEM_FLAG  10876
#endif
// CONFIRMED by its CONSUMER (the 0x9D builder 0xDCDA0): id and action are written
// into the header, then `call [D2Common 10523]` and its AL return is captured into
// the packet's component byte - i.e. 10523 is the (unit, item) -> category call.
#ifndef VPSTASH_ORD_GET_ITEM_COMPONENT
#define VPSTASH_ORD_GET_ITEM_COMPONENT  10523
#endif
// CONTROL UNIT: read the client's pPlayer field instead of pinning an accessor.
// D2ClientStrc::pPlayer sits at +0x174 in our layout, and the 1.13d evidence
// agrees: the struct at 0x6fc34ff4 carries +0x16C/+0x170/+0x174/+0x1A8 plus a
// +0x3D4 flag byte and reads/writes +0x174 as a unit reference. A wrong offset
// cannot silently corrupt anything here because we validate the unit type
// (eUnitType == UNIT_PLAYER at +0x00, confirmed on 1.13d) before use.
#define VPSTASH_113D_CLIENT_PLAYER_OFFSET 0x174
#endif // VPSTASH_D2GS_113D

#if defined(VPSTASH_D2GS_113D)
// Control unit for a client: field read + type validation (see the offset note
// above). Returns nullptr when the field is absent or is not a player unit, so a
// layout surprise degrades to "no category" rather than a bad pointer use.
static D2UnitStrc * VPSHARED_GetControlUnit113d(D2ClientStrc * pClient) {
    if (!pClient) {
        return nullptr;
    }
    D2UnitStrc * pUnit = *(D2UnitStrc **)((char *)pClient + VPSTASH_113D_CLIENT_PLAYER_OFFSET);
    if (!pUnit || pUnit->eUnitType != UNIT_PLAYER) {
        static int s_nWarned = 0;
        if (!s_nWarned) {
            s_nWarned = 1;
            D2BOOT_MarkFmt("shared stash [1.13d]: client+0x174 is not a player unit (type=%d) - category will be 0",
                           pUnit ? (int)pUnit->eUnitType : -1); // [d2boot]
        }
        return nullptr;
    }
    return pUnit;
}

// Resolve one D2Common/Fog ordinal once and cache it.
static void * VPSHARED_ResolveOrdinal113d(uint16_t nOrd, void ** ppCache, const char * szDll) {
    if (!*ppCache) {
        HMODULE hMod = GetModuleHandleA(szDll);
        if (!hMod) {
            return nullptr;
        }
        *ppCache = (void *)GetProcAddress(hMod, (LPCSTR)(uintptr_t)nOrd);
    }
    return *ppCache;
}

// SFile::GetAndCreateSavePath - the stock call sites are
//   mov edx,0x400 ; lea ecx,[esp+0x1fc] ; call [Fog slot]
// i.e. __fastcall: p1 = ECX = the out buffer, p2 = EDX = size. Both of its
// only two callers in the DLL (0x39834, 0x3bbbb) have exactly that shape.
static void VPSHARED_GetSaveDir113d(char * szOut, int nSize) {
    static void * s_pfnSavePath = nullptr;
    void * pfn = VPSHARED_ResolveOrdinal113d(VPSTASH_ORD_SAVE_PATH, &s_pfnSavePath, "Fog.dll");
    szOut[0] = '\0';
    if (!pfn) {
        D2BOOT_Mark("shared stash [1.13d]: GetAndCreateSavePath unresolved - pool path empty"); // [d2boot]
        return;
    }
    __asm__ __volatile__(
        "mov  %[out], %%ecx\n\t"
        "mov  %[n], %%edx\n\t"
        "call *%[fn]\n\t"
        :
        : [out] "r"(szOut), [n] "r"(nSize), [fn] "m"(pfn)
        : "eax", "ecx", "edx", "memory");
}
// CONFIRMED (D2GS agent; byte-verified in D2Common.dll): 11040 =
// INVENTORY_GetFirstItem (validates the 0x01020304 inventory magic at fn+8),
// 10897 = INVENTORY_GetNextItem (null-check only). The stock page-walk loop is
// GetFirstItem -> GetNextItem -> ITEM_GetContainingInventoryPage(10445), i.e.
// 1.13d iteration yields ITEMS directly (no node -> carried-item step).
#ifndef VPSTASH_ORD_INV_GET_FIRST
#define VPSTASH_ORD_INV_GET_FIRST  11040
#endif
#ifndef VPSTASH_ORD_INV_GET_NEXT
#define VPSTASH_ORD_INV_GET_NEXT   10897
#endif
// CONFIRMED (D2GS agent; byte-verified): 10903 = INVENTORY remove (pInventory,
// pItem, ret 8). Its body searches the slot (0x6fd89940), asserts the unit is an
// item ([unit]==4), bounds the slot <16, then drives the slot engine
// (0x6fd8b110) with mode 1 - place uses mode 0. NOT 10888, which is
// INVENTORY_Create (allocates 0x40 bytes and writes the 0x01020304 magic).
#ifndef VPSTASH_ORD_INV_REMOVE
#define VPSTASH_ORD_INV_REMOVE     10903
#endif
#endif // VPSTASH_D2GS_113D

#if defined(VPSTASH_D2GS_113D)
// [R218] Import-free helpers. A loadable d2vpstash.dll must not import
// D2Common/D2Game, so these resolve the two already-pinned ordinals at runtime
// instead of linking against them.
#ifndef VPSTASH_ORD_ITEM_PAGE
#define VPSTASH_ORD_ITEM_PAGE 10445
#endif
static int VPSHARED_ItemPage113d(D2UnitStrc * pItem) {            // D2Common 10445
    static void * s_pfn = nullptr;
    void * pfn = VPSHARED_ResolveOrdinal113d(VPSTASH_ORD_ITEM_PAGE, &s_pfn, "D2Common.dll");
    return pfn ? (int)((int(__stdcall*)(D2UnitStrc*))pfn)(pItem) : -1;
}
static int VPSHARED_SerializeItem113d(D2UnitStrc * pItem, uint8_t * pOut, uint32_t nMax) { // 10665
    static void * s_pfn = nullptr;
    void * pfn = VPSHARED_ResolveOrdinal113d(VPSTASH_ORD_SERIALIZE_ITEM, &s_pfn, "D2Common.dll");
    if (!pfn) {
        return -1;
    }
    return (int)((uint32_t(__stdcall*)(D2UnitStrc*, uint8_t*, uint32_t, void*, int, int))pfn)(
        pItem, pOut, nMax, nullptr, 0, 0);
}
#endif // VPSTASH_D2GS_113D

// Page-iteration adapters: same call shape on both builds, so the two loops in
// this file stay identical. On 1.13d the iteration already yields the item.
static D2UnitStrc * VPSHARED_IterFirst(void * pInventory) {
#if defined(VPSTASH_D2GS_113D)
    static void * s_pfn = nullptr;
    void * pfn = VPSHARED_ResolveOrdinal113d(VPSTASH_ORD_INV_GET_FIRST, &s_pfn, "D2Common.dll");
    return pfn ? ((D2UnitStrc *(__stdcall*)(void*))pfn)(pInventory) : nullptr;
#else
    return D2Common::INVENTORY::Inventory::INVENTORY_GetFirstItem((D2InventoryStrc *)pInventory);
#endif
}

static D2UnitStrc * VPSHARED_IterNext(D2UnitStrc * pNode) {
#if defined(VPSTASH_D2GS_113D)
    static void * s_pfn = nullptr;
    void * pfn = VPSHARED_ResolveOrdinal113d(VPSTASH_ORD_INV_GET_NEXT, &s_pfn, "D2Common.dll");
    return pfn ? ((D2UnitStrc *(__stdcall*)(D2UnitStrc*))pfn)(pNode) : nullptr;
#else
    return D2Common::INVENTORY::Inventory::INVENTORY_GetNextItem(pNode);
#endif
}

// The inventory node -> item step exists only in the 1.14d shape.
static D2UnitStrc * VPSHARED_IterItem(D2UnitStrc * pNode) {
#if defined(VPSTASH_D2GS_113D)
    return pNode;
#else
    return D2Common::INVENTORY::Inventory::GetCarriedItem(pNode);
#endif
}

// ---------------------------------------------------------------------------
// File layer (per pool)

static void VPSHARED_BuildFilePath(char * szOut, size_t nSize, const char * szKey) {
    char szSaveDir[1024];
    szOut[0] = '\0';
#if defined(VPSTASH_D2GS_113D)
    VPSHARED_GetSaveDir113d(szSaveDir, 0x400);
#else
    Fog::Source::SFile::GetAndCreateSavePath(szSaveDir, 0x400);
#endif
    if (szKey[0]) {
        sprintf(szOut, "%s_d2vp_shared_%s.sss", szSaveDir, szKey);
    } else {
        sprintf(szOut, "%s_d2vp_shared.sss", szSaveDir); // legacy single-player global file
    }
}

// Additive byte-sum, the .d2s checksum convention.
static uint32_t VPSHADOW_BlockChecksum(const uint8_t * pBytes, uint32_t nSize) {
    uint32_t dwSum = 0;
    for (uint32_t i = 0; i < nSize; ++i) {
        dwSum += pBytes[i];
    }
    return dwSum;
}

// Read + validate one 'JM' page block out of an open file at dwFileOffset.
// dwBlockSize / dwChecksum come from the file header. On success the page's
// block/size are stored (a < 4-byte record is a legitimately empty page and
// stores null/0); on any failure the page starts empty and a mark says why -
// the file on disk is never touched by a misread. Returns 1 when the page
// ended stored-or-empty-by-design, 0 on real corruption (the caller uses a
// page-1 failure to chase the .bak generation, v1 behavior).
static int VPSHARED_IngestPageBlock(VPPool * pPool, HANDLE hFile, int nPage, uint32_t dwBlockSize,
                                    uint32_t dwChecksum, uint32_t dwFileOffset, uint32_t dwFileSize) {
    if (pPool->apSharedBlock[nPage]) {
        VPSHARED_Free(pPool->apSharedBlock[nPage]);
        pPool->apSharedBlock[nPage] = nullptr;
    }
    pPool->anSharedSize[nPage] = 0;

    if (dwFileOffset > dwFileSize || dwBlockSize > dwFileSize - dwFileOffset || dwBlockSize > VPSTASH_BLOCK_CAP) {
        D2BOOT_Mark("shared stash: bad header (size) - starting empty"); // [d2boot]
        return 0;
    }
    if (dwBlockSize < 4) {
        // Below the 'JM'+count minimum: an empty page record - nothing to load.
        return 1;
    }

    uint8_t * pBlock = (uint8_t *)VPSHARED_Alloc(VPSTASH_BLOCK_CAP);
    if (!pBlock) {
        D2BOOT_Mark("shared stash: read failed - starting empty"); // [d2boot]
        return 0;
    }

    LARGE_INTEGER liAt;
    liAt.QuadPart = (int64_t)dwFileOffset;
    DWORD dwGot = 0; // DWORD, not uint32_t: ReadFile's out-param is LPDWORD (win32 'long'), mingw32 rejects unsigned int*
    if (!SetFilePointerEx(hFile, liAt, nullptr, FILE_BEGIN) ||
        !ReadFile(hFile, pBlock, dwBlockSize, &dwGot, nullptr) || dwGot != dwBlockSize) {
        VPSHARED_Free(pBlock);
        D2BOOT_Mark("shared stash: read failed - starting empty"); // [d2boot]
        return 0;
    }
    if (VPSHADOW_BlockChecksum(pBlock, dwBlockSize) != dwChecksum) {
        VPSHARED_Free(pBlock);
        D2BOOT_Mark("shared stash: checksum fail - file - starting empty"); // [d2boot]
        return 0;
    }
    if (pBlock[0] != 'J' || pBlock[1] != 'M') {
        VPSHARED_Free(pBlock);
        D2BOOT_Mark("shared stash: bad item block header - starting empty"); // [d2boot]
        return 0;
    }

    uint16_t nCount = (uint16_t)(pBlock[2] | (pBlock[3] << 8));
    if (nCount > VPSTASH_PAGE_ITEM_CAP) {
        // Clamp in memory only - the on-disk file stays verbatim.
        D2BOOT_MarkFmt("shared stash: item count %d out of range - clamped", (int)nCount); // [d2boot]
        pBlock[2] = (uint8_t)VPSTASH_PAGE_ITEM_CAP;
        pBlock[3] = 0;
    }
    pPool->apSharedBlock[nPage] = pBlock;
    pPool->anSharedSize[nPage] = dwBlockSize;
    return 1;
}

// Items-on-page helper for the load marks (0 when the page is empty).
static int VPSHARED_PageItemCount(VPPool * pPool, int nPage) {
    if (nPage < 0 || nPage >= VPSTASH_PAGES || !pPool->apSharedBlock[nPage] || pPool->anSharedSize[nPage] < 4) {
        return 0;
    }
    return (int)(uint16_t)(pPool->apSharedBlock[nPage][2] | (pPool->apSharedBlock[nPage][3] << 8));
}

// Validate + ingest one file (path given). Returns 1 with the shared
// blocks/gold filled (stored), 0 with a benign verdict (no file, or corrupt -
// a re-read cannot change it; the file on disk is never touched), -1 on a
// TRANSIENT failure (the file exists but could not be opened - a lock held
// by an AV scan or a sync client; a retry can succeed). [ROB-01] A stored
// read fills the pages the file carries (a pre-R82 file: pages 1-2; pages
// 3..100 stay empty; an R82-R145 ten-page file: pages 1-10, 11..100 empty);
// a v1 file fills page 1 - or 0 to start empty (file left untouched on
// disk; the caller then retries the .bak generation).
static int VPSHARED_TryReadFile(VPPool * pPool, const char * szPath) {
    HANDLE hFile = CreateFileA(szPath, GENERIC_READ, FILE_SHARE_READ, nullptr,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        int nErr = (int)GetLastError();
        if (nErr == ERROR_FILE_NOT_FOUND || nErr == ERROR_PATH_NOT_FOUND) {
            D2BOOT_Mark("shared stash: no file - starting empty"); // [d2boot]
            return 0;
        }
        // The file EXISTS but could not be opened - a sharing/lock violation
        // (AV scan, backup or sync client holding it). Transient: the caller
        // defers instead of pinning an empty session state, and nothing may
        // be saved until a read succeeds. [ROB-01]
        D2BOOT_MarkFmt("shared stash: open failed (%d) - file unreadable", nErr); // [d2boot]
        return -1;
    }

    uint8_t aHeader[VPSTASH_HEADER_SIZE_V2];
    DWORD dwRead = 0;
    int nOk = 0;
    // Every shape shares the first 20 bytes; a v2 file carries one more
    // size/checksum pair per page beyond the first, read after the shape
    // check below.
    if (ReadFile(hFile, aHeader, VPSTASH_HEADER_SIZE_V1, &dwRead, nullptr) && dwRead == VPSTASH_HEADER_SIZE_V1) {
        const uint8_t aMagicV1[4] = { 'V', 'P', 'S', '1' };
        const uint8_t aMagicV2[4] = { 'V', 'P', 'S', '2' };
        uint16_t nVersion = (uint16_t)(aHeader[4] | (aHeader[5] << 8));
        uint16_t nPages = (uint16_t)(aHeader[6] | (aHeader[7] << 8));
        uint32_t dwGold = *(uint32_t *)&aHeader[8];
        // v2 kept its magic + version across the R82 ten-page widening and
        // the R146 hundred-page one: the page count is accepted at any width
        // this build can hold (1..100), so pre-R82 files (2 pages) and
        // R82-R145 files (10 pages) load and the pages they don't carry
        // start empty (BSS zero). A file written by a WIDER build
        // (pageCount > 100) is rejected whole - it needs a newer binary.
        int nIsV1 = memcmp(aHeader, aMagicV1, 4) == 0 && nVersion == VPSTASH_FILE_VERSION_V1 && nPages == VPSTASH_PAGE_COUNT_V1;
        int nIsV2 = memcmp(aHeader, aMagicV2, 4) == 0 && nVersion == VPSTASH_FILE_VERSION
                    && nPages >= 1 && nPages <= VPSTASH_PAGE_COUNT;
        uint32_t dwHeaderSize = 12 + 8 * (uint32_t)nPages; // v1's 1 page -> the 20-byte header
        LARGE_INTEGER liSize;
        uint32_t dwFileSize = (GetFileSizeEx(hFile, &liSize) && liSize.QuadPart >= 0 && liSize.QuadPart <= 0xffffffffll)
                                  ? (uint32_t)liSize.QuadPart : 0xffffffffu;
        if (!nIsV1 && !nIsV2) {
            D2BOOT_Mark("shared stash: bad header (magic/version) - starting empty"); // [d2boot]
        } else if (nIsV2 && !(ReadFile(hFile, aHeader + VPSTASH_HEADER_SIZE_V1,
                                       dwHeaderSize - VPSTASH_HEADER_SIZE_V1, &dwRead, nullptr) &&
                             dwRead == dwHeaderSize - VPSTASH_HEADER_SIZE_V1)) {
            D2BOOT_Mark("shared stash: header read failed - starting empty"); // [d2boot]
        } else {
            pPool->dwSharedGold = dwGold;
            // One ingest pass serves every shape: page i's size/checksum
            // record lives at header[12+8i]/[16+8i] and its block right
            // behind the previous page's. v1 is literally the 1-page shape
            // (page 0 at offset 20); a v2 file's pages chain from its own
            // header size.
            uint32_t dwSize0 = *(uint32_t *)&aHeader[12];
            if (dwHeaderSize > dwFileSize || dwSize0 > dwFileSize - dwHeaderSize) {
                // Validate page-1's extent HERE so a later page's offset
                // below cannot run past the file (IngestPageBlock re-checks
                // every page against the same bounds).
                D2BOOT_Mark("shared stash: bad header (size) - starting empty"); // [d2boot]
            } else {
                int nPage0Ok = 1;
                uint32_t dwOff = dwHeaderSize;
                for (int i = 0; i < (int)nPages; ++i) {
                    int nOkI = VPSHARED_IngestPageBlock(pPool, hFile, i, *(uint32_t *)&aHeader[12 + 8 * i],
                                                        *(uint32_t *)&aHeader[16 + 8 * i], dwOff, dwFileSize);
                    if (i == 0) {
                        // Page 1's verdict drives the .bak chase (v1
                        // behavior); a corrupt later page degrades to an
                        // empty page only.
                        nPage0Ok = nOkI;
                    }
                    dwOff += *(uint32_t *)&aHeader[12 + 8 * i];
                }
                nOk = nPage0Ok;
            }
            int nTotalItems = 0;
            for (int i = 0; i < (int)nPages; ++i) {
                nTotalItems += VPSHARED_PageItemCount(pPool, i);
            }
            if (nIsV1) {
                D2BOOT_MarkFmt("shared stash: loaded %d items (gold %d) (v1 file)",
                               nTotalItems, (int)dwGold); // [d2boot]
            } else {
                if (nPages < VPSTASH_PAGE_COUNT) {
                    D2BOOT_MarkFmt("shared stash: v2 file has %d pages - pages %d..%d start empty (narrower file)",
                                   (int)nPages, (int)nPages + 1, (int)VPSTASH_PAGE_COUNT); // [d2boot]
                }
                D2BOOT_MarkFmt("shared stash: loaded %d items in %d pages (gold %d)",
                               nTotalItems, (int)nPages, (int)dwGold); // [d2boot]
            }
        }
    } else {
        D2BOOT_Mark("shared stash: header read failed - starting empty"); // [d2boot]
    }

    CloseHandle(hFile);
    return nOk;
}

// Lazy read of the pool's shared file. [ROB-01] Three verdicts: a success
// pins the loaded state; a benign failure (no file anywhere, or corrupt
// primary whose .bak also benign-fails) pins the empty state - a re-read
// cannot change it, the on-disk file is never touched by a misread, and the
// owner can inspect it; a TRANSIENT failure (file exists but
// locked/unreadable) pins NOTHING - gbLoaded stays 0 so the next
// toggle retries, and until a read succeeds neither a toggle nor a save
// runs, so a transient lock can never flush an empty session state over the
// real file.
static void VPSHARED_EnsureLoaded(VPPool * pPool) {
    if (pPool->gbLoaded) {
        return;
    }

    char szPath[1100];
    VPSHARED_BuildFilePath(szPath, sizeof(szPath), pPool->szKey);
    int nPrimary = VPSHARED_TryReadFile(pPool, szPath);
    if (nPrimary > 0) {
        pPool->gbLoaded = 1;
        pPool->gbLoadFailed = 0;
        return;
    }
    if (nPrimary < 0) {
        pPool->gbLoadFailed = 1;
        D2BOOT_Mark("shared stash: unreadable - load deferred; toggles and saves refused until a read succeeds"); // [d2boot]
        return;
    }
    // Primary benign (absent or corrupt): retry once with the .bak generation.
    char szBak[1100];
    sprintf(szBak, "%s.bak", szPath);
    int nBak = VPSHARED_TryReadFile(pPool, szBak);
    if (nBak > 0) {
        pPool->gbLoaded = 1;
        pPool->gbLoadFailed = 0;
        return;
    }
    if (nBak < 0) {
        pPool->gbLoadFailed = 1;
        D2BOOT_Mark("shared stash: unreadable - load deferred; toggles and saves refused until a read succeeds"); // [d2boot]
        return;
    }
    // No file anywhere (the normal first run) or both generations corrupt:
    // the empty state IS the truth - pin it, saves may run.
    pPool->gbLoaded = 1;
    pPool->gbLoadFailed = 0;
}

// Atomic write: full temp file, one-generation .bak of the old file, then
// MoveFileExA replace. Any failure leaves the old file + .bak untouched and
// keeps the session memory copy (mark says which).
// [ROB-01] Second lock of the transient-read hole: if the load never
// succeeded this game, the in-memory shared state may be an empty shell -
// refuse to serialize it over a file this build could not read. (Unreachable
// through the toggle path, which refuses while gbLoadFailed; kept as the
// belt to the flush-before-save suspenders.)
static void VPSHARED_WriteFileLocked(VPPool * pPool) {
    if (pPool->gbLoadFailed) {
        D2BOOT_Mark("shared stash: save refused - last load failed (disk untouched)"); // [d2boot]
        return;
    }

    char szPath[1100];
    VPSHARED_BuildFilePath(szPath, sizeof(szPath), pPool->szKey);

    uint32_t aSizes[VPSTASH_PAGES];
    uint32_t dwTotal = VPSTASH_HEADER_SIZE_V2;
    for (int i = 0; i < VPSTASH_PAGES; ++i) {
        aSizes[i] = pPool->anSharedSize[i];
        dwTotal += aSizes[i];
    }

    uint8_t * pOut = (uint8_t *)VPSHARED_Alloc(dwTotal);
    if (!pOut) {
        D2BOOT_Mark("shared stash: write failed (alloc) - keeping memory copy"); // [d2boot]
        return;
    }

    pOut[0] = 'V'; pOut[1] = 'P'; pOut[2] = 'S'; pOut[3] = '2';
    pOut[4] = (uint8_t)(VPSTASH_FILE_VERSION & 0xff);
    pOut[5] = (uint8_t)(VPSTASH_FILE_VERSION >> 8);
    pOut[6] = (uint8_t)(VPSTASH_PAGE_COUNT & 0xff);
    pOut[7] = (uint8_t)(VPSTASH_PAGE_COUNT >> 8);
    *(uint32_t *)&pOut[8] = pPool->dwSharedGold;
    uint32_t dwAt = VPSTASH_HEADER_SIZE_V2;
    for (int i = 0; i < VPSTASH_PAGES; ++i) {
        *(uint32_t *)&pOut[12 + 8 * i] = aSizes[i];
        *(uint32_t *)&pOut[16 + 8 * i] = aSizes[i] ? VPSHADOW_BlockChecksum(pPool->apSharedBlock[i], aSizes[i]) : 0;
        if (aSizes[i]) {
            memcpy(pOut + dwAt, pPool->apSharedBlock[i], aSizes[i]);
        }
        dwAt += aSizes[i];
    }

    char szTmp[1100];
    char szBak[1100];
    sprintf(szTmp, "%s.tmp", szPath);
    sprintf(szBak, "%s.bak", szPath);

    HANDLE hFile = CreateFileA(szTmp, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        D2BOOT_MarkFmt("shared stash: write failed (%d) - keeping memory copy", (int)GetLastError()); // [d2boot]
        VPSHARED_Free(pOut);
        return;
    }

    DWORD dwWritten = 0;
    int nFailed = (!WriteFile(hFile, pOut, dwTotal, &dwWritten, nullptr) || dwWritten != dwTotal);
    FlushFileBuffers(hFile);
    CloseHandle(hFile);
    VPSHARED_Free(pOut);
    if (nFailed) {
        D2BOOT_MarkFmt("shared stash: write failed (%d) - keeping memory copy", (int)GetLastError()); // [d2boot]
        return;
    }

    // One-generation .bak of the previous file (best effort - a missing
    // previous file is the normal first-save case).
    CopyFileA(szPath, szBak, FALSE);
    if (!MoveFileExA(szTmp, szPath, MOVEFILE_REPLACE_EXISTING)) {
        D2BOOT_MarkFmt("shared stash: write failed (rename %d) - keeping memory copy", (int)GetLastError()); // [d2boot]
        return;
    }

    D2BOOT_MarkFmt("shared stash: saved '%s' (%d bytes, gold %d)",
                   pPool->szKey[0] ? pPool->szKey : "(legacy)", (int)dwTotal, (int)pPool->dwSharedGold); // [d2boot]
}

// ---------------------------------------------------------------------------
// Page-4 serialization / dematerialization / materialization

// Serialize the player's current INVPAGE_STASH set into a fresh pool buffer
// using the retail per-item encoder (the .d2s item-block bytes). Returns 1
// with (*ppOut, *pnSize) set (null/0 for an empty page), 0 on failure.
static int VPSHARED_SerializePage4(::SGAMEDATA * pGame, D2UnitStrc * pUnit, uint8_t ** ppOut, uint32_t * pnSize) {
    *ppOut = nullptr;
    *pnSize = 0;
    if (!pUnit || !pUnit->pInventory) {
        return 1;
    }

    uint8_t * pBlock = (uint8_t *)VPSHARED_Alloc(VPSTASH_BLOCK_CAP);
    if (!pBlock) {
        D2BOOT_Mark("shared stash: page serialize failed (alloc)"); // [d2boot]
        return 0;
    }

    pBlock[0] = 'J';
    pBlock[1] = 'M';
    uint16_t nCount = 0;
    uint8_t * pCursor = pBlock + 4;
    uint8_t * pEnd = pBlock + VPSTASH_BLOCK_CAP;

    for (D2UnitStrc * pNode = VPSHARED_IterFirst(pUnit->pInventory); pNode;
         pNode = VPSHARED_IterNext(pNode)) {
        D2UnitStrc * pCarried = VPSHARED_IterItem(pNode);
        if (!pCarried || pCarried->eUnitType != UNIT_ITEM) {
            continue;
        }
        #if defined(VPSTASH_D2GS_113D)
        if (VPSHARED_ItemPage113d(pCarried) != (int)INVPAGE_STASH) {
#else
        if (D2Common::ITEMS::Items::ITEM_GetContainingInventoryPage(pCarried) != INVPAGE_STASH) {
#endif
            continue;
        }
        if (nCount >= VPSTASH_PAGE_ITEM_CAP) {
            D2BOOT_Mark("shared stash: page item cap reached - clamped"); // [d2boot]
            break;
        }
        // Retail per-item encoder, same call shape as the .d2s writer
        // (SAVE_SerializeInventoryItems): own statlist marker, all items.
#if defined(VPSTASH_D2GS_113D)
        int nWritten = VPSHARED_SerializeItem113d(pCarried, pCursor, (uint32_t)(pEnd - pCursor));
#else
        int nWritten = D2Common::ITEMS::Items::SerializeItem(pCarried, pCursor, (size_t)(pEnd - pCursor),
                                                             (D2StatListExStrc *)0x1, 1, 0);
#endif
        if (nWritten < 0) {
            D2BOOT_Mark("shared stash: page serialize failed (encoder)"); // [d2boot]
            VPSHARED_Free(pBlock);
            return 0;
        }
        pCursor += nWritten;
        ++nCount;
    }

    pBlock[2] = (uint8_t)(nCount & 0xff);
    pBlock[3] = (uint8_t)(nCount >> 8);
    *ppOut = pBlock;
    *pnSize = (uint32_t)(pCursor - pBlock);
    return 1;
}

// Snapshot the page-4 item units into a fixed array (max VPSTASH_PAGE_ITEM_CAP).
static int VPSHARED_CollectPage4(D2UnitStrc * pUnit, D2UnitStrc ** apItems, int nMax) {
    int nItems = 0;
    if (!pUnit || !pUnit->pInventory) {
        return 0;
    }
    for (D2UnitStrc * pNode = VPSHARED_IterFirst(pUnit->pInventory); pNode;
         pNode = VPSHARED_IterNext(pNode)) {
        D2UnitStrc * pCarried = VPSHARED_IterItem(pNode);
        if (!pCarried || pCarried->eUnitType != UNIT_ITEM) {
            continue;
        }
        #if defined(VPSTASH_D2GS_113D)
        if (VPSHARED_ItemPage113d(pCarried) != (int)INVPAGE_STASH) {
#else
        if (D2Common::ITEMS::Items::ITEM_GetContainingInventoryPage(pCarried) != INVPAGE_STASH) {
#endif
            continue;
        }
        if (nItems >= nMax) {
            break;
        }
        apItems[nItems++] = pCarried;
    }
    return nItems;
}

// Dematerialize: detach + free every page-4 unit. The removal helper also
// sends the client 0x9D remove packets, so the grid empties on screen.
// Set when a gated 1.13d removal did not actually shrink the stash page - the
// caller refuses the toggle rather than proceeding on unverified state.
static int s_nVpRemoveFailed = 0;

static void VPSHARED_DematerializePage4(::SGAMEDATA * pGame, D2UnitStrc * pUnit) {
    D2UnitStrc * apItems[VPSTASH_PAGE_ITEM_CAP];
    int nItems = VPSHARED_CollectPage4(pUnit, apItems, VPSTASH_PAGE_ITEM_CAP);
    s_nVpRemoveFailed = 0;
#if defined(VPSTASH_D2GS_113D)
    static void * s_pfnRemove = nullptr;
    void * pfnRemove = VPSHARED_ResolveOrdinal113d(VPSTASH_ORD_INV_REMOVE, &s_pfnRemove, "D2Common.dll");
    if (!pfnRemove) {
        D2BOOT_Mark("shared stash [1.13d]: INVENTORY remove unresolved - toggle refused"); // [d2boot]
        s_nVpRemoveFailed = 1;
        return;
    }
    for (int i = 0; i < nItems; ++i) {
        if (!pUnit->pInventory) {
            s_nVpRemoveFailed = 1;
            break;
        }
        ((int(__stdcall*)(void*, D2UnitStrc*))pfnRemove)((void*)pUnit->pInventory, apItems[i]);
    }
    // RUNTIME GUARD: a wrong/blocked removal must never be mistaken for success -
    // it would leave items in place while the pool assumes they left (dupe). If
    // the page did not shrink, refuse the whole toggle.
    if (!s_nVpRemoveFailed) {
        int nAfter = VPSHARED_CollectPage4(pUnit, apItems, VPSTASH_PAGE_ITEM_CAP);
        if (nAfter >= nItems && nItems > 0) {
            D2BOOT_MarkFmt("shared stash [1.13d]: remove left %d of %d items - toggle refused", nAfter, nItems); // [d2boot]
            s_nVpRemoveFailed = 1;
        }
    }
#else
    for (int i = 0; i < nItems; ++i) {
        // Frees the unit (ITEM_DeleteItemAndFreeUnit) and notifies the client.
        D2Game::ITEMS::ItemMode::ITEM_RemoveItemFromCurrentLocation(pGame, pUnit, apItems[i]);
    }
#endif
}

#if defined(VPSTASH_D2GS_113D)
// ===========================================================================
// 1.13d D2GS realm build - resolution plumbing for the stock binaries.
// "Never guess a number": every value below is either confirmed by the D2GS
// agent's disassembly or marked TODO and must be filled before this branch
// compiles into a shippable DLL.
// ---------------------------------------------------------------------------
// Confirmed   : D2Common 10665 = SerializeItem (6-arg, ret 0x18)
// Confirmed   : D2Common 10518 = STATSEX_GetCurrentStat (3-arg, ret 0xC)
// Confirmed   : D2Game  RVA 0x0DB780 = SCMD_QueuePacketForClient
//               (pClient in EAX, [esp+4]=size, [esp+8]=buf, ret 8)
// TODO(agent) : D2Common ordinals for ITEM_EditItemData_eItemFlag and
//               ITEM_GetItemData_eD2ItemFlag
// TODO(agent) : D2Common ordinal for Units::GetItemComponent (feeds +0x03)
// TODO(agent) : D2Game RVA for Clients::CLIENTS_GetControlUnit
// SETTLED     : the 1.13d 0x9C header is the same 8-byte shape as our own
//               D2GSPacketSrv0x9C (SCmd.h:259 - the struct our client parses):
//               +0 id, +1 action/page, +2 TOTAL size, +3 category,
//               +4 GUID (dword), +8 bit stream. The earlier 11-byte reading
//               was a field-order misremembering, not a version difference.
// (Ordinal/RVA defines and the ordinal resolver live in the earlier R210
//  block above the file layer, so the save-path call can use them.)
// The bit stream starts right behind the 8-byte header (matches both our
// packet struct and the stock builder's buf+8).
#ifndef VPSTASH_113D_9C_BODY
#define VPSTASH_113D_9C_BODY 8
#endif
static const uintptr_t VPSTASH_113D_D2GAME_RVA_QUEUE = 0x0DB780;

// The queue function takes pClient in EAX (ret 8), so a plain C pointer
// cannot express it. Minimal shim: push buf, push size, load EAX, call.
static void VPSHARED_QueuePacket113d(void * pClient, uint32_t nSize, const void * pBuf) {
    static void * s_pfnQueue = nullptr;
    static int s_nQueueResolved = 0;
    if (!s_nQueueResolved) {
        uintptr_t nBase = (uintptr_t)GetModuleHandleA("D2Game.dll");
        s_pfnQueue = (void *)(nBase + VPSTASH_113D_D2GAME_RVA_QUEUE);
        s_nQueueResolved = 1;
        D2BOOT_MarkFmt("shared stash [1.13d]: queue fn resolved base=%p fn=%p", (void*)nBase, s_pfnQueue); // [d2boot]
    }
    if (!s_pfnQueue) {
        return;
    }
    __asm__ __volatile__(
        "push %[buf]\n\t"
        "push %[size]\n\t"
        "mov  %[client], %%eax\n\t"
        "call *%[fn]\n\t"
        :
        : [buf] "m"(pBuf), [size] "m"(nSize), [client] "r"(pClient), [fn] "m"(s_pfnQueue)
        : "eax", "ecx", "edx", "memory");
}
#endif // VPSTASH_D2GS_113D

// [R206] Single version-sensitive door for the S->C 0x9C "put in container"
// emission. In this (1.14d) tree the retail call takes an item-flag mask; the
// D2GS agent's disassembly of the stock 1.13d path shows it never builds a
// flag word at all - it emits [0x9C][page][size][item bytes] and toggles the
// item flag with two ITEM_EditItemData_eItemFlag calls instead. A realm
// server build (hook DLL against stock 1.13d D2Game) must therefore REPLACE
// this body with the 1.13d sequence; keeping it to one function means the
// port is a single-site change and the retrieval logic above stays untouched.
static void VPSHARED_SendPutInContainer(D2ClientStrc * pClient, D2UnitStrc * pUnit, D2UnitStrc * pItem) {
#if defined(VPSTASH_D2GS_113D) && defined(VPSTASH_113D_NO_0X9C)
    // State-only realm build: no client repaint (see VPSTASH_113D_NO_0X9C).
    D2BOOT_Mark("shared stash [1.13d]: 0x9C resync disabled (state-only build)"); // [d2boot]
    (void)pClient; (void)pUnit; (void)pItem;
#elif defined(VPSTASH_D2GS_113D)
    // --- 1.13d D2GS realm build -------------------------------------------
    // Confirmed against the stock 1.13d binaries by the D2GS agent:
    //   SerializeItem           = D2Common ordinal 10665 (6-arg, ret 0x18)
    //   EditItemData_eItemFlag  = D2Common ordinal VPSTASH_ORD_EDIT_FLAG  (TODO)
    //   QueuePacketForClient    = D2Game RVA 0x0DB780 (pClient in EAX, ret 8)
    // The wire shape is our own D2GSPacketSrv0x9C (8-byte header, body at +8).
    D2GSPacketSrv0x9C sPacket;
    memset(&sPacket, 0, sizeof(sPacket));
    static void * s_pfnSerialize = nullptr;
    static void * s_pfnEditFlag = nullptr;
    static void * s_pfnGetFlag = nullptr;
    static void * s_pfnGetComponent = nullptr;
    void * pfnSerialize = VPSHARED_ResolveOrdinal113d(VPSTASH_ORD_SERIALIZE_ITEM, &s_pfnSerialize, "D2Common.dll");
    void * pfnEditFlag = VPSHARED_ResolveOrdinal113d(VPSTASH_ORD_EDIT_ITEM_FLAG, &s_pfnEditFlag, "D2Common.dll");
    void * pfnGetFlag = VPSHARED_ResolveOrdinal113d(VPSTASH_ORD_GET_ITEM_FLAG, &s_pfnGetFlag, "D2Common.dll");
    void * pfnGetComponent = VPSHARED_ResolveOrdinal113d(VPSTASH_ORD_GET_ITEM_COMPONENT, &s_pfnGetComponent, "D2Common.dll");
    if (!pfnSerialize || !pfnEditFlag || !pfnGetFlag || !pfnGetComponent) {
        D2BOOT_Mark("shared stash [1.13d]: 0x9C ordinal resolution failed - item not pushed"); // [d2boot]
        return;
    }

    D2UnitStrc * pOwner = VPSHARED_GetControlUnit113d(pClient);
    sPacket.nPacketId = 0x9C;
    sPacket.nAction = (uint8_t)INVPAGE_STASH;   // page 4 = stash
    sPacket.nCategory = (uint8_t)((int(__stdcall*)(D2UnitStrc*, D2UnitStrc*))pfnGetComponent)(pOwner, pItem);
    sPacket.nItemGUID = pItem ? pItem->nUnitGUID : -1;

    // Flag dance exactly as stock does it (set presentation state, serialize,
    // restore the saved state).
    uint32_t nSavedFlags = ((uint32_t(__stdcall*)(D2UnitStrc*))pfnGetFlag)(pItem);
    ((void(__stdcall*)(D2UnitStrc*, uint32_t, int))pfnEditFlag)(pItem, 0xFFFFFFFFu, 0);
    uint32_t nBits = ((uint32_t(__stdcall*)(D2UnitStrc*, uint8_t*, uint32_t, void*, int, int))pfnSerialize)(
                         pItem, (uint8_t*)sPacket.aBitStream, 0xF4, nullptr, 0, 0);
    ((void(__stdcall*)(D2UnitStrc*, uint32_t, int))pfnEditFlag)(pItem, nSavedFlags, 1);

    // Size is the TOTAL (header + payload) and must be written last.
    sPacket.nFullPacketSize = (uint8_t)(VPSTASH_113D_9C_BODY + nBits);
    // v1 deliberately omits Send0x9DAllSocketedItems: it only re-pushes socket
    // filler icons, the stream is already correct, and skipping it avoids one
    // more unverified RVA on the first live test.
    VPSHARED_QueuePacket113d(pClient, (uint32_t)sPacket.nFullPacketSize, &sPacket);
#else
    // The standard full-flag complement every retail 1.14d put-in-container
    // send uses (Player.cpp / UNIT/Item.cpp).
    D2Game::GAME::SCmd::Send0x9CItemPutInContainer(pClient, pUnit, pItem,
        ~(ITEMFLAG_TAGGED | ITEMFLAG_DROW | ITEMFLAG_CURSED | ITEMFLAG_STAFFMODS | ITEMFLAG_MAGICAL |
          ITEMFLAG_RUNEWORD | ITEMFLAG_CRUDE | ITEMFLAG_PLAYERNAME | ITEMFLAG_SAVED | ITEMFLAG_ETHEREAL |
          ITEMFLAG_COMPACT | ITEMFLAG_100000 | ITEMFLAG_SERVER | ITEMFLAG_RESTRICT | ITEMFLAG_BEGINNER |
          ITEMFLAG_BODYPART | ITEMFLAG_HARDCORE | ITEMFLAG_DISABLED | ITEMFLAG_NEW | ITEMFLAG_MONSTER |
          ITEMFLAG_SOCKETED | ITEMFLAG_SORTED | ITEMFLAG_RESTORED | ITEMFLAG_BROKEN | ITEMFLAG_TAKEN |
          ITEMFLAG_ADDED | ITEMFLAG_REMOVED | ITEMFLAG_IDENTIFIED | ITEMFLAG_IGNORE | ITEMFLAG_CURSOR |
          ITEMFLAG_BOUGHT | ITEMFLAG_RELOAD));
#endif
}

// Materialize a 'JM' block into the (now empty) page 4 and push the client
// 0x9C put-in-container packets for everything that landed. Returns 1 when
// the page materialized completely (an empty block is a complete empty
// page), 0 when a record failed to decode - the page-4 set is then a SUBSET
// of the block, and the caller must treat the stored block as authoritative.
// [ROB-02]
// [R211] Route B materialisation for the 1.13d realm build: walk our own 'JM'
// block and rebuild units through primitives whose arguments are all known.
// Route A (the stock 0x3a000/0x38bc0 chain) was rejected: the agent proved
// 0x384e0 is the bit-packed 0x47-family decoder (3/5/7/8-bit field widths),
// not our record layout, so it would have misparsed our blocks.
//   ReadInfoFromStreamVersioned - D2Common, one of only TWO NAMED exports
//       (GetProcAddress by name; no ordinal) - 5 args __stdcall
//   ITEM_CreateFromSaveData     - D2Game RVA, TODO
//   PLRSAVE_RestoreItemToUnit   - D2Game RVA, TODO
#if defined(VPSTASH_D2GS_113D)
// CONFIRMED (D2GS agent + byte-verified here): D2Game imports the parse by NAME
// (IAT rva 0xf809c = D2Common!ITEMSReadInfoFromStreamVersioned, thunk 0x6fc2b330),
// and the stock loader's sequence at 0x6fc5a51c..a620 is
//   call parse -> call create (0xE2B70) -> call restore (0x6FC70)
// exactly the loop below.
#ifndef VPSTASH_113D_RVA_CREATE_FROM_SAVE
#define VPSTASH_113D_RVA_CREATE_FROM_SAVE  0x0E2B70
#endif
#ifndef VPSTASH_113D_RVA_RESTORE_TO_UNIT
#define VPSTASH_113D_RVA_RESTORE_TO_UNIT   0x06FC70
#endif
// OPEN QUESTION for the agent: which format constant does stock 1.13d pass into
// ReadInfoFromStreamVersioned / ITEM_CreateFromSaveData for a 'JM' item block?
// Our tree uses 0x60, but the stock 'JM' walker sits under the 0x47 branch, so
// the numbering may differ between versions - a wrong value yields garbage
// items rather than a clean failure, so this must be confirmed, not guessed.
// CONFIRMED (D2GS agent, stock dispatcher 0x6fc5a990): 1.13d accepts 0x47 for the
// legacy JM decode and 0x48..0x60 for the modern one, so 0x60 is legal and routes
// to the modern loader - which matches our stock-produced blocks. No version skew.
#ifndef VPSTASH_113D_ITEM_FORMAT
#define VPSTASH_113D_ITEM_FORMAT 0x60
#endif

static int VPSHARED_MaterializePage4_113d(::SGAMEDATA * pGame, D2UnitStrc * pUnit, uint8_t * pBlock, uint32_t nSize) {
    if (!pBlock || nSize < 4) {
        return 1;
    }
    if (pBlock[0] != 'J' || pBlock[1] != 'M') {
        D2BOOT_Mark("shared stash [1.13d]: block is not a JM list - skipped"); // [d2boot]
        return 0;
    }

    static void * s_pfnReadInfo = nullptr;
    static void * s_pfnCreate = nullptr;
    static void * s_pfnRestore = nullptr;
    if (!s_pfnReadInfo) {
        HMODULE hCommon = GetModuleHandleA("D2Common.dll");
        if (hCommon) {
            s_pfnReadInfo = (void *)GetProcAddress(hCommon, "ITEMSReadInfoFromStreamVersioned");
        }
    }
    if (!s_pfnCreate || !s_pfnRestore) {
        uintptr_t nBase = (uintptr_t)GetModuleHandleA("D2Game.dll");
        s_pfnCreate = (void *)(nBase + VPSTASH_113D_RVA_CREATE_FROM_SAVE);
        s_pfnRestore = (void *)(nBase + VPSTASH_113D_RVA_RESTORE_TO_UNIT);
    }
    if (!s_pfnReadInfo || !s_pfnCreate || !s_pfnRestore) {
        D2BOOT_Mark("shared stash [1.13d]: materialize primitives unresolved - page stays empty"); // [d2boot]
        return 0;
    }

    uint32_t nCount = *(uint16_t *)(pBlock + 2);
    if (nCount > (uint32_t)VPSTASH_PAGE_ITEM_CAP) {
        nCount = (uint32_t)VPSTASH_PAGE_ITEM_CAP;
        D2BOOT_Mark("shared stash [1.13d]: item count clamped"); // [d2boot]
    }
    uint8_t * pCursor = pBlock + 4;
    uint8_t * pEnd = pBlock + nSize;
    int nRestored = 0;
    for (uint32_t i = 0; i < nCount; ++i) {
        if (pCursor >= pEnd) {
            break;
        }
        D2UnitDataItemDetailsStrc itemDetails;
        memset(&itemDetails, 0, sizeof(itemDetails));
        int32_t nConsumed = 0;
        ((BOOL(__stdcall*)(uint8_t*, size_t, BOOL, D2UnitDataItemDetailsStrc*, int32_t))s_pfnReadInfo)(
            pCursor, (size_t)(pEnd - pCursor), 1, &itemDetails, VPSTASH_113D_ITEM_FORMAT);
        if (itemDetails.classId == -1) {
            D2BOOT_Mark("shared stash [1.13d]: record decode failed - page truncated"); // [d2boot]
            break;
        }
        D2UnitStrc * pItem = ((D2UnitStrc *(__fastcall*)(::SGAMEDATA*, void*, uint8_t*, size_t, uint32_t,
                                                         D2UnitDataItemDetailsStrc*, int32_t*, uint32_t))s_pfnCreate)(
            pGame, nullptr, pCursor, (size_t)(pEnd - pCursor), 1, &itemDetails, &nConsumed, VPSTASH_113D_ITEM_FORMAT);
        if (nConsumed <= 0) {
            D2BOOT_Mark("shared stash [1.13d]: record length unavailable - page truncated"); // [d2boot]
            break;
        }
        pCursor += nConsumed;
        if (pItem) {
            ((int(__fastcall*)(::SGAMEDATA*, D2UnitStrc*, D2UnitStrc*, int32_t))s_pfnRestore)(pGame, pUnit, pItem, 0);
            ++nRestored;
        }
    }
    D2BOOT_MarkFmt("shared stash [1.13d]: materialized %d/%u items", nRestored, nCount); // [d2boot]
    return (uint32_t)nRestored == nCount;
}
#endif // VPSTASH_D2GS_113D

static int VPSHARED_MaterializePage4(::SGAMEDATA * pGame, D2UnitStrc * pUnit, uint8_t * pBlock, uint32_t nSize) {
    if (!pBlock || nSize < 4) {
        return 1;
    }

#if defined(VPSTASH_D2GS_113D)
    int nMaterialsed = VPSHARED_MaterializePage4_113d(pGame, pUnit, pBlock, nSize);
    if (!nMaterialsed) {
        return 0;
    }
    // fall through to the client resync below
#else
    short int * pAfter = nullptr;
    // Format 0x60 (the 1.10+ .d2s item-list version), no page handler - the
    // same restore call the trade-cancel backup uses. Placement comes from
    // each record's own location bits (stash + x/y), retail decode.
    int nResult = D2Game::PLAYER::PlayerSave::PLRSAVE_DispatchItemLoadByFormat(pGame, pUnit, pBlock, 0x60, (int32_t)nSize, 0, &pAfter);
    if (nResult) {
        D2BOOT_MarkFmt("shared stash: load error %d - partial page", nResult); // [d2boot]
        return 0; // [ROB-02] caller keeps the stored block authoritative.
    }
#endif

#if defined(VPSTASH_D2GS_113D) && defined(VPSTASH_113D_NO_0X9C)
    // [R222] state-only realm build: the entire client resync is compiled out, so
    // pClient is never declared and nothing below can reference it.
    return 1;
#else
    D2ClientStrc * pClient = D2Game::UNIT::SUnit::SUNIT_GetClient(pUnit, ".\\PLAYER\\VanillaPlusStash.cpp", 0x1d5);
    if (!pClient) {
        return 1; // No client to resync (should not happen in SP) - state is still correct.
    }

    D2UnitStrc * apItems[VPSTASH_PAGE_ITEM_CAP];
    int nItems = VPSHARED_CollectPage4(pUnit, apItems, VPSTASH_PAGE_ITEM_CAP);
    for (int i = 0; i < nItems; ++i) {
        VPSHARED_SendPutInContainer(pClient, pUnit, apItems[i]);
    }
    return 1;
#endif
}

// ---------------------------------------------------------------------------
// Page activation (the whole feature)

// Activate shared page nPage (internal index 0..VPSTASH_PAGES-1 = the UI's
// "page 1".."page 100"; the client sends the 1-based number in the 0x4F param
// word). Three cases, all per-character (the activation record carries this
// player's snapshot; the pool carries this account's pages):
//   personal -> page nPage : snapshot the personal set, materialize page
//                            nPage, borrow the goldbank (v1 toggle shape);
//   page A   -> page B     : serialize page 4 into page A's block, page B's
//                            block materializes - the goldbank borrow simply
//                            continues (ONE pool across all pages) and the
//                            .sss is written on the next restore-to-personal
//                            (or save flush), as in v1;
//   page nPage again       : idempotent no-op (the client may repeat).
static void VPSHARED_ActivatePage(VPActive * pAct, ::SGAMEDATA * pGame, D2UnitStrc * pUnit, int nPage) {
    if (nPage < 0 || nPage >= VPSTASH_PAGES) {
        return;
    }
    VPPool * pPool = pAct->pPool;
    if (pAct->gnPage == nPage + 1) {
        return; // Idempotent: the client may repeat the request.
    }

    if (pAct->gnPage == 0) {
        VPSHARED_EnsureLoaded(pPool);
        // [ROB-01] A transient read failure (file locked by an AV scan or a
        // sync client) deferred the load: refuse the toggle now - the next
        // click retries the read - instead of running the session on an
        // empty state the next save would flush over the real file.
        if (pPool->gbLoadFailed) {
            D2BOOT_Mark("shared stash: toggle refused - stash file unreadable (will retry; disk untouched)"); // [d2boot]
            return;
        }

        // 1. Snapshot the personal set FIRST - it is the only copy while a
        //    shared page is active (it is NOT in the .d2s until the next save).
        uint8_t * pPersonal = nullptr;
        uint32_t nPersonal = 0;
        if (!VPSHARED_SerializePage4(pGame, pUnit, &pPersonal, &nPersonal)) {
            return; // Degrade: keep the personal page live and untouched.
        }

        // 2. Park the personal set, then materialize the requested page.
        VPSHARED_DematerializePage4(pGame, pUnit);
        if (s_nVpRemoveFailed) {
            // Removing the personal set could not be verified: abandon the toggle
            // with the personal page intact (never trade on unverified state).
            D2BOOT_Mark("shared stash: toggle aborted - personal page could not be detached"); // [d2boot]
            VPSHARED_Free(pPersonal);
            return;
        }
        if (pAct->pPersonalBlock) {
            VPSHARED_Free(pAct->pPersonalBlock);
        }
        pAct->pPersonalBlock = pPersonal;
        pAct->nPersonalSize = nPersonal;
        pAct->gbActivePagePartial = !VPSHARED_MaterializePage4(pGame, pUnit, pPool->apSharedBlock[nPage], pPool->anSharedSize[nPage]);
        if (pAct->gbActivePagePartial) {
            // [ROB-02] The stored block stays this page's authoritative copy;
            // toggling away will NOT re-serialize the subset over it.
            D2BOOT_Mark("shared stash: stored block kept authoritative - toggling away will not overwrite it"); // [d2boot]
        }

        // 3. Goldbank borrow: park the personal bank, show the shared pool.
#if !defined(VPSTASH_D2GS_113D)
        uint32_t dwBank = D2Common::Stats::StatsEx::STATSEX_GetCurrentStat(pUnit, UNITSTAT_goldbank, 0);
        pAct->dwPersonalGoldSnapshot = dwBank;
        int32_t nDelta = (int32_t)pPool->dwSharedGold - (int32_t)dwBank;
        if (nDelta) {
            D2Game::UNIT::Party::ChangeGoldForPlayer(pUnit, UNITSTAT_goldbank, nDelta);
        }
#else
    // [R218] v1 realm build skips the goldbank borrow: STATSEX/ChangeGoldForPlayer
    // are D2Common/D2Game imports and gold is not needed to prove the mechanism.
    D2BOOT_Mark("shared stash [1.13d]: gold borrow skipped (v1)"); // [d2boot]
#endif

        pAct->gnPage = nPage + 1;
        D2BOOT_MarkFmt("shared stash: shared page %d active for '%s' (%d items, gold %d)",
                       nPage + 1, pPool->szKey[0] ? pPool->szKey : "(legacy)",
                       VPSHARED_PageItemCount(pPool, nPage),
                       (int)pPool->dwSharedGold); // [d2boot]
        return;
    }

    // Shared page A -> shared page B.
    int nFrom = pAct->gnPage - 1;

    // 1. Serialize the current page-4 set into page A's block - swap it in
    //    only on success so a failed serialize can never poison the page.
    //    [ROB-02] If A materialized partially, page 4 is a SUBSET of A's
    //    stored block: skip the serialize entirely and keep the stored
    //    block - a decode failure degrades this session's edits on A, it
    //    never erases records that exist on disk.
    if (pAct->gbActivePagePartial) {
        D2BOOT_MarkFmt("shared stash: page %d was partial - stored copy kept, session edits on it discarded", (int)(nFrom + 1)); // [d2boot]
    } else {
        uint8_t * pBlock = nullptr;
        uint32_t nSize = 0;
        if (!VPSHARED_SerializePage4(pGame, pUnit, &pBlock, &nSize)) {
            return; // Degrade: keep the current shared page live and untouched.
        }
        if (pPool->apSharedBlock[nFrom]) {
            VPSHARED_Free(pPool->apSharedBlock[nFrom]);
        }
        pPool->apSharedBlock[nFrom] = pBlock;
        pPool->anSharedSize[nFrom] = nSize;
    }

    // 2. Park page A, materialize page B.
    VPSHARED_DematerializePage4(pGame, pUnit);
    if (s_nVpRemoveFailed) {
        D2BOOT_Mark("shared stash: page switch aborted - page A could not be detached"); // [d2boot]
        return;
    }
    pAct->gbActivePagePartial = !VPSHARED_MaterializePage4(pGame, pUnit, pPool->apSharedBlock[nPage], pPool->anSharedSize[nPage]);
    if (pAct->gbActivePagePartial) {
        D2BOOT_Mark("shared stash: stored block kept authoritative - toggling away will not overwrite it"); // [d2boot]
    }

    pAct->gnPage = nPage + 1;
    D2BOOT_MarkFmt("shared stash: switched to page %d (%d items)",
                   nPage + 1, VPSHARED_PageItemCount(pPool, nPage)); // [d2boot]
}

static void VPSHARED_Park(VPActive * pAct, ::SGAMEDATA * pGame, D2UnitStrc * pUnit) {
    VPPool * pPool = pAct->pPool;
    int nFrom = pAct->gnPage - 1;

    // 1. Serialize the active shared set AS IT STANDS NOW (session changes
    //    included) into a scratch block; swap it in only on success so a
    //    failed write can never poison the in-memory shared copy.
    //    [ROB-02] A partial materialization is the exception: page 4 is a
    //    subset of the stored block, so the serialize is skipped and the
    //    stored block survives as authoritative (this session's edits on
    //    the page are discarded - a decode failure degrades, never erases).
    if (pAct->gbActivePagePartial) {
        D2BOOT_MarkFmt("shared stash: page %d was partial - stored copy kept, session edits on it discarded", (int)(nFrom + 1)); // [d2boot]
    } else {
        uint8_t * pShared = nullptr;
        uint32_t nShared = 0;
        if (!VPSHARED_SerializePage4(pGame, pUnit, &pShared, &nShared)) {
            return; // Degrade: keep the shared page live and untouched.
        }
        if (pPool->apSharedBlock[nFrom]) {
            VPSHARED_Free(pPool->apSharedBlock[nFrom]);
        }
        pPool->apSharedBlock[nFrom] = pShared;
        pPool->anSharedSize[nFrom] = nShared;
    }

    // 2. Park the shared set, restore the personal set.
    VPSHARED_DematerializePage4(pGame, pUnit);
    if (s_nVpRemoveFailed) {
        // The save cannot be refused from here (I1 would be violated either way),
        // so make it loud: shared items may have leaked into this character save.
        D2BOOT_Mark("shared stash: WARNING shared set not detached before save (I1 at risk)"); // [d2boot]
    }
    VPSHARED_MaterializePage4(pGame, pUnit, pAct->pPersonalBlock, pAct->nPersonalSize);

    // 3. Un-borrow the goldbank: absorb what the retail gold buttons moved.
#if !defined(VPSTASH_D2GS_113D)
    uint32_t dwBank = D2Common::Stats::StatsEx::STATSEX_GetCurrentStat(pUnit, UNITSTAT_goldbank, 0);
    pPool->dwSharedGold = dwBank;
    int32_t nDelta = (int32_t)pAct->dwPersonalGoldSnapshot - (int32_t)dwBank;
    if (nDelta) {
        D2Game::UNIT::Party::ChangeGoldForPlayer(pUnit, UNITSTAT_goldbank, nDelta);
    }
#else
    // [R218] v1 realm build skips the goldbank borrow: STATSEX/ChangeGoldForPlayer
    // are D2Common/D2Game imports and gold is not needed to prove the mechanism.
    D2BOOT_Mark("shared stash [1.13d]: gold borrow skipped (v1)"); // [d2boot]
#endif

    pAct->gnPage = 0;
    VPSHARED_WriteFileLocked(pPool);
    D2BOOT_MarkFmt("shared stash: personal page restored (%d items)",
                   (int)(pAct->pPersonalBlock && pAct->nPersonalSize >= 4 ? (uint16_t)(pAct->pPersonalBlock[2] | (pAct->pPersonalBlock[3] << 8)) : 0)); // [d2boot]
}

// ---------------------------------------------------------------------------
// Entry points (see VanillaPlusStash.h)

BOOL __stdcall VPSHARED_ShouldInterceptClickButton(::SGAMEDATA * pGame, D2UnitStrc * pUnit, int16_t nButtonId, uint32_t dwParam) {
    if (!pUnit || !pGame) {
        return FALSE;
    }
    if (!VPSHARED_FeatureActive()) {
        return FALSE;
    }
    // [R205] Button id + pad magic both come from the ini (they must match
    // the client's features/shared_stash_button / shared_stash_pad_magic).
    // The magic disambiguates a stash toggle from a genuine retail click of
    // the same, whitelisted button id (needed on realms, where d2server.dll
    // drops ids it doesn't whitelist before D2Game ever runs).
    if (nButtonId != (int16_t)s_nVpStashButtonCache) {
        return FALSE;
    }
    if ((uint16_t)(dwParam >> 16) != (uint16_t)s_nVpStashPadMagicCache) {
        return FALSE; // a real click of this button - retail path handles it
    }
    // [R204] Game-type gate removed: every game type is server-backed D2Game
    // in this build (SP runs the local server; BNet/OBNet/LAN run D2Game on
    // the realm host), so the toggle is handled everywhere. Pools are per
    // account, so two players online hold two different pools - no
    // exclusivity needed (the v1 single-pool exclusive-holder draft is
    // superseded by the per-account keys).
    return TRUE;
}

void __stdcall VPSHARED_OnClickButton(::SGAMEDATA * pGame, D2UnitStrc * pUnit, int16_t nButtonId, uint32_t dwParam) {
    if (!pGame || !pUnit || nButtonId != (int16_t)s_nVpStashButtonCache) {
        return;
    }
    if ((uint16_t)(dwParam >> 16) != (uint16_t)s_nVpStashPadMagicCache) {
        return; // not a stash toggle
    }
    // [R205] The page rides the packet's LAST u16; the pad word in front of
    // it carries the magic. Decode the page from the low word.
    dwParam = (uint32_t)(uint16_t)dwParam;
    VPSHARED_EnsureOwner(pGame);

    // [R204] Per-account pools: the key is this character's account name
    // ("" = the legacy single-player global file), so each account shares
    // its pool across its own characters only - never with other players.
    char szKey[VPSTASH_KEY_MAX + 1];
    VPSHARED_GetAccountKey(pUnit, szKey, sizeof(szKey));
    VPPool * pPool = VPSHARED_GetPool(szKey);
    if (!pPool) {
        return;
    }

    // dwParam = the ABSOLUTE desired page: 0 personal, 1..100 shared pages
    // 1..100 (a hundred since R146 - the planned 100 arrived; the param word
    // always had the room). Anything above the page count is ignored with a
    // mark (never a halt).
    if (dwParam == 0) {
        VPActive * pActive = VPSHARED_FindActive(pUnit);
        if (pActive && pActive->gnPage > 0) {
            VPSHARED_Park(pActive, pGame, pUnit);
        }
        return; // already personal (or no shared page ever opened): no-op
    }
    if (dwParam > (uint32_t)VPSTASH_PAGES) {
        D2BOOT_MarkFmt("shared stash: page %d out of range - ignored", (int)dwParam); // [d2boot]
        return;
    }

    VPActive * pActive = VPSHARED_GetActive(pUnit, pPool);
    if (!pActive) {
        return;
    }
    VPSHARED_ActivatePage(pActive, pGame, pUnit, (int)dwParam - 1);
}

void __stdcall VPSHARED_FlushBeforeSave(::SGAMEDATA * pGame, D2UnitStrc * pUnit) {
    // Zero-op unless THIS character holds a shared page: with per-account
    // pools another player's save cannot touch this account's pool at all
    // (their page 4 is their own personal set). The ini gate is deliberately
    // NOT consulted here: once the shared page is live, the personal set
    // exists only in memory, so the park must happen even if the owner
    // flipped the key off mid-session.
    if (!pGame || !pUnit) {
        return;
    }
    VPActive * pActive = VPSHARED_FindActive(pUnit);
    if (!pActive || pActive->gnPage <= 0) {
        return;
    }

    // [R204] Remember the page so VPSHARED_AfterSave can put the shared page
    // back once this save has built (realm saves fire per-player on
    // intervals/waypoints, not just at exit - without the re-activation every
    // server save would kick the holder to their personal page).
    pActive->gnPostSavePage = pActive->gnPage;

    D2BOOT_Mark("shared stash: flush before save"); // [d2boot]
    VPSHARED_Park(pActive, pGame, pUnit);
}

// [R204] Called from the thin VP site at the END of
// PlayerSave::SaveGameAllGameTypes - after the .d2s/buffer has been built
// from the parked (personal-only) state. Puts the shared page back for the
// holder so realm interval/waypoint saves don't kick them to their personal
// page: the park is per-save (I1 keeps the save retail-only), the shared
// page is per-session.
void __stdcall VPSHARED_AfterSave(::SGAMEDATA * pGame, D2UnitStrc * pUnit) {
    if (!pGame || !pUnit) {
        return;
    }
    VPActive * pActive = VPSHARED_FindActive(pUnit);
    if (!pActive || !pActive->gnPostSavePage) {
        return; // not holding, or nothing parked for this save
    }
    int nPage = pActive->gnPostSavePage - 1;
    pActive->gnPostSavePage = 0;
    // Key flipped off mid-session: stay on the personal page (the flush
    // already parked the shared set safely).
    if (!VPSHARED_FeatureActive()) {
        D2BOOT_Mark("shared stash: post-save re-activation skipped (feature key off)"); // [d2boot]
        return;
    }
    // The player toggled while the save was building: their click already
    // picked the live page - the parked request is obsolete.
    if (pActive->gnPage != 0) {
        D2BOOT_MarkFmt("shared stash: post-save re-activation skipped (player moved to page %d)", pActive->gnPage); // [d2boot]
        return;
    }

    D2BOOT_MarkFmt("shared stash: re-activating shared page %d after save", nPage + 1); // [d2boot]
    VPSHARED_ActivatePage(pActive, pGame, pUnit, nPage);
}

// ===========================================================================
// [R218] d2vpstash.dll entry point (realm build only).
//
// The D2GS stub patches stock D2Game at 0x6fc8cf6f (the 0x4F handler, after its
// nSize==7 validation) and calls this once per ClickButton packet. At that
// instant the handler's two "fastcall-style" arguments - pGame and pUnit, which
// it later pushes into PlayerTrade::ClickButton - are still live in ECX and EDX,
// so the stub must forward them alongside the packet pointer:
//
//     push [esp+4]      ; pPacket
//     push edx          ; pUnit
//     push ecx          ; pGame
//     call VPSHARED_HookPacket
//
// Return 1 = handled: the stub answers SERVERSTATUS_OK (xor eax,eax; ret 8), so
// d2server logs no [cheat] line. Return 0 = not ours: the stub replays the
// displaced bytes (8B 44 24 04 56 57) and resumes at 0x6fc8cf75.
//
// Packet layout at the hook (1.13d, verified): +0 type 0x4F, +1 button u16,
// +3 pad u16, +5 page u16.
#if defined(VPSTASH_113D_DLL)
// [R223] extern "C" is REQUIRED: this TU is C++, so without it the symbol is
// mangled (_Z19VPSHARED_HookPacketPvS_) and ld cannot satisfy the .def's
// `VPSHARED_HookPacket@12` ("symbol not defined"). C linkage + __stdcall yields
// _VPSHARED_HookPacket@12 exactly as the D2GS stub's GetProcAddress expects.
extern "C" int __stdcall VPSHARED_HookPacket(void * pGame, void * pUnit, void * pPacket) {
    if (!pGame || !pUnit || !pPacket) {
        return 0;
    }
    const uint8_t * p = (const uint8_t *)pPacket;
    if (p[0] != 0x4F) {
        return 0;
    }
    uint16_t nButton = *(const uint16_t *)(p + 1);
    uint16_t nPad    = *(const uint16_t *)(p + 3);
    uint16_t nPage   = *(const uint16_t *)(p + 5);
    uint32_t dwParam = ((uint32_t)nPad << 16) | nPage;

    if (!VPSHARED_ShouldInterceptClickButton((::SGAMEDATA *)pGame, (D2UnitStrc *)pUnit,
                                             (int16_t)nButton, dwParam)) {
        return 0; // a real retail click of this id - let stock handle it
    }
    VPSHARED_OnClickButton((::SGAMEDATA *)pGame, (D2UnitStrc *)pUnit, (int16_t)nButton, dwParam);
    return 1;
}
#endif // VPSTASH_113D_DLL
