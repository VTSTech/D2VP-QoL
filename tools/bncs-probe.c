/* bncs-probe.c — standalone Battle.net BNCS handshake probe (R175 companion tool).
 *
 * Purpose: replay the EXACT wire sequence D2VP produces against the LAN PvPGN server,
 * but from a tiny unmodded console process, with per-operation logging and a socket
 * autopsy on any failure. This separates "the connection pattern gets killed" from
 * "D2VP.exe the process gets killed":
 *
 *   - probe dies at the same point with the same 10053 autopsy
 *         -> the kill is connection/pattern-based (network filter / WFP rule /
 *            global AV network shield) and does not target D2VP specifically.
 *   - probe sails through
 *         -> the kill targets D2VP.exe (AV reputation / per-app verdict);
 *            add F:\GAMES\Diablo II to the AV exclusions and re-test.
 *
 * Wire sequence (verified identical to D2VP's in the 2026-11-20 pcap):
 *   connect TCP host:6112
 *   -> 0x01 protocol byte
 *   -> 0x50 AUTH_INFO (47-byte payload, header FF 50 33 00; byte-identical layout to
 *      NET_SID_CLIENT_Send_0x50_AuthInfo: protocol=0, "IX86", "D2XP", versionid 14,
 *      "CAN", local ip, tz bias, locale id, language id, "CAN" country abbrev, "Canada")
 *   <- 0x25 SERVER_ECHOREQ (8 bytes)  -> echo it back (what D2VP does)
 *   <- AUTHREQ_109 (103 bytes)        -> just log it
 *   then sit 15 s in a select/recv loop, echoing any further 0x25.
 *
 * Options:
 *   bncs-probe.exe [host] [port] [--skip0x50] [--slow N]
 *     --skip0x50  do not send AUTH_INFO (tests whether the kill needs the 0x50/echo step)
 *     --slow N    sleep N ms between operations (tests the ~16 ms timing invariant)
 *
 * Build (MSYS2 MinGW 32-bit or any MinGW/clang toolchain):
 *   clang bncs-probe.c -o bncs-probe.exe -lws2_32
 *   gcc   bncs-probe.c -o bncs-probe.exe -lws2_32
 *
 * Output: console + bncs-probe.log (same directory). Every line: [tick delta ms] event.
 */

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

static FILE* g_log = NULL;
static DWORD g_t0 = 0;

static void Log(const char* fmt, ...) {
    char szLine[1024];
    va_list args;
    va_start(args, fmt);
    _vsnprintf(szLine, sizeof(szLine) - 1, fmt, args);
    szLine[sizeof(szLine) - 1] = '\0';
    va_end(args);
    printf("[%08lu ms] %s\n", (unsigned long)(GetTickCount() - g_t0), szLine);
    if (g_log) {
      fprintf(g_log, "[%08lu ms] %s\n", (unsigned long)(GetTickCount() - g_t0), szLine);
      fflush(g_log);
    }
}

