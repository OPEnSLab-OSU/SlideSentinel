#!/usr/bin/python
import math
import xlwt

f= open("oldData/GPS_LOGS.TXT","r")

def decimalDegree(ddm):
    deg = math.floor(ddm / 100)
    min = (ddm-(deg*100)) / 60
    return(deg + min)

def timeConv(tim):
    temp = str(tim)
    temp2 = "{}{}:{}{}:{}{}".format(temp[0],temp[1],temp[2],temp[3],temp[4],temp[5])
    return temp2

#decimalDegree(float(x.split(',')[5]))
#locations 3 and 5
wb = xlwt.Workbook()
ws = wb.add_sheet('latlontime')


ws.write(0, 0, "Latitude")
ws.write(0, 1, "Longitude")
ws.write(0, 2, "Time HH:MM:SS")


pos = 0
for x in f:
    entry = x.split(',')
    try:
        if entry[8] == 'R' and float(entry[10].rstrip()) == 10.0:
            pos += 1
            lat = decimalDegree(float(entry[3]))
            lon = decimalDegree(float(entry[5]))
            timeLatLon = timeConv(float(entry[2]))
            ws.write(pos, 0, lat)
            ws.write(pos, 1, lon)
            ws.write(pos, 2, timeLatLon)

    except:
        pass

wb.save('csvFiles/timeLatLon.csv')
