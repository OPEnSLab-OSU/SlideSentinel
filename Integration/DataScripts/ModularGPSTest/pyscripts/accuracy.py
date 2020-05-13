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
import geopy.distance
import math

class test_accuracy:
    def __init__(self, file_name, _surveyLat, _surveyLon):
        self.fname = file_name
        self.surveyLat = self.decimalDegree(_surveyLat)
        self.surveyLon = self.decimalDegree(_surveyLon)

    def decimalDegree(self, ddm):
        deg = math.floor(ddm / 100)
        min = (ddm-(deg*100)) / 60
        return(deg + min)

    def distance_between_GPS_coords(self, lat1, lon1):
        lat1 = self.decimalDegree(lat1)
        lon1 = self.decimalDegree(lon1)

        coord1 = (lat1, lon1)
        coord2 = (self.surveyLat, self.surveyLon)

        return geopy.distance.vincenty(coord1, coord2).meters

    def event_accuracy(self):
        f = open(self.fname, "r")

        counter = 0
        sum = 0
        for x in f:
            entry = x.split(',')
            try:
                if int(entry[6]) == 4:
                    counter += 1
                    sum += self.distance_between_GPS_coords(float(entry[2]), float(entry[4]))

            except:
                pass

        average = sum / counter

        return average
