#ifndef __CONFIG_H
#define __CONFIG_H

#if defined TARGET_MAPLE_MINI
 		#define HAS_LED1_PIN
 		#define LED1_CLOCK_EN		(RCC->APB2ENR |= RCC_APB2ENR_IOPBEN)	//[1]
		#define LED1_CLOCK_DIS	(RCC->APB2ENR &= ~RCC_APB2ENR_IOPBEN)	//[1]
		#define LED1_BIT_0			(GPIOB->CRL &= ~GPIO_CRL_CNF1_0)			//[2]
		#define LED1_BIT_1			(GPIOB->CRL &= ~GPIO_CRL_CNF1_1)			//[3]
		#define LED1_MODE				(GPIOB->CRL |= GPIO_CRL_MODE1) 				//[4] Output, max 50 MHz
		#define LED1_ON					(GPIOB->BSRR = GPIO_BSRR_BS1)
		#define LED1_OFF				(GPIOB->BRR	= GPIO_BRR_BR1)
		
		#define HAS_DISC_PIN
		#define DISC_CLOCK_EN		(RCC->APB2ENR |= RCC_APB2ENR_IOPBEN)
		#define LED1_CLOCK_DIS	(RCC->APB2ENR &= ~RCC_APB2ENR_IOPBEN)	//[1]
		#define DISC_BIT_0			(GPIOB->CRH |= GPIO_CRH_CNF9_0)  	//GPIO_CRH_CNF9_0 //Bit 0
		#define DISC_BIT_1			(GPIOB->CRH &= ~GPIO_CRH_CNF9_1)	//GPIO_CRH_CNF9_1 //Bit 1
		#define DISC_MODE				(GPIOB->CRH |= GPIO_CRH_MODE9)  	//Pin Mode
	
#elif defined TARGET_GENERIC_F103_PC13
 		#define HAS_LED1_PIN
		#define LED1_CLOCK_EN		(RCC->APB2ENR |= RCC_APB2ENR_IOPCEN)//[1]
		#define LED1_CLOCK_DIS	(RCC->APB2ENR &= ~RCC_APB2ENR_IOPCEN)	//[1]
		#define LED1_BIT_0			(GPIOC->CRH |= GPIO_CRH_CNF13_0)		//[2]
		#define LED1_BIT_1			(GPIOC->CRH &= ~GPIO_CRH_CNF13_1)		//[3]
		#define LED1_MODE				(GPIOC->CRH |= GPIO_CRH_MODE13) 		//[4] Output, max 50 MHz
		#define LED1_OFF				(GPIOC->BSRR = GPIO_BSRR_BS13)
		#define LED1_ON					(GPIOC->BRR	= GPIO_BRR_BR13)
			
#else
    #error "No config for this target"
#endif

#endif