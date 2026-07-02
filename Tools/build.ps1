# Build the BeastForgeKingdom editor target (portable: project path derived from
# this script's location; engine path via -Engine, $env:UE_ROOT, or default).
param(
    [switch]$Game,
    [string]$Engine = $(if ($env:UE_ROOT) { $env:UE_ROOT } else { 'D:\UE_5.8' })
)
$proj = Join-Path (Split-Path $PSScriptRoot -Parent) 'BeastForgeKingdom.uproject'
if (-not (Test-Path "$Engine\Engine\Build\BatchFiles\Build.bat")) {
    Write-Error "Engine not found at '$Engine'. Pass -Engine <path> or set UE_ROOT."
    exit 1
}
$target = if ($Game) { 'BeastForgeKingdom' } else { 'BeastForgeKingdomEditor' }
& "$Engine\Engine\Build\BatchFiles\Build.bat" $target Win64 Development -Project="$proj" -WaitMutex
exit $LASTEXITCODE
