@echo off
rem Runs the WebAssembly build of Myna. Add this folder to PATH to type "myna".
node --no-warnings "%~dp0run.mjs" %*
