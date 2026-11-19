# D2VP-SetPrivateGateway.ps1 — point D2VP (Diablo II 1.14d client) at a private Battle.net server.
#
# What the game actually reads (verified in D2VP-QoL Source):
#   1. HKCU\Software\Battle.net\Configuration, value "Override Battle.net gateways"
#      (REG_MULTI_SZ) — if present it REPLACES the retail gateway list (Bnclient
#      BNGatewayAccess::Load reads it before "Diablo II Battle.net gateways").
#      Blob layout: <version> <selected-index> then per gateway: <DNS/IP> <ZONE> <Name>,
#      version must be >= 1000 or the client autodeletes the list.
#   2. HKCU\Software\Blizzard Entertainment\Diablo II, value "BNETIP" (REG_SZ) — hard
#      override for the connect address (Bnclient BnSend::GetBnetIp reads it after the
#      gateway list and prefers it).
#
# This script writes BOTH (kept consistent), backs up the existing gateway value to a
# .reg file first, and prints the resulting state. The retail list value is left
# untouched, so D2VP-RestoreGateway.ps1 restores original Battle.net by deleting the
# two override values (no backup needed to get retail back).
#
# NOTE: the client always connects on TCP port 6112 (hardcoded). Your PvPGN/mirror
# server must listen on 6112. Realm/game-server ports behind it are the server's own
# configuration (typically 6113/6114).
#
# Usage:
#   .\D2VP-SetPrivateGateway.ps1 -Server pvpgn.myhost.com
#   .\D2VP-SetPrivateGateway.ps1 -Server 192.168.1.50 -Name "Lan Server" -Zone 1
#   .\D2VP-SetPrivateGateway.ps1 -Server pvpgn.myhost.com -NoBnetIP     # gateway list only
#   .\D2VP-SetPrivateGateway.ps1 -Show                                  # just print current state

[CmdletBinding()]
param(
    [Parameter(Position = 0)]
    [string]$Server,

    [string]$Name = "D2VP Private Server",

    # GMT offset in hours used for the gateway entry (display/zone-pick only).
    [double]$Zone = ($(Get-TimeZone).BaseUtcOffset.TotalHours * -1),

    # Skip writing the BNETIP hard override (gateway list only).
    [switch]$NoBnetIP,

    # Print the current gateway state and exit.
    [switch]$Show,

    # Leave the retail list intact but REMOVE any private override instead of setting one.
    [switch]$Undo
)

$ErrorActionPreference = "Stop"

$BnetKeyPath = "HKCU:\Software\Battle.net\Configuration"
$D2KeyPath   = "HKCU:\Software\Blizzard Entertainment\Diablo II"
$ValueOverride = "Override Battle.net gateways"
$ValueRetail   = "Diablo II Battle.net gateways"
$ValueBnetIp   = "BNETIP"

function Get-GatewayState {
    $state = [ordered]@{}
    if (Test-Path $BnetKeyPath) {
        $k = Get-Item -Path $BnetKeyPath
        foreach ($v in @($ValueOverride, $ValueRetail)) {
            if ($k.GetValue($v, $null) -is [string[]]) {
                $state[$v] = ($k.GetValue($v) -join "`n")
            } elseif ($null -ne $k.GetValue($v, $null)) {
                $state[$v] = ($k.GetValue($v) -join "`n")
            }
        }
    }
    if (Test-Path $D2KeyPath) {
        $ip = (Get-Item -Path $D2KeyPath).GetValue($ValueBnetIp, $null)
        if ($ip) { $state[$ValueBnetIp] = $ip }
    }
    return $state
}

function Show-State {
    $s = Get-GatewayState
    if ($s.Count -eq 0) {
        Write-Host "No Battle.net gateway values found under HKCU (the game will write retail defaults from gateways.txt on next launch)."
        return
    }
    foreach ($key in $s.Keys) {
        Write-Host "--- $key ---"
        if ($key -eq $ValueBnetIp) {
            Write-Host "  $ValueBnetIp = $($s[$key])"
        } else {
            $lines = $s[$key] -split "`n"
            if ($lines.Count -ge 2) {
                Write-Host ("  version = {0}, selected gateway = {1}, entries:" -f $lines[0], $lines[1])
                for ($i = 2; $i + 2 -lt $lines.Count; $i += 3) {
                    Write-Host ("    [{0}] {1}  (GMT {2})  {3}" -f (($i - 2) / 3 + 1), $lines[$i], $lines[$i + 1], $lines[$i + 2])
                }
            } else {
                Write-Host "  $lines"
            }
        }
    }
}

