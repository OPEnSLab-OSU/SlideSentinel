#ifndef _PROPERTIES_H_
#define _PROPERTIES_H_

#include <Arduino.h>
#include "ArduinoJson.h"

#define SS_PROP "PROP"
#define MAX_PROP_LEN 200  
#define NUM_PROP 7        ///< Number of diagnostics (size of array)
#define INVALID_PROP -1   ///< If we do not want to change a certain property when sending new props from base, we send a -1 in that index of the array
#define TIMEOUT 0         ///< Timeout property (For radio)
#define RETRIES 1         ///< Num retries before fail (for radio)
#define WAKE_TIME 2       ///< Rover wake time (in minutes)
#define SLEEP_TIME 3      ///< Rover sleep time (in minutes)
#define SENSITIVITY 4     ///< Accelerometer sensitivty (0-FF, FF being highest sensitivity)
#define LOG_FREQ 5        ///< Rate at which GNSSController will accept positional data from piksi.
#define THRESHOLD 6       ///< Minimum aceptable RTK mode. Example: val of 3 means only RTK Floats or fixes will be accepted, while a value of 0 means all polled data will be accepted.


/**
 * The Property class is responsible for containing all of the properties the system uses. 
 * 'Properties' are essentially configurable options for the system. Refer to the macros at 
 * the top of this file for an explanation of each property. The class contains functions for 
 * getting/setting props, as well as serializing the property data (To be sent over radio to base).
 */
class Properties {
private:
  int m_prop[NUM_PROP]; ///< array containg all of the properties. Refer to macros for what each index represents.

public:

  /**
   * Constructor for properties class. Does nothing. 
   */
  Properties();

  /**
   * Creates a serialized JSON array of the property values. 
   * 
   * @param doc Reference to JSON document that the serialized proeprty data gets added to.
   */
  void write(JsonDocument &doc);

  /**
   * Reads serialized JSON array of property data sent from the base and 
   * updates the properties in this class's m_prop[] array. The function 
   * deserializes the data and sets the data in m_prop[] only if the value is 
   * not equal to -1 (meaning the old value of the property is kept).
   * 
   * @param buf String of properties to be desrialized and parsed into m_prop array.
   */
  void read(char* buf);

  /**
   * Getter function for property array, m_prop[].
   * 
   * @param prop index of array to obtain value from.
   * @return the value of the property specified by the index.
   */
  int get(int prop);

  /**
   * Setter function for property array, m_prop[]. Sets the value if 
   * it is not equal to -1, meaning it will keep the old prop value.
   * 
   * @param prop index of m_prop[] to set
   * @param val new property value to be set in m_prop[] @prop index.
   */
  void set(int prop, int val);

  /**
   * Initializes all of the properties in the array m_prop[] to -1.
   */
  void init();

  /**
   * Prints out all of the current rover properties to the Serial monitor.
   * This generally used for debugging purposes.
   */
  void print();
};

#endif // _PROPERTIES_H_
