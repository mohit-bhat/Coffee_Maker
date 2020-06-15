//headers
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "nrf.h"
#include "nrf_delay.h"
#include "nordic_common.h"
#include "boards.h"
#include "app_error.h"
#include "bsp.h"
#include "app_pwm.h"
#include "nrf_gpio.h"
#include "Drive.h"



//initializations

void pwm_ready_callback(uint32_t pwm_id) // PWM callback function			From PWM Library in SDK
{
	ready_flag = true;
}

void pwmInit() //initializes pwm											heavy inspiration from PWM Library in SDK
{
	ret_code_t err_code;
	app_pwm_config_t pwm1_cfg = APP_PWM_DEFAULT_CONFIG_1CH(500000L, pwmPin);
	pwm1_cfg.pin_polarity[1] = APP_PWM_POLARITY_ACTIVE_HIGH;
	err_code = app_pwm_init(&PWM1, &pwm1_cfg, pwm_ready_callback);
	APP_ERROR_CHECK(err_code);
	app_pwm_enable(&PWM1);
}

void relayGPIOInit() //initializes gpio
{
	nrf_gpio_cfg_output(relayPin);
}

void saadc_callback(nrf_drv_saadc_evt_t const *p_event)
{
	ret_code_t err_code;
	if (p_event->type == NRF_DRV_SAADC_EVT_DONE) //Capture offset calibration complete event
	{

		if ((m_adc_evt_counter % SAADC_CALIBRATION_INTERVAL) == 0) //Evaluate if offset calibration should be performed. Configure the SAADC_CALIBRATION_INTERVAL constant to change the calibration frequency
		{
			nrf_drv_saadc_abort();	  // Abort all ongoing conversions. Calibration cannot be run if SAADC is busy
			m_saadc_calibrate = true; // Set flag to trigger calibration in main context when SAADC is stopped
		}
		if (m_saadc_calibrate == false)
		{
			err_code = nrf_drv_saadc_buffer_convert(p_event->data.done.p_buffer, SAADC_SAMPLES_IN_BUFFER); //Set buffer so the SAADC can write to it again.
			APP_ERROR_CHECK(err_code);
		}

		m_adc_evt_counter++;
	}
	else if (p_event->type == NRF_DRV_SAADC_EVT_CALIBRATEDONE)
	{

		err_code = nrf_drv_saadc_buffer_convert(m_buffer_pool[0], SAADC_SAMPLES_IN_BUFFER); //Set buffer so the SAADC can write to it again.
		APP_ERROR_CHECK(err_code);
		err_code = nrf_drv_saadc_buffer_convert(m_buffer_pool[1], SAADC_SAMPLES_IN_BUFFER); //Need to setup both buffers, as they were both removed with the call to nrf_drv_saadc_abort before calibration.
		APP_ERROR_CHECK(err_code);
	}
}

void saadc_init()
{
	ret_code_t err_code;
	nrf_drv_saadc_config_t saadc_config;
	nrf_saadc_channel_config_t channel_config;

	//Configure SAADC
	saadc_config.low_power_mode = true;						//Enable low power mode.
	saadc_config.resolution = NRF_SAADC_RESOLUTION_12BIT;	//Set SAADC resolution to 12-bit. This will make the SAADC output values from 0 (when input voltage is 0V) to 2^12=2048 (when input voltage is 3.6V for channel gain setting of 1/6).
	saadc_config.oversample = SAADC_OVERSAMPLE;				//Set oversample to 4x. This will make the SAADC output a single averaged value when the SAMPLE task is triggered 4 times.
	saadc_config.interrupt_priority = APP_IRQ_PRIORITY_LOW; //Set SAADC interrupt to low priority.

	//Initialize SAADC
	err_code = nrf_drv_saadc_init(&saadc_config, saadc_callback); //Initialize the SAADC with configuration and callback function. The application must then implement the saadc_callback function, which will be called when SAADC interrupt is triggered
	APP_ERROR_CHECK(err_code);

	//Configure SAADC channel
	channel_config.reference = NRF_SAADC_REFERENCE_INTERNAL; //Set internal reference of fixed 0.6 volts
	channel_config.gain = NRF_SAADC_GAIN1_6;				 //Set input gain to 1/6. The maximum SAADC input voltage is then 0.6V/(1/6)=3.6V. The single ended input range is then 0V-3.6V
	channel_config.acq_time = NRF_SAADC_ACQTIME_10US;		 //Set acquisition time. Set low acquisition time to enable maximum sampling frequency of 200kHz. Set high acquisition time to allow maximum source resistance up to 800 kohm, see the SAADC electrical specification in the PS.
	channel_config.mode = NRF_SAADC_MODE_SINGLE_ENDED;		 //Set SAADC as single ended. This means it will only have the positive pin as input, and the negative pin is shorted to ground (0V) internally.
	channel_config.pin_p = NRF_SAADC_INPUT_AIN1;			 //Select the input pin for the channel. AIN0 pin maps to physical pin P0.02.
	channel_config.pin_n = NRF_SAADC_INPUT_DISABLED;		 //Since the SAADC is single ended, the negative pin is disabled. The negative pin is shorted to ground internally.
	channel_config.resistor_p = NRF_SAADC_RESISTOR_DISABLED; //Disable pullup resistor on the input pin
	channel_config.resistor_n = NRF_SAADC_RESISTOR_DISABLED; //Disable pulldown resistor on the input pin

	//Initialize SAADC channel
	err_code = nrf_drv_saadc_channel_init(0, &channel_config); //Initialize SAADC channel 0 with the channel configuration
	APP_ERROR_CHECK(err_code);

	if (SAADC_BURST_MODE)
	{
		NRF_SAADC->CH[0].CONFIG |= 0x01000000; //Configure burst mode for channel 0. Burst is useful together with oversampling. When triggering the SAMPLE task in burst mode, the SAADC will sample "Oversample" number of times as fast as it can and then output a single averaged value to the RAM buffer. If burst mode is not enabled, the SAMPLE task needs to be triggered "Oversample" number of times to output a single averaged value to the RAM buffer.
	}

	err_code = nrf_drv_saadc_buffer_convert(m_buffer_pool[0], SAADC_SAMPLES_IN_BUFFER); //Set SAADC buffer 1. The SAADC will start to write to this buffer
	APP_ERROR_CHECK(err_code);

	err_code = nrf_drv_saadc_buffer_convert(m_buffer_pool[1], SAADC_SAMPLES_IN_BUFFER); //Set SAADC buffer 2. The SAADC will write to this buffer when buffer 1 is full. This will give the applicaiton time to process data in buffer 1.
	APP_ERROR_CHECK(err_code);
}

