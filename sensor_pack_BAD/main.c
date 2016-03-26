#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"
#include "sensorlib/hw_sht21.h"
#include "sensorlib/i2cm_drv.h"
#include "sensorlib/sht21.h"
#include "drivers/pinout.h"
//#include "drivers/buttons.h"


//the sensors I2C addresses
#define SHT21_I2C_ADDRESS   0x40  //humidity
#define TMP006_I2C_ADDRESS      0x41  //temperature
#define ISL29023_I2C_ADDRESS    0x44   //light
#define BMP180_I2C_ADDRESS   0x77  //pressure


//the system clock speed
uint32_t g_ui32SysClock;

//instance structure for I2C master driver
tI2CMInstance g_sI2CInst;

//instance structure for the sensors
tSHT21 g_sSHT21Inst;

volatile uint_fast8_t g_vui8DataFlag;

volatile uint_fast8_t g_vui8ErrorFlag;

//*****************************************************************************
//
// SHT21 Sensor callback function.  Called at the end of SHT21 sensor driver
// transactions. This is called from I2C interrupt context. Therefore, we just
// set a flag and let main do the bulk of the computations and display.
//
//*****************************************************************************
void
SHT21AppCallback(void *pvCallbackData, uint_fast8_t ui8Status)
{
    //
    // Turn off the LED to show that transaction is complete.
    //
    LEDWrite(CLP_D1 | CLP_D2, 0);

    //
    // If the transaction succeeded set the data flag to indicate to
    // application that this transaction is complete and data may be ready.
    //
    if(ui8Status == I2CM_STATUS_SUCCESS)
    {
        g_vui8DataFlag = 1;
    }

    //
    // Store the most recent status in case it was an error condition.
    //
    g_vui8ErrorFlag = ui8Status;
}



void ConfigureUART(void){

	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

	GPIOPinConfigure(GPIO_PA0_U0RX);
	GPIOPinConfigure(GPIO_PA1_U0TX);
	GPIOPinTypeUART(GPIO_PORTA_BASE,GPIO_PIN_0|GPIO_PIN_1);

	UARTClockSourceSet(UART0_BASE,UART_CLOCK_SYSTEM);

	UARTStdioConfig(0,9600,g_ui32SysClock);

}


/*
 * main.c
 */