if ($Show) { Show-State; exit 0 }

if ($Undo) {
    if (Test-Path $BnetKeyPath) {
        Remove-ItemProperty -Path $BnetKeyPath -Name $ValueOverride -ErrorAction SilentlyContinue
    }
    if (Test-Path $D2KeyPath) {
        Remove-ItemProperty -Path $D2KeyPath -Name $ValueBnetIp -ErrorAction SilentlyContinue
    }
    Write-Host "Private gateway override removed. Retail Battle.net gateways restored."
    Show-State
    exit 0
}

if ([string]::IsNullOrWhiteSpace($Server)) {
    Write-Host "Usage: .\D2VP-SetPrivateGateway.ps1 -Server <dns-or-ip> [-Name <name>] [-Zone <gmt>] [-NoBnetIP] [-Show] [-Undo]"
    Write-Host "Run with -Show to print the current gateway state."
    exit 1
}

# ---- backup: export the whole Configuration key once, before touching anything ----
$backupPath = Join-Path -Path $PSScriptRoot -ChildPath "D2VP-Gateway-Backup.reg"
if (-not (Test-Path $backupPath)) {
    $regPath = "HKCU\Software\Battle.net\Configuration"
    if (Test-Path "Registry::$regPath") {
        & reg.exe export $regPath $backupPath /y | Out-Null
        if ($LASTEXITCODE -eq 0) {
            Write-Host "Backup written: $backupPath"
        } else {
            Write-Warning "reg export failed ($LASTEXITCODE) - continuing without a backup file (retail list value is still not modified by this script)."
        }
    }
} else {
    Write-Host "Backup already exists, keeping it: $backupPath"
}

# ---- build the MULTI_SZ gateway blob ----
# Entry count must satisfy the client's 3n+1 parse (version + index + 3 strings per gateway).
$version = "1014"
$index   = "01"
$zoneStr = "{0}" -f [int]$Zone

$entries = [System.Collections.Generic.List[string]]::new()
$entries.Add($version)
$entries.Add($index)
foreach ($srv in @($Server)) {
    $entries.Add($srv.Trim())
    $entries.Add($zoneStr)
    $entries.Add($Name)
}

# ---- write the override (this is the value the client prefers) ----
if (-not (Test-Path $BnetKeyPath)) {
    New-Item -Path $BnetKeyPath -Force | Out-Null
}
New-ItemProperty -Path $BnetKeyPath -Name $ValueOverride `
    -PropertyType MultiString -Value $entries.ToArray() -Force | Out-Null
Write-Host "Wrote HKCU\Software\Battle.net\Configuration\$ValueOverride (MultiString):"
for ($i = 2; $i + 2 -lt $entries.Count; $i += 3) {
    Write-Host ("  gateway {0}: {1}  (GMT {2})  {3}" -f (($i - 2) / 3 + 1), $entries[$i], $entries[$i + 1], $entries[$i + 2])
}

# ---- write the BNETIP hard override (GetBnetIp prefers it over the list DNS) ----
if (-not $NoBnetIP) {
    if (-not (Test-Path $D2KeyPath)) {
        New-Item -Path $D2KeyPath -Force | Out-Null
    }
    New-ItemProperty -Path $D2KeyPath -Name $ValueBnetIp `
        -PropertyType String -Value (@($Server)[0].Trim()) -Force | Out-Null
    Write-Host "Wrote HKCU\Software\Blizzard Entertainment\Diablo II\$ValueBnetIp = $(@($Server)[0].Trim())"
} else {
    Write-Host "Skipped BNETIP hard override (-NoBnetIP)."
}

Write-Host ""
Write-Host "Done. The client connects on TCP 6112 - make sure the server listens there."
Write-Host "Restart D2VP. On the title screen the realm button now shows: $Name"
Write-Host "To go back to real Battle.net: .\D2VP-RestoreGateway.ps1 (or this script with -Undo)."
