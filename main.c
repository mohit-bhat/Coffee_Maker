
/** @file
* @brief Coffee Maker Project
* @defgroup Coffee_Maker
* Author, Mohit Bhat solely
* Main file for coffee maker project
*/

#include <stdbool.h>
#include <stdint.h>

#include "nrf.h"
#include "nrf_delay.h"
#include "nordic_common.h"
#include "boards.h"
#include "Drive.c"

/**
 * @brief Function for application main entry.
 */
int main(void)
{
    bsp_board_init(BSP_INIT_LEDS); //LED for fun

    //initialize input/output
    allInitializations();

    //get initial config
    coffeeConfig *config = pollWireless();

    //run coffee maker
    while (true)
        runCoffeeMaker(config);

    free(config);
}
/** @} */