void tempSensorInit()
{
	NRF_POWER->DCDCEN = 1; //LOW POWER
	void saadc_init(void);
	return;
}

void LCDInit() //!
{
	//placeholder
}

void wirelessInit() //!
{
	//placeholder
}

void allInitializations()
{
	pwmInit();
	relayGPIOInit();
	tempSensorInit();
	LCDInit();
	wirelessInit();
}

//running functions
double dutyCycleChange(double temperatureGoal, double inputTemperature, double dutyCycle) //calculates new dutyCycle
{
	if (temperatureGoal == 100.0)
		return 100.0;
	if (temperatureGoal - inputTemperature > 0)
		dutyCycle = ((100.0 - dutyCycle) * (temperatureGoal - inputTemperature) / inputTemperature) + dutyCycle;
	if (temperatureGoal - inputTemperature <= 0)
	{
		dutyCycle = dutyCycle * temperatureGoal / inputTemperature;
	}

	if (dutyCycle > 100.0)
		dutyCycle = 100.0;
	if (dutyCycle < 0.0)
		dutyCycle = 0.0;

	return dutyCycle;
}

double tempConversion(uint64_t inputValue)
{
	//values from diagram on temp-sensor datasheet and adc setting
	double refVoltage = 3.6;
	double resolution = 2 ^ 12;
	double rise = 1.75;
	double run = 100;
	double start = 1.25;

	double iv = (double)inputValue;
	double voltage = (refVoltage * iv) / (resolution);
	return run * ((voltage - start) / (rise - start));
}

double pollTemp()
{
	for (int i = 0; i < 4; i++)
		nrf_drv_saadc_sample();
	return tempConversion(*nrf_saadc_buffer_pointer_get());
}

void updateLCD(double temperature) //!
{
	//placeholder
	temperature = temperature;
}

void runPWM(double temperatureGoal, int runTimeSec) //runs pwm control of coffee machine and times run cycle
{
	//covering for invalid input
	if (temperatureGoal > 100.0)
		temperatureGoal = 100.0;
	if (temperatureGoal < lowerTempLimit || runTimeSec < lowerRunTimeLimit || runTimeSec > upperRunTimeLimit)
		return;

	//Initializing variables
	double dutyCycle = 100.0;
	double inputTemperature = 0.0;

	//running coffee maker
	for (int i = 0; i < runTimeSec; i++)
	{
		//get temp
		inputTemperature = pollTemp();
		updateLCD(inputTemperature);
		if (inputTemperature > 110) //overtempt safety
			return;
		dutyCycle = dutyCycleChange(temperatureGoal, inputTemperature, dutyCycle); //adjust duty cycle in response to temp
		while (app_pwm_channel_duty_set(&PWM1, 0, dutyCycle) == NRF_ERROR_BUSY)
			;								  //apply duty cycle change, waiting until able
		for (int i = 0; i < LEDS_NUMBER; i++) //LED for fun
		{
			bsp_board_led_invert(i);
		}
		nrf_delay_ms(1000); //delay
	}
}

void setRelay(bool on) //changes relay state
{
	if (on)
		nrf_gpio_pin_set(relayPin);
	else
		nrf_gpio_pin_clear(relayPin);
}

coffeeConfig *pollWireless() //!
{
	//placeholder

	coffeeConfig *config = (coffeeConfig *)malloc(sizeof(coffeeConfig));
	config->runCycle = runCycle;
	config->temperatureGoal = temperatureGoal;
	config->runTimeSec = runTimeSec;
	config->delayTimeSec = delayTimeSec;
	return config;
}

void resetWirelessRegisters() //!
{
	//placeholder
}

void runCoffeeMaker(coffeeConfig *config)
{
	if (config->runCycle)
	{
		resetWirelessRegisters();
		nrf_delay_ms(1000 * config->delayTimeSec);
		setRelay(true);
		runPWM(config->temperatureGoal, config->runTimeSec);
		setRelay(false);
		config->runCycle = false;
	}
	nrf_delay_ms(500);
	config = pollWireless();
}