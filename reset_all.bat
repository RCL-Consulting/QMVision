@echo off
setlocal

echo Resetting all vendored git repositories...

for /d %%D in (external\*) do (
    if exist "%%D\.git" (
        echo --------------------------------------------------
        echo Resetting repo: %%D
        pushd %%D
        git reset --hard HEAD
        git clean -fdx
        popd
    )
)

echo --------------------------------------------------
echo Done resetting all repositories.
pause