int main(void) {
	float fTemperature, fHumidity;
	    int32_t i32IntegerPart;
	    int32_t i32FractionPart;


	g_ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
	                                             SYSCTL_OSC_MAIN |
	                                             SYSCTL_USE_PLL |
	                                             SYSCTL_CFG_VCO_480), 120000000);
	//confiugre the GPIO pins
	PinoutSet(false,false);

	ConfigureUART();

	//configure I2C pins
	SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C8);

	GPIOPinConfigure(GPIO_PA2_I2C8SCL);
	GPIOPinConfigure(GPIO_PA3_I2C8SDA);

	GPIOPinTypeI2C(GPIO_PORTA_BASE,GPIO_PIN_2);
	GPIOPinTypeI2C(GPIO_PORTA_BASE,GPIO_PIN_3);

	//enable interrupts
	IntMasterEnable();

	//initialize I2C
	I2CMInit(&g_sI2CInst,I2C8_BASE,INT_I2C8,0xff,0xff,g_ui32SysClock);
	IntPrioritySet(INT_I2C7,0xE0);

	//initialize the sensors
	SHT21Init(&g_sSHT21Inst, &g_sI2CInst, SHT21_I2C_ADDRESS,
			SHT21AppCallback,&g_sSHT21Inst);

	//delay for 20 ms
	SysCtlDelay(g_ui32SysClock / (50 * 3));

	while(1){
		  //
		        // Turn on D2 to show we are starting a transaction with the sensor.
		        // This is turned off in the application callback.
		        //
		        LEDWrite(CLP_D1 | CLP_D2 , CLP_D2);

		        //
		        // Write the command to start a humidity measurement.
		        //
		        SHT21Write(&g_sSHT21Inst, SHT21_CMD_MEAS_RH, g_sSHT21Inst.pui8Data, 0,
		                SHT21AppCallback, &g_sSHT21Inst);

		        //
		        // Wait for the I2C transactions to complete before moving forward.
		        //
		        SHT21AppI2CWait(__FILE__, __LINE__);

		        //
		        // Wait 33 milliseconds before attempting to get the result. Datasheet
		        // claims this can take as long as 29 milliseconds.
		        //
		        SysCtlDelay(g_ui32SysClock / (30 * 3));

		        //
		        // Turn on D2 to show we are starting a transaction with the sensor.
		        // This is turned off in the application callback.
		        //
		        LEDWrite(CLP_D1 | CLP_D2 , CLP_D2);

		        //
		        // Get the raw data from the sensor over the I2C bus.
		        //
		        SHT21DataRead(&g_sSHT21Inst, SHT21AppCallback, &g_sSHT21Inst);

		        //
		        // Wait for the I2C transactions to complete before moving forward.
		        //
		        SHT21AppI2CWait(__FILE__, __LINE__);

		        //
		        // Get a copy of the most recent raw data in floating point format.
		        //
		        SHT21DataHumidityGetFloat(&g_sSHT21Inst, &fHumidity);

		        //
		        // Turn on D2 to show we are starting a transaction with the sensor.
		        // This is turned off in the application callback.
		        //
		        LEDWrite(CLP_D1 | CLP_D2 , CLP_D2);

		        //
		        // Write the command to start a temperature measurement.
		        //
		        SHT21Write(&g_sSHT21Inst, SHT21_CMD_MEAS_T, g_sSHT21Inst.pui8Data, 0,
		                SHT21AppCallback, &g_sSHT21Inst);

		        //
		        // Wait for the I2C transactions to complete before moving forward.
		        //
		        SHT21AppI2CWait(__FILE__, __LINE__);

		        //
		        // Wait 100 milliseconds before attempting to get the result. Datasheet
		        // claims this can take as long as 85 milliseconds.
		        //
		        SysCtlDelay(g_ui32SysClock / (10 * 3));

		        //
		        // Turn on D2 to show we are starting a transaction with the sensor.
		        // This is turned off in the application callback.
		        //
		        LEDWrite(CLP_D1 | CLP_D2 , CLP_D2);

		        //
		        // Read the conversion data from the sensor over I2C.
		        //
		        SHT21DataRead(&g_sSHT21Inst, SHT21AppCallback, &g_sSHT21Inst);

		        //
		        // Wait for the I2C transactions to complete before moving forward.
		        //
		        SHT21AppI2CWait(__FILE__, __LINE__);

		        //
		        // Get the most recent temperature result as a float in celcius.
		        //
		        SHT21DataTemperatureGetFloat(&g_sSHT21Inst, &fTemperature);

		        //
		        // Convert the floats to an integer part and fraction part for easy
		        // print. Humidity is returned as 0.0 to 1.0 so multiply by 100 to get
		        // percent humidity.
		        //
		        fHumidity *= 100.0f;
		        i32IntegerPart = (int32_t) fHumidity;
		        i32FractionPart = (int32_t) (fHumidity * 1000.0f);
		        i32FractionPart = i32FractionPart - (i32IntegerPart * 1000);
		        if(i32FractionPart < 0)
		        {
		            i32FractionPart *= -1;
		        }

		        //
		        // Print the humidity value using the integers we just created.
		        //
		        UARTprintf("Humidity %3d.%03d\t", i32IntegerPart, i32FractionPart);

		        //
		        // Perform the conversion from float to a printable set of integers.
		        //
		        i32IntegerPart = (int32_t) fTemperature;
		        i32FractionPart = (int32_t) (fTemperature * 1000.0f);
		        i32FractionPart = i32FractionPart - (i32IntegerPart * 1000);
		        if(i32FractionPart < 0)
		        {
		            i32FractionPart *= -1;
		        }

		        //
		        // Print the temperature as integer and fraction parts.
		        //
		        UARTprintf("Temperature %3d.%03d\n", i32IntegerPart, i32FractionPart);

		        //
		        // Delay for one second. This is to keep sensor duty cycle
		        // to about 10% as suggested in the datasheet, section 2.4.
		        // This minimizes self heating effects and keeps reading more accurate.
		        //
		        SysCtlDelay(g_ui32SysClock / 3);
	}


	
	return 0;
}