/* One-shot forensic probe, mirroring D2VP R174's SAFESOCK_LogRecvAutopsy. */
static void Autopsy(SOCKET s, const char* szContext, int nErr) {
    char szPeer[32], szLocal[32];
    DWORD dwTick = GetTickCount();
    sockaddr_in sa;
    int nLen = (int)sizeof(sa);
    BOOL bPeer = getpeername(s, (sockaddr*)&sa, &nLen) != -1;
    if (bPeer) {
      _snprintf(szPeer, sizeof(szPeer) - 1, "%u.%u.%u.%u",
                sa.sin_addr.S_un.S_un_b.s_b1, sa.sin_addr.S_un.S_un_b.s_b2,
                sa.sin_addr.S_un.S_un_b.s_b3, sa.sin_addr.S_un.S_un_b.s_b4);
      szPeer[sizeof(szPeer) - 1] = '\0';
    } else {
      _snprintf(szPeer, sizeof(szPeer) - 1, "unavail(err=%d)", WSAGetLastError());
    }
    nLen = (int)sizeof(sa);
    BOOL bLocal = getsockname(s, (sockaddr*)&sa, &nLen) != -1;
    if (bLocal) {
      _snprintf(szLocal, sizeof(szLocal) - 1, "%u.%u.%u.%u",
                sa.sin_addr.S_un.S_un_b.s_b1, sa.sin_addr.S_un.S_un_b.s_b2,
                sa.sin_addr.S_un.S_un_b.s_b3, sa.sin_addr.S_un.S_un_b.s_b4);
      szLocal[sizeof(szLocal) - 1] = '\0';
    } else {
      _snprintf(szLocal, sizeof(szLocal) - 1, "unavail(err=%d)", WSAGetLastError());
    }
    int nSoErr = -1, nType = -1, nOptLen = 4, nE1, nE2;
    nE1 = getsockopt(s, SOL_SOCKET, SO_ERROR, (char*)&nSoErr, &nOptLen);
    int nErr1 = nE1 == -1 ? WSAGetLastError() : 0;
    nOptLen = 4;
    nE2 = getsockopt(s, SOL_SOCKET, SO_TYPE, (char*)&nType, &nOptLen);
    int nErr2 = nE2 == -1 ? WSAGetLastError() : 0;
    fd_set r, e;
    FD_ZERO(&r); FD_ZERO(&e); FD_SET(s, &r); FD_SET(s, &e);
    timeval tv; tv.tv_sec = 0; tv.tv_usec = 0;
    int nSel = select(0x40, &r, NULL, &e, &tv);
    char chPeek[8];
    int nPeek = recv(s, chPeek, (int)sizeof(chPeek), MSG_PEEK);
    int nPeekErr = nPeek < 0 ? WSAGetLastError() : 0;
    char chZero = 0;
    int nSend0 = send(s, &chZero, 0, 0);
    int nSendErr = nSend0 < 0 ? WSAGetLastError() : 0;
    Log("AUTOPSY(%s) err=%d tick=%lu peer=%s local=%s soerr=%d getsockopt(err=%d) type=%d/%d sel=%d peek=%d/%d send0=%d/%d",
        szContext, nErr, (unsigned long)dwTick, szPeer, szLocal,
        nSoErr, nErr1, nType, nErr2, nSel, nPeek, nPeekErr, nSend0, nSendErr);
}

static void HexHead(const char* szTag, const char* pBuf, int nBytes) {
    char szHex[96];
    int nShow = nBytes < 16 ? nBytes : 16;
    int i;
    for (i = 0; i < nShow; i++) {
      _snprintf(szHex + i * 3, 4, "%02X ", (unsigned char)pBuf[i]);
    }
    szHex[nShow * 3] = '\0';
    Log("%s (%d bytes): %s", szTag, nBytes, szHex);
}

/* Build D2VP's exact 0x50 AUTH_INFO packet: 4-byte header + 47-byte payload. */
static int BuildAuthInfo(char* pOut, int nOutSize) {
    const char* szCountry = "Canada";
    int nPayload = 40 + (int)strlen(szCountry) + 1;
    int nTotal = 4 + nPayload;
    if (nOutSize < nTotal) {
      return 0;
    }
    memset(pOut, 0, nTotal);
    pOut[0] = (char)0xFF;
    pOut[1] = 0x50;
    pOut[2] = (char)(nTotal & 0xFF);
    pOut[3] = (char)(nTotal >> 8);
    /* payload dwords (little-endian host order, matching the struct the game sends) */
    *(unsigned long*)(pOut + 4)  = 0;                      /* protocol        */
    memcpy(pOut + 8,  "IX86", 4);                          /* platform        */
    memcpy(pOut + 12, "D2XP", 4);                          /* product         */
    *(unsigned long*)(pOut + 16) = 14;                     /* version id      */
    memcpy(pOut + 20, "CAN", 3);                           /* lang abbrev     */
    *(unsigned long*)(pOut + 24) = 0x0C00A8C0;             /* local ip 192.168.0.12 (info only) */
    *(unsigned long*)(pOut + 28) = 0;                      /* tz bias         */
    *(unsigned long*)(pOut + 32) = 0x00001009;             /* locale id       */
    *(unsigned long*)(pOut + 36) = 0x00001009;             /* language id     */
    memcpy(pOut + 40, "CAN", 3);                           /* country abbrev  */
    memcpy(pOut + 44, szCountry, (int)strlen(szCountry) + 1);
    return nTotal;
}

