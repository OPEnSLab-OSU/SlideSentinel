#!/usr/bin/python
import math
import xlwt

f= open("oldData/GPS_LOGS.TXT","r")

def decimalDegree(ddm):
    deg = math.floor(ddm / 100)
    min = (ddm-(deg*100)) / 60
    return(deg + min)

#decimalDegree(float(x.split(',')[5]))
#locations 3 and 5
wbLa = xlwt.Workbook()
wbLo = xlwt.Workbook()
wsLa = wbLa.add_sheet('Latitude')
wsLo = wbLo.add_sheet('Longitude')

wsLa.write(0, 0, "Latitude")
wsLo.write(0, 0, "Longitude")

pos = 0
for x in f:
    entry = x.split(',')
    try:
        if entry[8] == 'R' and float(entry[10].rstrip()) == 10.0:
            pos += 1
            lat = decimalDegree(float(entry[3]))
            lon = decimalDegree(float(entry[5]))
            wsLa.write(pos, 0, lat)
            wsLo.write(pos, 0, lon)

    except:
        pass

wbLa.save('csvFiles/latitude.csv')
wbLo.save('csvFiles/longitude.csv')
