# Modular Receiver Tests
This modular test will be available for testing any gps receiver with a Base and Rover combination utlizing the Slide Sentinel interface. The point of these tests will assess what enviornments the gps receiver would be available for deployment. The tests will also assess whether they can retain a sub-centimeter reading which is a necessary for the Slide Sentinel project. 

## Receivers Included in initial tests	
#### S2525F8-GL-RTK
#### Piksi Multi 

## Whats being tested?
### Accuracy
This will test how accurate the GPS Receiver is at collecting a gps coordinate. This will assess whether the GPS Receiver is collecting sub centimeter readings. The code for this test is written in accuracy.py.
### Continuity
This will test the average time it takes for a GPS Receiver to regain a Fixed-Integer solution after it has been lossed. The purpose of this is to test how long it takes the receiver to regain a high quality solution for a gps coordinate. This test has yet to be created but will be created in continuity.py.
### Availability
Availability tests the total amount of time the receiver stays in a Fixed-Integer solution as a percentage. This will tell how often a Fixed-Integer solution occurs throughout the whole duration of a Rovers gps coordinate gathering cycle. The code for this test is written in availability.py.
### Time To First Fix 
Time to first fix(TTFF) finds the amount of time it takes to get a Fixed-Integer solution and subsequently hold for a allotted time. In the data that is analyzed; there are occurrences of a Fixed-Integer solution that doesn't best represent an accurate reading. Thus, a time threshold for how long the first Fixed-Integer holds will most likely have a more accurate reading. The code for this test is written in ttff.py. 
## Procedure For Conducting Tests
### Getting Data
A survey grade gps must be used to compare the quality of the gps readings gathered by the receivers being tested. The GPS will be tested in three areas: open sky, minor obtrusion, major obtrusion. A total of four points will be recorded by the survey gps: Central location for the Base, and the three other positions recently discussed. Then test each position with one Base and Rover keeping the Base in a static open sky location (preferably in a central location to all three test positions). Run the gps receivers within the Slide Sentinel interface hot for one hour. NMEA-0183 strings will be saved to a SD card provided in the Slide Sentinel Interface. Once all three locations are surveyed and tested. Sufficient data has been gathered to test the GPS Receiver via Testing Scripts.
### Testing Scripts
#### How they work
testEnv.py is going to be the main testing enviornment where you enter a few pieces of information. First, the data must be extracted from the Slide Sentinel Interface and uploaded into the same folder as the test scripts. Note: The current test scripts only evaluate GPGGA formatted NMEA strings. Then within testEnv set the surveyLatitude and surveyLongitude to the gps coordinates recorded per location by the survey grade gps. Run testEnv script via console and the results will be printed to console.

