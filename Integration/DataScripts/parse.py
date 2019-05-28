#!/usr/bin/python
import math
import xlwt

f= open("GPS_LOGS.TXT","r")
w, h = 13, 13;
decimalsLat = [[0 for x in range(w)] for y in range(h)]

def evalDecimals(deg, decimal):#adds to locations counter 0-9 for decimal position decimal
    decimalsLat[decimal][deg] += 1

def helperLat(degree):#evals each decimal
    v = len(degree) - 3
    for i in range(v):
        evalDecimals(int(degree[i + 3]), i)

def decimalDegree(ddm):
    deg = math.floor(ddm / 100)
    min = (ddm-(deg*100)) / 60
    return(deg + min)

#decimalDegree(float(x.split(',')[5]))
#locations 3 and 5
#wbLa = xlwt.Workbook()
#wbLo = xlwt.Workbook()
#wsLa = wbLa.add_sheet('Latitude')
#wsLo = wbLo.add_sheet('Longitude')

#wsLa.write(0, 0, "Latitude")
#wsLo.write(0, 0, "Longitude")


def helperLong(degree):
    v = len(degree) - 4
    for i in range(v):
        evalDecimals(int(degree[i + 4]), i)


pos = 0
for x in f:
    entry = x.split(',')
    try:
        if entry[8] == 'R' and float(entry[10].rstrip()) == 10.0:
            pos += 1
            lat = decimalDegree(float(entry[3]))
            lon = decimalDegree(float(entry[5]))
            helperLat(str(lat))
            #helperLong(str(lon))
            #wsLa.write(pos, 0, lat)
            #wsLo.write(pos, 0, lon)

            #print(decimalDegree(float(entry[3])))       #latitude
            #print("\n")
            #print(decimalDegree(float(entry[5])))      #longitude
            #print("\n")
    except:
        pass


for i in range(w):
    print("Decimal: " + str(i + 1))
    for x in range(10):
        print(str(x) + ": " + str(decimalsLat[i][x]))


#wbLa.save('latitude.csv')
#wbLo.save('longitude.csv')
