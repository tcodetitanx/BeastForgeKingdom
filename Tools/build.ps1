# Build the BeastForgeKingdom editor target.
param([switch]$Game)
$engine = 'D:\UE_5.8'
$proj = 'D:\UEprojects\BeastForgeKingdom\BeastForgeKingdom.uproject'
$target = if ($Game) { 'BeastForgeKingdom' } else { 'BeastForgeKingdomEditor' }
& "$engine\Engine\Build\BatchFiles\Build.bat" $target Win64 Development -Project="$proj" -WaitMutex
exit $LASTEXITCODE
