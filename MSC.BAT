@echo off
cl /MT /Ox /O2 /Ob2 /c garlic.cpp random.cpp thread.cpp benchmark.cpp
rc /d_MT -r garlic.rc
link garlic.obj random.obj thread.obj benchmark.obj garlic.res user32.lib gdi32.lib comdlg32.lib comctl32.lib shell32.lib libeay32.lib advapi32.lib shlwapi.lib
if exist garlic.obj del garlic.obj
if exist random.obj del random.obj
if exist thread.obj del thread.obj
if exist benchmark.obj del benchmark.obj
if exist garlic.res del garlic.res
