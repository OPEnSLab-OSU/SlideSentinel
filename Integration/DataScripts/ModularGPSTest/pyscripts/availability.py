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
from decimal import Decimal, ROUND_UP
from ttff import test_ttff as tttff

class test_availability():
    def __init__(self, file_name):
        self.fname = file_name
        self.ttff_obj = tttff("NULL", 0)

    def acquire_average_fixed_int(self, aArray, startT, finalT):
        sumTime = 0
        counter = 0
        for times in aArray:
            counter += 1
            sumTime += times

        totalTimeOfFileRecordings = float(self.ttff_obj.time_difference_in_seconds(startT, finalT))

        return sumTime / totalTimeOfFileRecordings

    def event_availability(self):
        f = open(self.fname, "r")

        availabilityArray = []
        trackTime = False
        firstEntry = False

        beginTime = lastTime = finalTime = recordingStartTime = 0

        for x in f:
            entry = x.split(',')

            try:
                if firstEntry == False:
                    recordingStartTime = entry[1]
                    firstEntry = True

                if int(entry[6]) == 4:
                    if trackTime == False:
                        trackTime = True
                        beginTime = entry[1]

                    lastTime = entry[1]
                else:
                    if trackTime == True:
                        if self.ttff_obj.time_difference_in_seconds(beginTime, lastTime) == 0:
                            availabilityArray.append(1)
                        else:
                            availabilityArray.append(int(self.ttff_obj.time_difference_in_seconds(beginTime, lastTime)))

                    trackTime = False
            except:
                pass

            finalTime = entry[1]

        if trackTime == True:
            availabilityArray.append(int(self.ttff_obj.time_difference_in_seconds(beginTime, lastTime)))

        avg = self.acquire_average_fixed_int(availabilityArray, recordingStartTime, finalTime)
        newAvg = Decimal(avg*100).quantize(Decimal(".01"), rounding=ROUND_UP)

        return newAvg