int main(int argc, char** argv) {
    const char* szHost = "192.168.0.14";
    u_short nPort = 6112;
    int bSkip0x50 = 0;
    int nSlowMs = 0;
    int i;
    for (i = 1; i < argc; i++) {
      if (!strcmp(argv[i], "--skip0x50")) {
        bSkip0x50 = 1;
      } else if (!strcmp(argv[i], "--slow") && i + 1 < argc) {
        nSlowMs = atoi(argv[++i]);
      } else if (argv[i][0] != '-') {
        szHost = argv[i];
        if (i + 1 < argc && argv[i + 1][0] != '-') {
          nPort = (u_short)atoi(argv[++i]);
        }
      }
    }

    g_t0 = GetTickCount();
    g_log = fopen("bncs-probe.log", "w");
    Log("bncs-probe start host=%s port=%u skip0x50=%d slow=%dms", szHost, (unsigned)nPort, bSkip0x50, nSlowMs);

    WSADATA wsa;
    int nRc = WSAStartup(0x101, &wsa);
    if (nRc) {
      Log("WSAStartup failed %d", nRc);
      return 1;
    }

    SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == -1) {
      Log("socket() failed %d", WSAGetLastError());
      return 1;
    }
    /* keepalive on, like the game's Safesock::Create */
    BOOL bKeep = 1;
    setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, (char*)&bKeep, sizeof(bKeep));

    sockaddr_in saServer;
    memset(&saServer, 0, sizeof(saServer));
    saServer.sin_family = AF_INET;
    saServer.sin_port = htons(nPort);
    saServer.sin_addr.s_addr = inet_addr(szHost);

    Log("connecting...");
    if (connect(s, (sockaddr*)&saServer, (int)sizeof(saServer)) == -1) {
      Log("connect failed err=%d", WSAGetLastError());
      Autopsy(s, "connect", WSAGetLastError());
      return 1;
    }
    Log("connected");

    if (nSlowMs) Sleep(nSlowMs);
    char chProto = 1;
    if (send(s, &chProto, 1, 0) == -1) {
      Log("send 0x01 failed err=%d", WSAGetLastError());
      Autopsy(s, "send0x01", WSAGetLastError());
      return 1;
    }
    Log("sent 0x01 protocol byte");

    if (!bSkip0x50) {
      char chAuthInfo[64];
      int nLen = BuildAuthInfo(chAuthInfo, (int)sizeof(chAuthInfo));
      if (nSlowMs) Sleep(nSlowMs);
      if (nLen <= 0 || send(s, chAuthInfo, nLen, 0) == -1) {
        Log("send 0x50 failed (len=%d err=%d)", nLen, WSAGetLastError());
        Autopsy(s, "send0x50", WSAGetLastError());
        return 1;
      }
      Log("sent 0x50 AUTH_INFO (%d bytes)", nLen);
    }

    /* select/recv loop: echo 0x25, log anything else, sit 15 s, autopsy on death */
    char chBuf[8192];
    DWORD dwWatchEnd = GetTickCount() + 15000;
    while (GetTickCount() < dwWatchEnd) {
      fd_set r;
      FD_ZERO(&r);
      FD_SET(s, &r);
      timeval tv;
      tv.tv_sec = 0;
      tv.tv_usec = 500000;
      int nSel = select(0x40, &r, NULL, NULL, &tv);
      if (nSel <= 0) {
        continue;
      }
      int nBytes = recv(s, chBuf, (int)sizeof(chBuf), 0);
      if (nBytes == -1) {
        int nErr = WSAGetLastError();
        Log("RECV DIED err=%d", nErr);
        Autopsy(s, "recv-loop", nErr);
        Log("VERDICT: plain unmodded process connection was killed the same way - the kill is connection-based, not D2VP-specific");
        break;
      }
      if (nBytes == 0) {
        Log("recv 0 - server closed gracefully (FIN)");
        Autopsy(s, "recv-fin", 0);
        break;
      }
      HexHead("recv", chBuf, nBytes);
      if (nBytes >= 4 && (unsigned char)chBuf[0] == 0xFF && (unsigned char)chBuf[1] == 0x25) {
        if (send(s, chBuf, nBytes, 0) == -1) {
          int nErr = WSAGetLastError();
          Log("echo send failed err=%d", nErr);
          Autopsy(s, "echo-send", nErr);
          break;
        }
        Log("echoed 0x25 (%d bytes)", nBytes);
      } else if (nBytes >= 4) {
        Log("server packet op=0x%02x len=%u - logged, not echoed",
            (unsigned)(unsigned char)chBuf[1], (unsigned)(unsigned char)chBuf[2] | ((unsigned)(unsigned char)chBuf[3] << 8));
      }
    }
    if (GetTickCount() >= dwWatchEnd) {
      Log("survived the full 15 s watch window with no abort - connection healthy");
      Log("VERDICT: the kill targets D2VP.exe specifically (or the game's socket setup), not this handshake pattern");
    }
    closesocket(s);
    WSACleanup();
    if (g_log) {
      fclose(g_log);
    }
    return 0;
}
