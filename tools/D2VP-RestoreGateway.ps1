# D2VP-RestoreGateway.ps1 — put D2VP (Diablo II 1.14d client) back on real Battle.net.
#
# Undo for D2VP-SetPrivateGateway.ps1. The retail gateway list value
# ("Diablo II Battle.net gateways" under HKCU\Software\Battle.net\Configuration) is
# never modified by the set script, so removing the two private-server overrides is
# enough:
#   1. Delete "Override Battle.net gateways" (BNGatewayAccess::Load falls back to the
#      retail list when this value is absent).
#   2. Delete "BNETIP" under HKCU\Software\Blizzard Entertainment\Diablo II
#      (BnSend::GetBnetIp honors it as a hard connect-address override).
# If the retail list value is ALSO gone or looks broken, the game rebuilds the retail
# defaults from Data\Global\excel\gateways.txt on the next launch on its own, so this
# script never has to write gateway data itself.
#
# -RestoreBackup: additionally import the D2VP-Gateway-Backup.reg written by the set
# script (only needed if you also want the exact pre-change registry state back, e.g.
# a modified retail list or a pre-existing BNETIP).
#
# Usage:
#   .\D2VP-RestoreGateway.ps1              # remove private overrides, keep retail list
#   .\D2VP-RestoreGateway.ps1 -RestoreBackup
#   .\D2VP-RestoreGateway.ps1 -Show        # just print current state

[CmdletBinding()]
param(
    [switch]$RestoreBackup,
    [switch]$Show
)

$ErrorActionPreference = "Stop"

$BnetKeyPath = "HKCU:\Software\Battle.net\Configuration"
$D2KeyPath   = "HKCU:\Software\Blizzard Entertainment\Diablo II"
$ValueOverride = "Override Battle.net gateways"
$ValueRetail   = "Diablo II Battle.net gateways"
$ValueBnetIp   = "BNETIP"
$backupPath    = Join-Path -Path $PSScriptRoot -ChildPath "D2VP-Gateway-Backup.reg"

function Show-State {
    if (Test-Path $BnetKeyPath) {
        $k = Get-Item -Path $BnetKeyPath
        foreach ($v in @($ValueOverride, $ValueRetail)) {
            $val = $k.GetValue($v, $null)
            if ($null -ne $val) {
                Write-Host "--- $v ---"
                ($val -join "`n") -split "`n" | ForEach-Object { Write-Host "  $_" }
            } else {
                Write-Host "$v : (not present)"
            }
        }
    } else {
        Write-Host "$BnetKeyPath : (key not present)"
    }
    if (Test-Path $D2KeyPath) {
        $ip = (Get-Item -Path $D2KeyPath).GetValue($ValueBnetIp, $null)
        Write-Host ("{0} : {1}" -f $ValueBnetIp, ($(if ($ip) { $ip } else { "(not present)" })))
    }
}

if ($Show) { Show-State; exit 0 }

# ---- remove the private-server overrides ----
if (Test-Path $BnetKeyPath) {
    Remove-ItemProperty -Path $BnetKeyPath -Name $ValueOverride -ErrorAction SilentlyContinue
    Write-Host "Removed $ValueOverride."
} else {
    Write-Host "$BnetKeyPath not present - nothing to remove there."
}

if (Test-Path $D2KeyPath) {
    Remove-ItemProperty -Path $D2KeyPath -Name $ValueBnetIp -ErrorAction SilentlyContinue
    Write-Host "Removed $ValueBnetIp (if it existed)."
}

# ---- optional: exact pre-change state from the set script's backup ----
if ($RestoreBackup) {
    if (Test-Path $backupPath) {
        & reg.exe import $backupPath
        if ($LASTEXITCODE -eq 0) {
            Write-Host "Imported backup: $backupPath"
        } else {
            Write-Warning "reg import failed ($LASTEXITCODE) - overrides were still removed above."
        }
    } else {
        Write-Warning "No backup file found at $backupPath - skipping (retail list was never modified, so this is fine)."
    }
}

Write-Host ""
Write-Host "Done. Real Battle.net gateways are active again (retail defaults are rebuilt"
Write-Host "from gateways.txt automatically if the retail list value was missing)."
Write-Host "Restart D2VP for the change to take effect."
Write-Host ""
Write-Host "Current state:"
Show-State
