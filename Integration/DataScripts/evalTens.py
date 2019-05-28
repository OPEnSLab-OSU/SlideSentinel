#!/usr/bin/python
import math
import xlwt

f= open("data/ALL.TXT","r")

def decimalDegree(ddm):
    deg = math.floor(ddm / 100)
    min = (ddm-(deg*100)) / 60
    return(deg + min)

def checkTen(nmea):
    temp = nmea.split('*')
    return float(temp[0])

#decimalDegree(float(x.split(',')[5]))
#locations 3 and 5


pos = 0
countHi = 0
expression = False

for x in f:
    entry = x.split(',')
    pos += 1

    try:
        if entry[13] == 'R' and checkTen(entry[15]) == 10.0:
            countHi += 1
            expression = True

        else:
            if expression == True:
                pos2 = pos - countHi
                print "At Line: ", pos2
                print "Count Hi: ", countHi
                countHi = 0
                expression = False

    except:
        pass
