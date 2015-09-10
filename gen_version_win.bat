git describe --all --tags --long
if %ERRORLEVEL% NEQ 0 GOTO :error

for /f "delims=" %%i in ('git describe --all --tags --long') do echo #define OPENAPOC_VERSION "%%i" > version.h
GOTO :end

:error
echo #define OPENAPOC_VERSION "0.0-git_not_in_path" > version.h
GOTO :end

:end
