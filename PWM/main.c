#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/fpu.h"   //floating point
#include "driverlib/gpio.h"
#include "driverlib/debug.h"
#include "driverlib/pwm.h"
#include "driverlib/pin_map.h"
#include "inc/hw_gpio.h"


#define PWM_FREQUENCY 100
#define APP_PI        3.1415926535897932384626433832795f
#define STEPS		  256


//DON'T KNOW WHAT THIS CODE DOES YET....
//READ WORKBOOK


int main(void)
{
	volatile uint32_t Load;
	volatile uint32_t BlueLevel;
	volatile uint32_t PWMClock;
	volatile uint32_t SysClkFreq;
	volatile uint32_t Index;
	float fAngle;

	SysClkFreq = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480), 120000000);

	//enabling the gpios and PWM
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);

	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_2|GPIO_PIN_3);
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2|GPIO_PIN_3, 0x00);		// assure the red and blue LEDs are off

	PWMClockSet(PWM0_BASE,PWM_SYSCLK_DIV_64);						// 120MHz/64 = 1.875MHz

	GPIOPinConfigure(GPIO_PG0_M0PWM4);
	GPIOPinTypePWM(GPIO_PORTG_BASE, GPIO_PIN_0);

	PWMClock = SysClkFreq / 64;
	Load = (PWMClock / PWM_FREQUENCY) - 1;					//1875000/100 = 18750

	PWMGenConfigure(PWM0_BASE, PWM_GEN_2, PWM_GEN_MODE_DOWN);
	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, Load);

	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, Load/2);		// a starter for the load value
	PWMOutputState(PWM0_BASE, PWM_OUT_4_BIT, true);
	PWMGenEnable(PWM0_BASE, PWM_GEN_2);

	Index = 0;

	while(1)
	{
		fAngle = Index * (2.0f * APP_PI/STEPS);
		BlueLevel = (uint32_t) (9370.0f * (1 + sinf(fAngle)));	// 18750/2 > 9370 to keep load value < max
		PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, BlueLevel + 1);	// +1 keeps the load value > 0
		Index++;
		if (Index == (STEPS - 1))
		{
			Index = 0;
		}
		SysCtlDelay(SysClkFreq/(STEPS));						// total delay about 3 seconds
	}
}


