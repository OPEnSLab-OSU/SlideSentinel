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

class test_ttff:
    def __init__(self, file_name, eTime):
        self.fname = file_name
        self.comparativeElapsedTime = eTime

    def split_time_string(self, _time):
        temp = _time.split('.')

        return temp[0]

    def convert_to_time_string(self, _time):
        time_ = self.split_time_string(_time)
        FMT = '%H%M%S'

        return datetime.strptime(time_, FMT)

    def time_difference_in_seconds(self, t1, t2):
        time1 = self.convert_to_time_string(t1)
        time2 = self.convert_to_time_string(t2)

        timeDifference = time2 - time1
        return timeDifference.total_seconds()

    def event_ttff(self):
        f = open(self.fname, "r")

        trackTime = False
        firstEntry = False
        beginTimer = currentTime = recordingStartTime = 0
        ttfftime = count = 0
        for x in f:
            count += 1
            entry = x.split(',')
            try:
                if firstEntry == False:
                    recordingStartTime = entry[1]
                    firstEntry = True

                if int(entry[6]) == 5:
                    if trackTime == False:
                        beginTime = entry[1]
                        trackTime = True

                    elapsedTimeForFix = float(self.time_difference_in_seconds(beginTime, entry[1]))

                    if elapsedTimeForFix > self.comparativeElapsedTime: #if there was a fix for 20 seconds
                        ttfftime = self.time_difference_in_seconds(recordingStartTime, entry[1])
                        break

                #There was a break in continuity of a fixed integer solution so reset trackTime
                else:
                    trackTime = False

            except:
                pass

        return ttfftime
