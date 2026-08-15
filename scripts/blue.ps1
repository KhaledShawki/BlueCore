$RootDir = Resolve-Path "$PSScriptRoot\.."
$BlueCli = Join-Path $RootDir "scripts\blue.py"

& python $BlueCli @args
exit $LASTEXITCODE
