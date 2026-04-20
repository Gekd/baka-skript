# baka-skript

Wildnav: https://github.com/TIERS/wildnav

## Dependencies

Following system libraries are required:
1. Libcurl, for installing https://curl.se/libcurl/
2. OpenCV, for installing https://docs.opencv.org/4.x/df/d65/tutorial_table_of_content_introduction.html
3. PROJ, for installing https://proj.org/en/stable/install.html#install

## Steps
1. `mkdir /data`
2. Add all the directories from Mavic 3T drone /DCIM to /data

DJI log files are stored in remote controller and are encrypted from version 13 and above. To extract '.csv' files from CLI you need to have access to DJI API key, if not then online log viewers can help.

3. Add extracted .csv files to /data directory 
4. `mkdir /build`
5. `cd /build`
6. `cmake ..`
7. `make`
8. `./skript --timezone 2 -w`
