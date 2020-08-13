$ErrorActionPreference = 'Stop';

$toolsDir     = "$(Split-Path -parent $MyInvocation.MyCommand.Definition)"
$url          = 'http://sourceforge.net/projects/nsis/files/NSIS%203/3.05/nsis-3.05-setup.exe'
$checksum     = '1a3cc9401667547b9b9327a177b13485f7c59c2303d4b6183e7bc9e6c8d6bfdb'
$checksumType = 'sha256'

$packageArgs = @{
  packageName    = $env:ChocolateyPackageName
  unzipLocation  = $toolsDir
  fileType       = 'exe'
  url            = $url
  checksum       = $checksum
  checksumType   = $checksumType
  softwareName   = 'Nullsoft Install System*'
  silentArgs     = '/S'
  validExitCodes = @(0,3010)
}

Install-ChocolateyPackage @packageArgs