#ifndef _CONMANAGER_H_
#define _CONMANAGER_H_
#define MAX_CONTROLLERS 10

#include <Arduino.h>
#include "Controller.h"
#include "SSModel.h"

/**
 * The ConManager class contains an array of all the controllers
 * used for the rover. The controllers are as follows:
 * COMController, FSController, GNSSController, RTCController, 
 * IMUController, and PMController. 
 * Each of these controllers contain functions called 'status()' 
 * and 'update' which are used to update property/diagnostic info. 
 * This class makes it very easy to collect diagnostic data of the 
 * controllers as well as update their proeperties.
 */
class ConManager {
private:
  int m_size; ///< # of controllers, used for indexing
  Controller *m_controllers[MAX_CONTROLLERS]; ///< Array of pointers to each controller, derived from Controller class.

public:

  /**
   * Class constructor for the ConManager. This simply initializes the m_size 
   * variable to 0.
   */
  ConManager(); 

  /**
   * This adds a ptr to the array of controllers, m_controllers. This function is called
   * in the setup process of main().
   * @param con ptr to the controller object to be added to the array.
   */
  void add(Controller *con);

  /**
   * Loops through the array of controllers, calling each of their 'status()' functions. 
   * The status() function in each controller updates the diagnostic info in SSModel.cpp.
   * @param model reference to SSModel object. This is passed in as an arg to each status() in the controllers.
   */
  void status(SSModel &model);

  /**
   * Updates each of the controllers with the properties set in SSModel by calling each controller's 
   * 'status()' function. This called in main() after each handshake in case new property info was sent 
   * from the base station. 
   * This ensures each controller has the most updated properties before polling. 
   * @param model reference to SSModel object. This is passed in as an arg to each status() in the controllers.
   */
  void update(SSModel &model);

  /**
   * Initializes each controller, loops through array of controllers calling init() for each one. 
   * NOTE: The last controller in the array is the FSController. This is intentional, because if the SD card
   * fails to intialize then we don't want to prevent the system from functioning. If this function returns 
   * false, then the system will fail.
   * @return True if all controllers initialize (except FSController), False otherwise. 
   */
  bool init();
};

#endif // _CONMANAGER_H_