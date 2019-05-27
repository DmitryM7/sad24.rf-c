REM ######################## Programs ##############################
robocopy %userprofile%\Documents\Arduino\sad24v2.ino sad24v2.ino
robocopy %userprofile%\Documents\Arduino\ArduinoISP  ArduinoISP
robocopy %userprofile%\Documents\Arduino\parilka     parilka
robocopy %userprofile%\Documents\Arduino\parilkaV2   parilkaV2
robocopy %userprofile%\Documents\Arduino\WaterSensor WaterSensor

REM ################################################################

REM ########### Own library ########################################
robocopy %userprofile%\Documents\Arduino\libraries\gprs2 gprs2
robocopy %userprofile%\Documents\Arduino\libraries\mstr mstr
robocopy %userprofile%\Documents\Arduino\libraries\worker worker
REM #################################################################

REM ### From other source libraries ###
robocopy %userprofile%\Documents\Arduino\libraries\bmp085 bmp085
robocopy %userprofile%\Documents\Arduino\libraries\DHT_sensor_library DHT_sensor_library
robocopy %userprofile%\Documents\Arduino\libraries\TimerOne TimerOne
robocopy %userprofile%\Documents\Arduino\libraries\LowPower LowPower
robocopy %userprofile%\Documents\Arduino\libraries\MemoryFree-master MemoryFree-master
robocopy %userprofile%\Documents\Arduino\libraries\DS3231 DS3231
REM ####################################

