#!/usr/bin/python

from accuracy import test_accuracy as tAcc
from ttff import test_ttff as tttff
from availability import test_availability as tAva

class test_suite():
    def __init__(self, f_name, surveyLat, surveyLon, tInterval):
        self.testAccuracy = tAcc(f_name, surveyLat, surveyLon)
        self.testTTFF = tttff(f_name, tInterval)
        self.testAvailability = tAva(f_name)

    def conduct_tests(self):

        accuracy = self.testAccuracy.event_accuracy()
        TimeTFF = self.testTTFF.event_ttff()
        availability = self.testAvailability.event_availability()

        #write stuff to file maybe for now just print to stdout
        print("Accuracy: " + str(accuracy) +"m")
        print("TTFF: " + str(TimeTFF) +"s")
        print("Availability Fixed-Int: " + str(availability) + "%")

def main():

    surveyedLatitude = 4433.9958187
    surveyedLongitude = 12317.6156572
    fileNameAssessing = "GPGGA.TXT"
    timeIntervalForTTFF = 20
    tSuite = test_suite(fileNameAssessing, surveyedLatitude, surveyedLongitude, timeIntervalForTTFF)

    tSuite.conduct_tests()


    """
    #Testing TTFF time difference between times WORKS
    #Need to add: TTFF to float
    temp = tttff("GPGGA.TXT")
    #print(str(temp.convert_to_time_string("162651.000")))
    t1 = "162651.000"
    t2 = "162701.000"
    timeDif = float(temp.time_difference_in_seconds(t1, t2))

    if timeDif < 5:
        print("Time " + str(timeDif))
    temp = tttff("test.txt")

    print(str(temp.event_ttff()))
    """

    """
    #Tested availability WORKS
    test = tAva("test.txt")
    aArray = []
    aArray = test.event_availability()
    for times in aArray:
        print("Time: " + str(times))

    """
    """
    test = tAva("GPGGA.TXT")
    temp = test.event_availability()

    print("Time Avg: " + str(temp) + "%")
    """


main()
