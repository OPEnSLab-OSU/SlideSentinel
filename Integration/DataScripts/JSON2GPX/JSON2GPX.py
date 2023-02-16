################################################
#
#   JSON2GPX.py
#   Converts Slide Sentinel Json Data into GPX Waypoints to easily be plotted in QGIS
#   Just Run this from inside a folder of .json documents from Rovers to convert them to GPX waypoints
#   Author: Will Richards
#
################################################

import json
import glob
from gps_time.core import GPSTime

for file in glob.glob("./*.json"):
    with open(str(file[2:-5]) + ".gpx", "w") as gpxFile:
        # GPX Boiler Plate 
        gpxFile.write("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n")
        gpxFile.write("<gpx version=\"1.0\">\n")
        with open(file, "r") as dataFile:
            for line in dataFile:
                jsonPacket = json.loads(line)

                # Use the gps-time library to convert our gps time into a UTC time, convert this time to the standard T,Z format
                timeStr = str(GPSTime(week_number = int(jsonPacket["Week"]), time_of_week=int(str(jsonPacket["Seconds"])[:6])).to_datetime()).replace(" ", "T")
                timeStr += "Z"

                # Craft the waypoint object and write it to the file
                waypoint = "\t<wpt lat=\"{0}\" lon=\"{1}\">\n\t\t<name>{2}</name>\n\t\t<time>{2}</time>\n\t\t<sat>{3}</sat>\n\t\t<pdop>{4}</pdop>\n\t\t<geoidheight>{5}</geoidheight>\n\t</wpt>\n".format(jsonPacket["Latitude"], jsonPacket["Longitude"], timeStr, jsonPacket["Satellites"], jsonPacket["PDOP"], jsonPacket["Height"])
                gpxFile.write(waypoint)

        # Close the GPX document
        gpxFile.write("</gpx>")