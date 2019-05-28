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

def checkTime(newTime, oldTime):
    if newTime >= oldTime:
        timeDif = newTime - oldTime
    else:
        timeDif = (24 % oldTime) + newTime

    if timeDif >= 2:
        return True
    else:
        return False

def extractTime(time):
    ti = float(time) / 10000
    return int(ti)

def changeCycle(tCur, tLast):
    tCur = extractTime(tCur)
    tLast = extractTime(tLast)
    return checkTime(tCur, tLast)

pos = 0
countHi = 0
countLow = 0
wakeCycle = 0
expression = False
fTR = True #Used for printing first cycle in cases time stamp breaks alg used in initial setup
lastTime = 0


#decimalDegree(float(x.split(',')[5]))
#locations 3 and 5

for x in f:
    entry = x.split(',')
    pos += 1
    curTime = entry[2]

    if fTR == True:
        wakeCycle += 1
        print "Wake Cycle ", wakeCycle
        lastTime = curTime
        fTR = False

    if changeCycle(curTime, lastTime) == True:
        wakeCycle += 1
        print "Wake Cycle ", wakeCycle

    try:
        if entry[13] == 'R' and checkTen(entry[15]) == 10.0:
            countHi += 1

            if expression == False:
                print "Count Low: ", countLow
                countLow = 0
                expression = True

        else:
            countLow += 1

            if expression == True:
                print "Count Hi: ", countHi
                countHi = 0
                expression = False

    except:
        pass


    lastTime = curTime
