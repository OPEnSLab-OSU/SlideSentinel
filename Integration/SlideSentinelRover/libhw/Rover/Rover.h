#ifndef _Rover_H_
#define _Rover_H_
#include "MAX4280.h"


/**
 * @brief The Rover class is responsible for controlling all subparts of the rover.
 *  
 */

class Rover {

public:
    Rover();

    /* Tells the max4820 to enable the radio relay. */
    void powerRadio();

    /* Tells the max4820 to disable the radio relay. */
    void powerDownRadio();

private:
    MAX4280 max4280;
};

#endif // _Rover_H_
