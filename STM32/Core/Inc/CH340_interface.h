#ifndef CH340_INTERFACE_H
#define CH340_INTERFACE_H

#include "main.h"
#include "MPU6050_interface.h"

//This data strucktre is used to set which symblo is drawn.
enum Mode {
    MODE_1 = 0,//Symbol krzyża
    MODE_2 = 1,//Symbol koła
    MODE_3 = 2,//do lazienki musze isc po migowemu
    NUMBER_OF_MODES
};



#endif /* CH340_INTERFACE_H */