#ifndef DRIVE_H__
#define DRIVE_H__

//headers
#include <stdbool.h>
#include <stdint.h>

#include "nrf.h"
#include "nrf_delay.h"
#include "nordic_common.h"
#include "boards.h"
#include "app_error.h"
#include "bsp.h"
#include "app_pwm.h"
#include "nrf_gpio.h"
#include "nrf_drv_saadc.h"
#include "app_util_platform.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_drv_power.h"
#include "nrf_drv_clock.h"
#include "nrf_drv_rtc.h"

//run settings
static bool runCycle = true;
static double temperatureGoal = 100.0;
static int runTimeSec = 60 * 6;
static int delayTimeSec = 2;

//internal settings
static double lowerTempLimit = 10.0;
static int lowerRunTimeLimit = 60 * 5;
static int upperRunTimeLimit = 60 * 60 * 24;
static uint32_t relayPin = NRF_GPIO_PIN_MAP(0, 19);
static uint32_t pwmPin = NRF_GPIO_PIN_MAP(0, 20);

//saadc things
#define SAADC_CALIBRATION_INTERVAL 5			 //Determines how often the SAADC should be calibrated relative to NRF_DRV_SAADC_EVT_DONE event. E.g. value 5 will make the SAADC calibrate every fifth time the NRF_DRV_SAADC_EVT_DONE is received.
#define SAADC_SAMPLES_IN_BUFFER 4				 //Number of SAADC samples in RAM before returning a SAADC event. For low power SAADC set this constant to 1. Otherwise the EasyDMA will be enabled for an extended time which consumes high current.
#define SAADC_OVERSAMPLE NRF_SAADC_OVERSAMPLE_4X //Oversampling setting for the SAADC. Setting oversample to 4x This will make the SAADC output a single averaged value when the SAMPLE task is triggered 4 times. Enable BURST mode to make the SAADC sample 4 times when triggering SAMPLE task once.
#define SAADC_BURST_MODE 1						 //Set to 1 to enable BURST mode, otherwise set to 0.

static nrf_saadc_value_t m_buffer_pool[2][SAADC_SAMPLES_IN_BUFFER];
static uint32_t m_adc_evt_counter = 0;
static bool m_saadc_calibrate = false;

/** @brief struct for storing coffee maker config data*/
typedef struct
{
	bool runCycle;
	double temperatureGoal;
	int runTimeSec;
	int delayTimeSec;
} coffeeConfig;

/** @brief Create the instance "PWM1" using TIMER1.		From PWM Library in SDK*/
APP_PWM_INSTANCE(PWM1, 1);

/** @brief A flag indicating PWM status.				From PWM Library in SDK*/
static volatile bool ready_flag;

/** @brief PWM callback function						From PWM Library in SDK */
void pwm_ready_callback(uint32_t pwm_id);

//initializations

/** @brief initializes pwm */
void pwmInit();

/** @brief initializes relay */
void relayGPIOInit();

/** @brief callback for saadc */
void saadc_callback(nrf_drv_saadc_evt_t const *p_event);

/** @brief initializes saadc */
void saadc_init();

/** @brief initializes temperature sensor */
void tempSensorInit();

/** @brief Placeholder function
 * teamate who was to write LCD functions not present
 */
void LCDInit();

/** @brief Placeholder function
 * teamate who was to write wireless functions not present
 */
void wirelessInit();

/** @brief runs all initialization functions */
void allInitializations();

//runtime functions

/** @brief returns adjusted duty cycle corrosponding to new temperature */
double dutyCycleChange(double temperatureGoal, double inputTemperature, double dutyCycle);

/** @brief converts adc value to degrees celsius */
double tempConversion(uint64_t inputValue);

/** @brief polls the adc connected to temperatrue sensor and returns value */
double pollTemp();

/**
 * @brief Placeholder function
 * teamate who was to write a function to display to LCD not present
 */
void updateLCD(double temperature);

/** @brief starts pwm cycle and maintains temperature until run time finished */
void runPWM(double temperatureGoal, int runTimeSec);

/** @brief sets relay to be on or off */
void setRelay(bool on);

/**
 * @brief Placeholder function
 * teamate who was to write wireless functions not present
 */
coffeeConfig *pollWireless();

/** @brief Placeholder function
 * teamate who was to write wireless functions not present
 */
void resetWirelessRegisters();

/**
 * @brief runs coffee maker according to config value
 * requries all initializations to be run before
*/
void runCoffeeMaker(coffeeConfig *config);

#endif