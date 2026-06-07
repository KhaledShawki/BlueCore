param(
    [Parameter(Position = 0, Mandatory = $true)]
    [ValidateSet("add-file", "remove-file", "rename-file", "add-project")]
    [string] $Command,

    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]] $RemainingArgs
)

$RootDir = Resolve-Path "$PSScriptRoot\.."
$Premake = Join-Path $RootDir "scripts\premake-windows.cmd"

& $Premake "blue-$Command" @RemainingArgs
exit $LASTEXITCODE
