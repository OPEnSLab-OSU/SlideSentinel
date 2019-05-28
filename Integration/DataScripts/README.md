# These scripts are used to analyze RTK data files

To use these scripts you must have python3 and pip install xlwt
repo is here https://github.com/python-excel/xlwt

## createCSV
This file creates two .csv files. It creates a file containing a cell of all of the 
latitude locations and a file containing all longitude cells. The data file of which
latitude and longitude is being pulled from is oldData/GPS_LOGS.txt.

## createDecTableLat
Creates a csv file named latDecimalTable.csv. Creates a table to evaluate each
decimal received in a latitude reading. 

## createDecTableLong
Creates a csv file named longDecimalTable.csv. Creates a table to evaluate each decimal received in a longitude reading.

## createTimeTableCSV
Creates a csv file named timeLatLon.csv. Creates a table with three columns Latitude | Longitude | time stamp (HH:MM:SS) 

## evalTens
Outputs to console any x length strip of tens. Where x is the number of 10's that 
occur after each other. 

## evalTensCycle
Does the same thing as evalTens but takes into account of the wake cycle. Each cycle should contain some hi low combination. Displays how many hi or low rtk values occur right after each other



