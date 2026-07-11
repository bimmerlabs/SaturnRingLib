:; for d in */ ; do echo "$d"; ( cd "$d" && ./compile.bat release ); rc=$?; if [ $rc -ne 0 ]; then echo "Build failed in $d with errorlevel $rc."; exit $rc; fi; done; exit;
@ECHO Off
set back=%cd%
for /d %%i in (.\*) do (
    cd "%%i"
    echo "%%i";
    compile.bat release
    if errorlevel 1 (
        echo Build failed in "%%i" with errorlevel %ERRORLEVEL%.
        cd ..
        exit /b %ERRORLEVEL%
    )
    cd ..
)
