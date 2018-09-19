cd c:\sad24.rf-c
robocopy %userprofile%\Documents\Arduino\libraries\debug C:\sad24.rf-c\debug
robocopy %userprofile%\Documents\Arduino\libraries\gprs2 C:\sad24.rf-c\gprs2
robocopy %userprofile%\Documents\Arduino\libraries\mstr C:\sad24.rf-c\mstr
robocopy %userprofile%\Documents\Arduino\libraries\TimerOne C:\sad24.rf-c\TimerOne
robocopy %userprofile%\Documents\Arduino\libraries\worker C:\sad24.rf-c\worker
robocopy %userprofile%\Documents\Arduino\sad24v2\sad24v2.ino C:\sad24.rf-c
git status
git add *
git commit -m %1
git push
git status
pause