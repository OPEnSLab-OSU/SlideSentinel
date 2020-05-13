#!/usr/bin/python
"""
#NMEA-0183 Structure for GGA messages
#   0      1            2          3        4           5  6   7   8    9   10  11  11  12   13
#$GPGGA, HHMMSS, ddmm.mmmmmmmmmmm, a, ddmm.mmmmmmmmmmm, a, x, xx, x.x, x.x, M, x.x, M, x.x, xxxx*hh
Entry Pos   Name
    0       NMEA Format ID
    1       UTC Time
    2       Latitude
    3       N/S Indicator
    4       Longitude
    5       E/W Indicator
    6       GPS Quality Indicator: 4 == Fixed Integer, 5 == Floating Integer
    7       Satellites Used
    8       HDOP
    9       Altitude
    10      Geoidal Seperation
    11      Age of Differential GPS data
    12      DGPS Station ID
    13      Checksum
"""
import math
from datetime import datetime

class test_availability():
    def __init__(self, file_name):
        self.fname = file_name
        self.ttff_obj = tttff("NULL", 0)


    def event_continuity(self):
        f = open(self.fname, "r")

        availabilityArray = []
        trackTime = False
        beginTime = 0
        for x in f:
            entry = x.split(',')
            try:
                if int(entry[6]) == 5:
                    if trackTime == False:
                        trackTime = True
                        beginTime = entry[1]

                else:
                    if trackTime == True:
                        availabilityArray.append(int(self.ttff_obj.time_difference_in_seconds(beginTime, entry[1])))

                    trackTime = False
            except:
                pass

        return availabilityArray
