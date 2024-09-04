# CC1101_Signal_Sniffer
Uses an esp32-OLED board and CC1101 module to sniff &amp; display 300, 400 &amp; 800 ISM frequencies

This projects uses smolbun/CC1101-Frequency-Analyzer code but instead of sending it to the serial port, it displays it on an ESP32/OLED module.
The ESP/OLED module is the HW-724 from Shenzhen Hongwei Microelectronics Co and purchased from Aliexpress.
The program displays 3 sets of frequency/strength readings and then clears the display and starts again for the next 3 readings, if any.

I actually created this as a quick test for my CC1101 module as it was not receiving signals very well.

