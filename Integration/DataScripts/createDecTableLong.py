#!/usr/bin/python
import math
import xlwt

f= open("oldData/GPS_LOGS.TXT","r")
w, h = 13, 13;
decimalsLon = [[0 for x in range(w)] for y in range(h)]

def getDecTitle(num):
    switcher = {
        0: "First",
        1: "Second",
        2: "Third",
        3: "Fourth",
        4: "Fifth",
        5: "Sixth",
        6: "Seventh",
        7: "Eigth",
        8: "Ninth",
        9: "Tenth",
        10: "Eleventh",
        11: "Twelveth",
        12: "Thirteenth",
    }
    return switcher.get(num, "No Val")

def evalDecimals(deg, decimal):#adds to locations counter 0-9 for decimal position decimal
    decimalsLon[decimal][deg] += 1

def helperLong(degree):
    v = len(degree) - 4
    for i in range(v):
        evalDecimals(int(degree[i + 4]), i)

def decimalDegree(ddm):
    deg = math.floor(ddm / 100)
    min = (ddm-(deg*100)) / 60
    return(deg + min)

#decimalDegree(float(x.split(',')[5]))
#locations 3 and 5
wb = xlwt.Workbook()
ws = wb.add_sheet('DecimalTableLongitude')

for x in f:
    entry = x.split(',')
    try:
        if entry[8] == 'R' and float(entry[10].rstrip()) == 10.0:
            lon = decimalDegree(float(entry[5]))
            helperLong(str(lon))

    except:
        pass


for i in range(w):
    colTitle = "Decimal: %d" % (i + 1)
    decTitle = getDecTitle(i)
    if i == 0:
        ws.write(0, i, colTitle)
        ws.write(0, 1, decTitle)
    else:
        ws.write(0, 2*i, colTitle)
        ws.write(0, 2*i+1, decTitle)

    for x in range(10):
        rowDec = "%d: " % (x)
        rowCount = str(decimalsLon[i][x])
        if i == 0:
            ws.write(x+1, i, rowDec)
            ws.write(x+1, 1, rowCount)
        else:
            ws.write(x+1, i*2, rowDec)
            ws.write(x+1, i*2+1, rowCount)

wb.save('csvFiles/longDecimalTable.csv')
