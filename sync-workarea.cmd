################################################################
# Скрипт для синхронизации библиотек в MS Windows              #
################################################################
cd c:\sad24.rf-c

robocopy gprs2 %userprofile%\Documents\Arduino\libraries\gprs2
robocopy mstr %userprofile%\Documents\Arduino\libraries\mstr
robocopy worker %userprofile%\Documents\Arduino\libraries\worker

robocopy TimerOne %userprofile%\Documents\Arduino\libraries\TimerOne
robocopy DHT_sensor_library %userprofile%\Documents\Arduino\libraries\DHT_sensor_library
robocopy bmp085 %userprofile%\Documents\Arduino\libraries\bmp085
robocopy DS3231 %userprofile%\Documents\Arduino\libraries\DS3231
robocopy LowPower %userprofile%\Documents\Arduino\libraries\LowPower

