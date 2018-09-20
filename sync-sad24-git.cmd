cd c:\sad24.rf-c
robocopy %userprofile%\Documents\Arduino\libraries\debug debug
robocopy %userprofile%\Documents\Arduino\libraries\gprs2 gprs2
robocopy %userprofile%\Documents\Arduino\libraries\mstr mstr
robocopy %userprofile%\Documents\Arduino\libraries\TimerOne TimerOne
robocopy %userprofile%\Documents\Arduino\libraries\worker worker
robocopy %userprofile%\Documents\Arduino\sad24v2\sad24v2.ino sad24.rf-c
git status
git add *
git commit -m %1
git push
git status
pause