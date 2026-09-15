param([string]$Qemu = 'D:\Programos\qemu\qemu-system-i386.exe', [string]$DataDisk = '')
$ErrorActionPreference = 'Stop'
$isoPath = Join-Path $PSScriptRoot 'build\release\felix-1.0-x86.iso'
if (-not (Test-Path -LiteralPath $isoPath)) { throw 'Build the ISO first. See README.md.' }
if (-not (Test-Path -LiteralPath $Qemu)) { $Qemu = (Get-Command qemu-system-i386.exe -ErrorAction Stop).Source }
$qemuArgs = @('-m', '512', '-cdrom', $isoPath, '-boot', 'd', '-vga', 'std', '-nic', 'user,model=e1000')
if ($DataDisk) {
    $diskPath = (Resolve-Path -LiteralPath $DataDisk).Path
    $qemuArgs += @('-drive', "file=$diskPath,format=raw,if=ide")
}
# This visible window is the interactive VM requested by the run command.
& $Qemu @qemuArgs
