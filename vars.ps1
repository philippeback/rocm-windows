$vcvars = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
cmd /c "`"$vcvars`" && set" | ForEach-Object {
    if ($_ -match '^(.*?)=(.*)$') {
        Set-Content "env:$($matches[1])" $matches[2]
    }
}
Write-Host "Environnement MSVC x64 charge."