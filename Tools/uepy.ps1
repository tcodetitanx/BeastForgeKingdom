# Run a python script headlessly inside the UE editor context.
param([Parameter(Mandatory=$true)][string]$Script)
$engine = 'D:\UE_5.8'
$proj = 'D:\UEprojects\BeastForgeKingdom\BeastForgeKingdom.uproject'
& "$engine\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" $proj -run=pythonscript -script="$Script" -unattended -nosplash -stdout -NoLogTimes
exit $LASTEXITCODE
