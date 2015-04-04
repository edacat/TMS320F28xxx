/* ========================================================================== */
/* ========================================================================== */
//  Group:          C2000
//  Target Device:  TMS320F2802x
/* ========================================================================== */
/* ========================================================================== */

#include "stddef.h"
#include "string.h"

#include "./include/DSP28x_Project.h"
#include "./include/F2802x_Examples.h"
#include "./include/F2802x_GlobalPrototypes.h"

#include "./include/cpu.h"
#include "./include/clk.h"
#include "./include/flash.h"
#include "./include/gpio.h"
#include "./include/pll.h"
#include "./include/pwm.h"
#include "./include/wdog.h"
#include "./include/pie.h"
//#include "./include/pie_init.h"
#include "./include/timer.h"
#include "./include/adc.h"
#include "./include/sci.h"

//#include "./F28027_SCI.h"
//#include "TM1638.h"

#include "init.h"

#include "n5110.h"
#include "5110.h"


/* ========================================================================== */
// Configure which ePWM timer interrupts are enabled at the PIE level:
// 1 = enabled,  0 = disabled
#define PWM1_INT_ENABLE   0
#define PWM2_INT_ENABLE   0
#define PWM3_INT_ENABLE   0

// Configure the period for each timer
#define PWM1_TIMER_TBPRD  0xFFFF
//#define PWM1_TIMER_TBPRD  0x1FFF

#define PWM2_TIMER_TBPRD  0x1FFF

//#define PWM3_TIMER_TBPRD  0x1FFF
#define PWM3_TIMER_TBPRD  0xFFFF

/* ========================================================================== */
#if (CPU_FRQ_40MHZ)
#define CPU_FREQ          (40000000) // Default=40 MHz Change to 50E6 for 50 MHz devices
#endif

#if (CPU_FRQ_50MHZ)
#define CPU_FREQ          (50000000) // Default=40 MHz Change to 50E6 for 50 MHz devices
#endif

#if (CPU_FRQ_60MHZ)
#define CPU_FREQ          (60000000) // Default=40 MHz Change to 50E6 for 50 MHz devices
#endif

#define LSPCLK_FREQ       (CPU_FREQ/4)

//#define SCI_FREQ          100E3
#define SCI_FREQ          115200
//#define SCI_PRD           (LSPCLK_FREQ/(SCI_FREQ*8))-1
#define SCI_BRR           (LSPCLK_FREQ/(SCI_FREQ*8))-1
/* ========================================================================== */

//#define  USE_UART_IRQ_1   (0)
//#define  USE_UART_IRQ_2   (1)

typedef enum {
	TERMINAL_PRINT=0,
	TERMINAL_READ,
	TERMINAL_SHOW,
	TERMINAL_ENTER,
	TERMINAL_WAIT,
} t_terminal_state;

typedef enum {
	E_OK=0,
	E_FAIL,
	E_BADPTR
} t_error;

typedef enum {
	SCI_TX_READY=0,
	SCI_TX_SENDING,
	SCI_TX_FINISH,
	SCI_NONE
} t_sci_stat;

typedef struct {
	struct {
		t_sci_stat tx :4;
	} sci;
} t_status;

t_status sys_stat;


// Prototype statements for functions found within this file.
void Init_All ( void );

interrupt void epwm1_timer_isr (void);
interrupt void epwm2_timer_isr (void);
interrupt void epwm3_timer_isr (void);
interrupt void timer0_isr      (void);
void           init_Cfg_EPwmTimers (void);

t_error wrapper_Init_Sys (void);
t_error wrapper_Init_PWM_IRQs (void);
t_error wrapper_Init_GPIO (void);
//t_error wrapper_Init_UART_pooling (void);
//t_error wrapper_Init_UART_IRQ (void);
void    wrapper_Main ( void );
void    wrapper_Error_Handle( t_error err );
void    error(void);

// Prototype statements for functions found within this file.
void            scia_fifo_init(void);
interrupt void  sciaTxFifoIsr (void);
interrupt void  sciaRxFifoIsr (void);

t_error  wrapper_Init_UART_IRQ (void);
t_error  wrapper_Init_TM1638 (void);


/* ========================================================================== */
/* Global variables */
/* ========================================================================== */
uint16_t     RamfuncsLoadStart;
uint16_t     RamfuncsLoadSize;
uint16_t     RamfuncsRunStart;

uint32_t     EPwm1TimerIntCount;
uint32_t     EPwm2TimerIntCount;
uint32_t     EPwm3TimerIntCount;
uint32_t     Timer0IntCount;

CPU_Handle   myCpu;
PLL_Handle   myPll;
CLK_Handle   myClk;
FLASH_Handle myFlash;
ADC_Handle   myAdc;
SCI_Handle   mySci;
GPIO_Handle  myGpio;
PIE_Handle   myPie;
PWM_Handle   myPwm1, myPwm2, myPwm3;
TIMER_Handle myTimer;
WDOG_Handle  myWDog;


t_error      err=E_OK;
int          i=0, i_tx=0, i_rx=0;
int          i_pwm3=0;

char         *p_sci_msg;
uint16_t     LoopCount;
uint16_t     ErrorCount;
uint16_t     ReceivedChar;
#define      RX_LEN         (8)
uint16_t     sdataA[RX_LEN];     // Send data for SCI-A
uint16_t     rdataA[RX_LEN];     // Received data for SCI-A
uint16_t     rdata_pointA;  // Used for checking the received data

uint16_t     RxTx;



unsigned char ArrayDisplay[LCD_X_RES];


/* ========================================================================== */
void InitGpio_Conf_HW (void) {
	EALLOW;
	GpioCtrlRegs.GPAMUX1.all |= gpio_mux;
	// 在这里插入初始化函数的代码
	GpioCtrlRegs.GPADIR.all |= gpio_dir;
	//	GpioDataRegs.GPASET.all |= 0xff;
	EDIS;
}
/* ========================================================================== */


void* memset(void *mem, register int ch, register size_t length)
{
     register char *m = (char *)mem;

     while (length--) *m++ = ch;
     return mem;
}


/* ==========================================================================
 * MAIN
 * ========================================================================== */
void main (void)
{
	Init_All ();

#if (1==__USE_LCD_5110__)
	Lcd_clear();
	Lcd_init();
/*
	Lcd_prints(0, 0, FONT_1X, "~" );
	Lcd_prints(1, 0, FONT_1X, "Hello world!" );
	Lcd_prints(1, 1, FONT_1X, "It's working." );
	Lcd_prints(1, 2, FONT_1X, "uschema.com" );
	Lcd_prints(1, 3, FONT_1X, "羊囗桉豚�" );
	Lcd_prints(1, 4, FONT_1X, "橡桴钿�" );
	Lcd_prints(1, 5, FONT_1X, "TMS320F28027." );
*/
    /*byte strings[6][14] = {
    	"Hello world! ",
    	"It's working.",
    	"uschema.com  ",
    	"羊囗桉豚�    ",
    	"橡桴钿�    ",
    	"TMS320F28027."
    };
	byte nn;

	for (nn=0; nn<6; nn++) {
		Lcd_prints(1, nn, FONT_1X, &strings[nn][0] );
	}*/

	//Lcd_rect_empty ( 0, 8, 8, LCD_X_RES-1, PIXEL_XOR);
	//Lcd_rect ( 0, 8, 8, LCD_X_RES-1, PIXEL_XOR);
	//Lcd_line( 20, 8, 30, 16, PIXEL_ON );
	//Lcd_circle ( 3, 4, 1, PIXEL_ON );

	memset (ArrayDisplay, 16, LCD_X_RES);

	ShowDisplayFromBufferByVertical( ArrayDisplay, 1/*K=0..1*/ );

	Lcd_update();
#endif //(1==__USE_LCD_5110__)

	// Main code
    for(;;) {
    	//wrapper_Main();
    	//tm1638_prints("123456789");
    	//tm1638_printx("1", 1);

#if (1==__USE_LCD_5110__)
    	Lcd_update();
#endif //(1==__USE__LCD_5110__)

    }
}
/* ========================================================================== */



/* ==========================================================================
 * NAME - Init_All
 * IN   - void
 * OUT  - void
 * RET  - void
   ========================================================================== */
void Init_All ( void ) {
	if (E_OK==err) {
        err = wrapper_Init_Sys ();     	 // Init system and handles
    } else {
    	wrapper_Error_Handle (err);
    }

    if (E_OK==err) {
    	err = wrapper_Init_GPIO ();   // Init GPIO system
    } else {
    	wrapper_Error_Handle (err);
    }

    if (E_OK==err) {
        err = wrapper_Init_PWM_IRQs ();  // Init IRQs
    } else {
    	wrapper_Error_Handle (err);
    }

    if (E_OK==err) {
    	err = wrapper_Init_UART_IRQ ();   // Init UART IRQ
    	//err = wrapper_Init_UART_pooling ();   // Init UART without IRQ
    } else {
    	wrapper_Error_Handle (err);
    }

    if (E_OK==err) {
    	err = wrapper_Init_TM1638 ();   // Init TM1638
    } else {
    	wrapper_Error_Handle (err);
    }
}


/* ==========================================================================
 * NAME - Wrapper_Main
 * IN   - void
 * OUT  - void
 * RET  - void
   ========================================================================== */
void wrapper_Main ( void ) {
#if 0
    /* asm (" NOP");
    for ( i=1; i<=10; i++ ) {
        GPIO_setPortData (myGpio, GPIO_Port_A, i );
    }  */

	// Describe of functionality
	// - UART Terminal for control
	// -- Timer #0 as base CLI (Command Line Interface)
	// -- CLI (Command Line Interface)
	// -- PWM #0, #1, #2 : On-Off
	// -- UART_PWM_generator
	// -- LCD and LEDs
	// -- GPIO


	// -- GPIO
/*
	uint16_t  ui16_gpio_in;
	ui16_gpio_in = GPIO_getPortData (myGpio, GPIO_Port_A);
	if ( ui16_gpio_in & (1<<12) ) {
		//GPIO_setLow(myGpio, GPIO_Number_0 );
		GPIO_setPortData (myGpio, GPIO_Port_A, 0xF );
		//SCI_write(mySci, '3');
	} else {
		//GPIO_setHigh(myGpio, GPIO_Number_0 );
		GPIO_setPortData (myGpio, GPIO_Port_A, 0xE );
	}
*/
	/*if (0)
    {
    	t_terminal_state  terminal_state;
		static uint16_t   try = 0xffff;

    	switch ( terminal_state ){
    		case TERMINAL_PRINT:
    	    	p_sci_msg = "\rEnter a character: \0";
    	        scia_msg(p_sci_msg);
    			terminal_state = TERMINAL_READ;
    		break;

    		case TERMINAL_READ:
    	        if (SCI_getRxFifoStatus(mySci) >= SCI_FifoStatus_1_Word) {
    	            ReceivedChar = SCI_getData(mySci); // Get character
        			terminal_state = TERMINAL_SHOW;
    	        } else {
        			if (--try)  terminal_state = TERMINAL_READ;
        			else        terminal_state = TERMINAL_SHOW;
    	        }
    		break;

    		case TERMINAL_SHOW:
    			try=0xffff;
	            // Echo character back
	            p_sci_msg = "  You sent: \0";
	    		terminal_state = TERMINAL_ENTER;
    		break;

    		case TERMINAL_ENTER:
	            scia_msg(p_sci_msg);
	            scia_xmit(ReceivedChar);
	            //LoopCount++;
	    		terminal_state = TERMINAL_WAIT;
    		break;

    		case TERMINAL_WAIT:
    	        // Wait for inc character
    	        //while (SCI_getRxFifoStatus(mySci) < SCI_FifoStatus_4_Words) {}
	    		terminal_state = TERMINAL_PRINT;
	        break;

    		default:
	    		terminal_state = TERMINAL_PRINT;
    		break;
    	}
    }*/
#endif //0
}
/* ========================================================================== */



/* ==========================================================================
 * NAME - Wrapper_Error_Handle
 * IN   - t_error err
 * OUT  - void
 * RET  - void
   ========================================================================== */
void wrapper_Error_Handle (t_error err) {
	switch (err) {
		case E_OK:		break;
		case E_FAIL:	case E_BADPTR:		default:
			error();
			//break;
	}
}
/* ========================================================================== */



/* ==========================================================================
 * NAME - Wrapper_Init_Sys
 * IN   - void
 * OUT  - void
 * RET  - t_error err
   ========================================================================== */
t_error wrapper_Init_Sys (void) {

	// PERIPHERAL CLOCK ENABLES
	//---------------------------------------------------
	// If you are not using a peripheral you may want to switch
	// the clock off to save power, i.e. set to =0
	// Note: not all peripherals are available on all 280x derivates.
	// Refer to the datasheet for your particular device.
	//------------------------------------------------
	SysCtrlRegs.PCLKCR0.bit.ADCENCLK   = 0;  // ADC
	SysCtrlRegs.PCLKCR3.bit.COMP1ENCLK = 0;	 // COMP1
	SysCtrlRegs.PCLKCR3.bit.COMP2ENCLK = 0;	 // COMP2
	SysCtrlRegs.PCLKCR0.bit.I2CAENCLK  = 1;  // I2C
	SysCtrlRegs.PCLKCR0.bit.SPIAENCLK  = 0;	 // SPI-A
	SysCtrlRegs.PCLKCR0.bit.SCIAENCLK  = 0;  // SCI-A
	SysCtrlRegs.PCLKCR1.bit.ECAP1ENCLK = 0;	 // eCAP1
	SysCtrlRegs.PCLKCR1.bit.EPWM1ENCLK = 1;  // ePWM1
	SysCtrlRegs.PCLKCR1.bit.EPWM2ENCLK = 1;  // ePWM2
	SysCtrlRegs.PCLKCR1.bit.EPWM3ENCLK = 1;  // ePWM3
	SysCtrlRegs.PCLKCR1.bit.EPWM4ENCLK = 0;  // ePWM4
	SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC  = 1;  // Enable TBCLK
	//------------------------------------------------


    // Initialize all the handles needed for this application
#if (1==USE_F28027_CLK)
    myClk   = CLK_init  ((void *)CLK_BASE_ADDR, sizeof(CLK_Obj));
#endif //(1==USE_F28027_CLK)

#if (1==USE_F28027_CPU)
    myCpu   = CPU_init  ((void *)NULL, sizeof(CPU_Obj));
#endif //(1==USE_F28027_CPU)

#if (1==USE_F28027_FLASH)
    myFlash = FLASH_init((void *)FLASH_BASE_ADDR, sizeof(FLASH_Obj));
#endif //(1==USE_F28027_FLASH)

#if (1==USE_F28027_GPIO)
    myGpio  = GPIO_init ((void *)GPIO_BASE_ADDR, sizeof(GPIO_Obj));
#endif //(1==USE_F28027_GPIO)

#if (1==USE_F28027_PIE)
    myPie   = PIE_init  ((void *)PIE_BASE_ADDR, sizeof(PIE_Obj));
#endif //(1==USE_F28027_PIE)

#if (1==USE_F28027_PLL)
    myPll   = PLL_init  ((void *)PLL_BASE_ADDR, sizeof(PLL_Obj));
#endif //(1==USE_F28027_PLL)

#if (1==USE_F28027_PWM)
	#if (1==PWM1_INT_ENABLE)
    myPwm1  = PWM_init  ((void *)PWM_ePWM1_BASE_ADDR, sizeof(PWM_Obj));
	#endif //(1==PWM1_INT_ENABLE)

	#if (1==PWM2_INT_ENABLE)
    myPwm2  = PWM_init  ((void *)PWM_ePWM2_BASE_ADDR, sizeof(PWM_Obj));
	#endif //(1==PWM2_INT_ENABLE)

	#if (1==PWM2_INT_ENABLE)
    myPwm3  = PWM_init  ((void *)PWM_ePWM3_BASE_ADDR, sizeof(PWM_Obj));
	#endif //(1==PWM2_INT_ENABLE)
#endif //(1==USE_F28027_PWM)

#if (1==USE_F28027_WDOG)
    myWDog  = WDOG_init ((void *)WDOG_BASE_ADDR, sizeof(WDOG_Obj));
#endif //(1==USE_F28027_WDOG)

#if (1==USE_F28027_TIMER)
    myTimer = TIMER_init((void *)TIMER0_BASE_ADDR, sizeof(TIMER_Obj));
#endif //(1==USE_F28027_TIMER)

#if (1==USE_F28027_ADC)
    myAdc   = ADC_init  ((void *)ADC_BASE_ADDR, sizeof(ADC_Obj));
#endif //(1==USE_F28027_ADC)

#if (1==USE_F28027_SCI)
    mySci   = SCI_init  ((void *)SCIA_BASE_ADDR, sizeof(SCI_Obj));
#endif //(1==USE_F28027_SCI)

    // Perform basic system initialization
    WDOG_disable (myWDog);
    CLK_enableAdcClock (myClk);
    (*Device_cal)();
    CLK_disableAdcClock (myClk);

    //Select the internal oscillator 1 as the clock source
    CLK_setOscSrc (myClk, CLK_OscSrc_Internal);

    // Setup the PLL for x10 /2 which will yield 50Mhz = 10Mhz * 10 / 2
    PLL_setup (myPll, PLL_Multiplier_10, PLL_DivideSelect_ClkIn_by_2);

    // Disable the PIE and all interrupts
    PIE_disable (myPie);
    PIE_disableAllInts (myPie);
    CPU_disableGlobalInts (myCpu);
    CPU_clearIntFlags (myCpu);

    // If running from flash copy RAM only functions to RAM
#ifdef _FLASH
    memcpy (&RamfuncsRunStart, &RamfuncsLoadStart, (size_t)&RamfuncsLoadSize);
#endif

    return (t_error) E_OK;
}
/* ========================================================================== */



/* ==========================================================================
 * NAME - Wrapper_Init_Cfg_IRQs
 * IN   - void
 * OUT  - void
 * RET  - t_error err
   ========================================================================== */
t_error wrapper_Init_PWM_IRQs (void) {
#if (1==USE_F28027_PWM)
    // Setup a debug vector table and enable the PIE
    PIE_setDebugIntVectorTable (myPie);
    PIE_enable (myPie);

    // Register interrupt handlers in the PIE vector table

#if (1==PWM1_INT_ENABLE)
    PIE_registerPieIntHandler (myPie, PIE_GroupNumber_3, PIE_SubGroupNumber_1, (intVec_t)&epwm1_timer_isr);
#endif //(1==PWM1_INT_ENABLE)

#if (1==PWM2_INT_ENABLE)
    PIE_registerPieIntHandler (myPie, PIE_GroupNumber_3, PIE_SubGroupNumber_2, (intVec_t)&epwm2_timer_isr);
#endif //(1==PWM2_INT_ENABLE)

#if (1==PWM3_INT_ENABLE)
    PIE_registerPieIntHandler (myPie, PIE_GroupNumber_3, PIE_SubGroupNumber_3, (intVec_t)&epwm3_timer_isr);
#endif //(1==PWM3_INT_ENABLE)

    // TODO ??? GROUP ???
    //PIE_registerPieIntHandler (myTimer, PIE_GroupNumber_3, PIE_SubGroupNumber_3, (intVec_t)&timer0_isr);

    // For this example, only initialize the ePWM Timers
    init_Cfg_EPwmTimers ();

    // Enable CPU INT3 which is connected to EPWM1-6 INT
    Timer0IntCount     = 0;
    CPU_enableInt (myCpu, CPU_IntNumber_3);

    // Enable EPWM INTn in the PIE: Group 3 interrupt 1-6
#if (1==PWM1_INT_ENABLE)
    EPwm1TimerIntCount = 0;    // Initalize counters:
    PIE_enablePwmInt (myPie, PWM_Number_1);
#endif //(1==PWM1_INT_ENABLE)

#if (1==PWM2_INT_ENABLE)
    EPwm2TimerIntCount = 0;    // Initalize counters:
    PIE_enablePwmInt (myPie, PWM_Number_2);
#endif //(1==PWM2_INT_ENABLE)

#if (1==PWM3_INT_ENABLE)
    EPwm3TimerIntCount = 0;    // Initalize counters:
    PIE_enablePwmInt (myPie, PWM_Number_3);
#endif //(1==PWM3_INT_ENABLE)

    //PIE_enableTimer0Int (myTimer);

    // Enable global Interrupts and higher priority real-time debug events
    CPU_enableGlobalInts (myCpu);
    CPU_enableDebugInt (myCpu);

#endif //(1==USE_F28027_PWM)
    return E_OK;
}
/* ========================================================================== */



/* ==========================================================================
 * NAME - Wrapper_Init_Cfg_Sys
 * IN   - void
 * OUT  - void
 * RET  - t_error err
   ========================================================================== */
t_error wrapper_Init_GPIO (void) {
#if (1==USE_F28027_GPIO)
    // Initalize GPIO
    GPIO_setDirection (myGpio, GPIO_Number_0, GPIO_Direction_Output);
    GPIO_setDirection (myGpio, GPIO_Number_1, GPIO_Direction_Output);
    GPIO_setDirection (myGpio, GPIO_Number_2, GPIO_Direction_Output);
    GPIO_setDirection (myGpio, GPIO_Number_3, GPIO_Direction_Output);
    GPIO_setDirection (myGpio, GPIO_Number_4, GPIO_Direction_Output);
    GPIO_setDirection (myGpio, GPIO_Number_5, GPIO_Direction_Output);
    GPIO_setPortData  (myGpio, GPIO_Port_A, 0x003F);

    GPIO_setMode(myGpio, GPIO_Number_5, GPIO_5_Mode_EPWM3B);

	//InitGpio_Conf_HW();
#endif //(1==USE_F28027_GPIO)
    return E_OK;
}
/* ========================================================================== */



/* ==========================================================================

    return E_OK;
 * NAME - init_Cfg_EPwmTimers
 * IN   - void
 * OUT  - void
 * RET  - void
   ========================================================================== */
void init_Cfg_EPwmTimers (void) {
#if (1==USE_F28027_PWM)
    // Stop all the TB clocks
    CLK_disableTbClockSync(myClk);
    
#if (1==PWM1_INT_ENABLE)
    CLK_enablePwmClock   (myClk, PWM_Number_1);
    PWM_setSyncMode      (myPwm1, PWM_SyncMode_EPWMxSYNC);      // Setup Sync
    PWM_enableCounterLoad(myPwm1);                              // Allow each timer to be sync'ed
    PWM_setPhase         (myPwm1, 100);
    PWM_setPeriod        (myPwm1, PWM1_TIMER_TBPRD);
    PWM_setCounterMode   (myPwm1, PWM_CounterMode_Up);          // Count up
    PWM_setIntMode       (myPwm1, PWM_IntMode_CounterEqualZero);// Select INT on Zero event
    PWM_enableInt        (myPwm1);                              // Enable INT
    PWM_setIntPeriod     (myPwm1, PWM_IntPeriod_FirstEvent);    // Generate INT on 1st event
#endif //(1==PWM1_INT_ENABLE)

#if (1==PWM2_INT_ENABLE)
    CLK_enablePwmClock   (myClk, PWM_Number_2);
    PWM_setSyncMode      (myPwm2, PWM_SyncMode_EPWMxSYNC);      // Setup Sync
    PWM_enableCounterLoad(myPwm2);                              // Allow each timer to be sync'ed
    PWM_setPhase         (myPwm2, 200);
    PWM_setPeriod        (myPwm2, PWM2_TIMER_TBPRD);
    PWM_setCounterMode   (myPwm2, PWM_CounterMode_Up);          // Count up
    PWM_setIntMode       (myPwm2, PWM_IntMode_CounterEqualZero);// Enable INT on Zero event
    PWM_enableInt        (myPwm2);                              // Enable INT
    PWM_setIntPeriod     (myPwm2, PWM_IntPeriod_SecondEvent);   // Generate INT on 2nd event
#endif //(1==PWM2_INT_ENABLE)

#if (1==PWM3_INT_ENABLE)
/*
    CLK_enablePwmClock   (myClk, PWM_Number_3);
    PWM_setSyncMode      (myPwm3, PWM_SyncMode_EPWMxSYNC);      // Setup Sync
    //PWM_setSyncMode      (myPwm3, PWM_SyncMode_CounterEqualZero);      // Setup Sync
    PWM_enableCounterLoad(myPwm3);                              // Allow each timer to be sync'ed
    PWM_setPhase         (myPwm3, 300);
    //PWM_setPhase         (myPwm3, 0);
    //PWM_setPeriod        (myPwm3, PWM3_TIMER_TBPRD);
    PWM_setPeriod        (myPwm3, 0x0fff);
    PWM_setCounterMode   (myPwm3, PWM_CounterMode_Up);          // Count up
    PWM_setIntMode       (myPwm3, PWM_IntMode_CounterEqualZero);// Enable INT on Zero event
    PWM_enableInt        (myPwm3);                              // Enable INT
    PWM_setIntPeriod     (myPwm3, PWM_IntPeriod_ThirdEvent);    // Generate INT on 3rd event
*/
    //=====================================================================
    // (Note: code for only 3 modules shown)
    // Initialization Time
    //========================
    /*
    // EPWM Module 1 config
    EPwm1Regs.TBPRD = 1200; // Period = 1201 TBCLK counts
    EPwm1Regs.TBPHS.half.TBPHS = 0; // Set Phase register to zero
    EPwm1Regs.TBCTL.bit.CTRMODE = TB_COUNT_UP; // Asymmetrical mode
    EPwm1Regs.TBCTL.bit.PHSEN = TB_DISABLE; // Phase loading disabled
    EPwm1Regs.TBCTL.bit.PRDLD = TB_SHADOW;
    EPwm1Regs.TBCTL.bit.SYNCOSEL = TB_SYNC_DISABLE;
    EPwm1Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;
    EPwm1Regs.CMPCTL.bit.SHDWBMODE = CC_SHADOW;
    EPwm1Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO; // load on CTR=Zero
    EPwm1Regs.CMPCTL.bit.LOADBMODE = CC_CTR_ZERO; // load on CTR=Zero
    EPwm1Regs.AQCTLA.bit.PRD = AQ_CLEAR;
    EPwm1Regs.AQCTLA.bit.CAU = AQ_SET;
    // EPWM Module 2 config
    EPwm2Regs.TBPRD = 1400; // Period = 1401 TBCLK counts
    EPwm2Regs.TBPHS.half.TBPHS = 0; // Set Phase register to zero
    EPwm2Regs.TBCTL.bit.CTRMODE = TB_COUNT_UP; // Asymmetrical mode
    EPwm2Regs.TBCTL.bit.PHSEN = TB_DISABLE; // Phase loading disabled
    EPwm2Regs.TBCTL.bit.PRDLD = TB_SHADOW;
    EPwm2Regs.TBCTL.bit.SYNCOSEL = TB_SYNC_DISABLE;
    EPwm2Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;
    EPwm2Regs.CMPCTL.bit.SHDWBMODE = CC_SHADOW;
    EPwm2Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO; // load on CTR=Zero
    EPwm2Regs.CMPCTL.bit.LOADBMODE = CC_CTR_ZERO; // load on CTR=Zero
    EPwm2Regs.AQCTLA.bit.PRD = AQ_CLEAR;
    EPwm2Regs.AQCTLA.bit.CAU = AQ_SET;
    */
    CLK_enablePwmClock   (myClk, PWM_Number_2);
    // EPWM Module 3 config
    EPwm3Regs.TBPRD = 800; // Period = 801 TBCLK counts
    EPwm3Regs.TBPHS.half.TBPHS = 0; // Set Phase register to zero
    EPwm3Regs.TBCTL.bit.CTRMODE = TB_COUNT_UP;
    EPwm3Regs.TBCTL.bit.PHSEN = TB_DISABLE; // Phase loading disabled
    EPwm3Regs.TBCTL.bit.PRDLD = TB_SHADOW;
    EPwm3Regs.TBCTL.bit.SYNCOSEL = TB_SYNC_DISABLE;
    EPwm3Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;
    EPwm3Regs.CMPCTL.bit.SHDWBMODE = CC_SHADOW;
    EPwm3Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO; // load on CTR=Zero
    EPwm3Regs.CMPCTL.bit.LOADBMODE = CC_CTR_ZERO; // load on CTR=Zero
    EPwm3Regs.AQCTLB.bit.PRD = AQ_TOGGLE; //AQ_CLEAR;
    EPwm3Regs.AQCTLB.bit.CBU = AQ_TOGGLE; //AQ_SET;
    // Run Time (Note: Example execution of one run-time instant)
    //=========================================================
    //EPwm1Regs.CMPA.half.CMPA = 700; // adjust duty for output EPWM1A
    //EPwm2Regs.CMPA.half.CMPA = 700; // adjust duty for output EPWM2A
    EPwm3Regs.CMPB = 500; // adjust duty for output EPWM3A

    PWM_setIntMode   (myPwm3, PWM_IntMode_CounterEqualZero);// Enable INT on Zero event
    PWM_enableInt    (myPwm3);                              // Enable INT
    PWM_setIntPeriod (myPwm3, PWM_IntPeriod_SecondEvent);   // Generate INT on 2nd event
#endif //(1==PWM3_INT_ENABLE)

    // Start all the timers synced
    CLK_enableTbClockSync(myClk);
#endif //(1==USE_F28027_PWM)
}
/* ========================================================================== */



/* ==========================================================================
 * NAME - epwm1_timer_isr
 * IN   - void
 * OUT  - void
 * RET  - void
   ========================================================================== */
// Interrupt routines uses in this example:
interrupt void epwm1_timer_isr (void) {
#if (1==USE_F28027_PWM)
#if (1==PWM1_INT_ENABLE)
    EPwm1TimerIntCount++;

    // Clear INT flag for this timer
    PWM_clearIntFlag(myPwm1);

    // Acknowledge this interrupt to receive more interrupts from group 3
    PIE_clearInt(myPie, PIE_GroupNumber_3);

    //GPIO_setPortData (myGpio, GPIO_Port_A, ++i_pwm3 & 0x1 ? 0xE : 0xF );
#endif //(1==PWM1_INT_ENABLE)
#endif //(1==USE_F28027_PWM)
}
/* ========================================================================== */



/* ==========================================================================
 * NAME - epwm2_timer_isr
 * IN   - void
 * OUT  - void
 * RET  - void
   ========================================================================== */
interrupt void epwm2_timer_isr (void) {
#if (1==USE_F28027_PWM)
#if (1==PWM2_INT_ENABLE)
    EPwm2TimerIntCount++;

    // Clear INT flag for this timer
    PWM_clearIntFlag(myPwm2);

    // Acknowledge this interrupt to receive more interrupts from group 3
    PIE_clearInt(myPie, PIE_GroupNumber_3);
#endif //(1==PWM2_INT_ENABLE)
#endif //(1==USE_F28027_PWM)
}
/* ========================================================================== */



/* ==========================================================================
 * NAME - epwm3_timer_isr
 * IN   - void
 * OUT  - void
 * RET  - void
   ========================================================================== */
interrupt void epwm3_timer_isr (void) {
#if (1==USE_F28027_PWM)
#if (1==PWM3_INT_ENABLE)
    EPwm3TimerIntCount += 10;
    //PWM_setPeriod (myPwm3, (uint16_t)EPwm3TimerIntCount);
/*
    //-------------------
    CLK_enablePwmClock   (myClk, PWM_Number_3);
    //PWM_setSyncMode      (myPwm3, PWM_SyncMode_EPWMxSYNC);      // Setup Sync
    PWM_setSyncMode      (myPwm3, PWM_SyncMode_CounterEqualZero);      // Setup Sync
    PWM_enableCounterLoad(myPwm3);                              // Allow each timer to be sync'ed
    PWM_setPhase         (myPwm3, 0);
    PWM_setPeriod        (myPwm3, 0xffff);
    //PWM_setChoppingDutyCycle
    PWM_setCounterMode   (myPwm3, PWM_CounterMode_Up);          // Count up
    PWM_setIntMode       (myPwm3, PWM_IntMode_CounterEqualZeroOrPeriod);// Enable INT on Zero event
    PWM_enableInt        (myPwm3);                              // Enable INT
    PWM_setIntPeriod     (myPwm3, PWM_IntPeriod_ThirdEvent);    // Generate INT on 3rd event
    //-------------------

    //PWM_setActionQual_CntUp_CmpB_PwmB(myPwm3, PWM_ActionQual_Toggle);
    PWM_setActionQual_Period_PwmB(myPwm3, PWM_ActionQual_Toggle);
*/
    // Clear INT flag for this timer
    PWM_clearIntFlag(myPwm3);

    // Acknowledge this interrupt to receive more interrupts from group 3
    PIE_clearInt(myPie, PIE_GroupNumber_3);

    //GPIO_setPortData (myGpio, GPIO_Port_A, ++i_pwm3 & 0x1 ? 0xE : 0xF );
    if ( ++i_pwm3 & 0x1)  GpioDataRegs.GPADAT.bit.GPIO0 = 1;
    else                  GpioDataRegs.GPADAT.bit.GPIO0 = 0;
#endif //(1==PWM3_INT_ENABLE)
#endif //(1==USE_F28027_PWM)
}
/* ========================================================================== */



/* ==========================================================================
 * NAME - timer0_isr
 * IN   - void
 * OUT  - void
 * RET  - void
   ========================================================================== */
interrupt void timer0_isr (void) {
#if (1==USE_F28027_TIMER)
    Timer0IntCount++;

    // Clear INT flag for this timer
    TIMER_clearFlag(myTimer);

    // Acknowledge this interrupt to receive more interrupts from group 3
    //TIMER_clearIntFlag(myTimer);
#endif //(1==USE_F28027_TIMER)
}
/* ========================================================================== */



/* ==========================================================================
 * NAME - error
 * IN   - void
 * OUT  - void
 * RET  - void
   ========================================================================== */
void error(void) {
    asm(" ESTOP0"); // Test failed!! Stop!
    for (;;){ }
}
/* ========================================================================== */



/* ==========================================================================
 * NAME - wrapper_Init_UART_IRQ
 * IN   - void
 * OUT  - void
 * RET  - t_error err
   ========================================================================== */
t_error wrapper_Init_UART_IRQ (void) {

#if (1==USE_F28027_SCI)
	// Initalize GPIO
	GPIO_setPullUp(myGpio, GPIO_Number_28, GPIO_PullUp_Enable);
	GPIO_setPullUp(myGpio, GPIO_Number_29, GPIO_PullUp_Disable);
	GPIO_setQualification(myGpio, GPIO_Number_28, GPIO_Qual_ASync);
	GPIO_setMode(myGpio, GPIO_Number_28, GPIO_28_Mode_SCIRXDA);
	GPIO_setMode(myGpio, GPIO_Number_29, GPIO_29_Mode_SCITXDA);

    // Step ... Clear all interrupts and initialize PIE vector table:
    // Disable CPU interrupts
       DINT;

    // Initialize PIE control registers to their default state.
    // The default state is all PIE interrupts disabled and flags
    // are cleared.
    // This function is found in the DSP2802x_PieCtrl.c file.
       InitPieCtrl();

    // Disable CPU interrupts and clear all CPU interrupt flags:
       IER = 0x0000;
       IFR = 0x0000;

    // Initialize the PIE vector table with pointers to the shell Interrupt
    // Service Routines (ISR).
    // This will populate the entire table, even if the interrupt
    // is not used in this example.  This is useful for debug purposes.
    // The shell ISR routines are found in DSP2802x_DefaultIsr.c.
    // This function is found in DSP2802x_PieVect.c.
       InitPieVectTable();

    // Interrupts that are used in this example are re-mapped to
    // ISR functions found within this file.
       EALLOW;	// This is needed to write to EALLOW protected registers
       PieVectTable.SCIRXINTA = &sciaRxFifoIsr;
       PieVectTable.SCITXINTA = &sciaTxFifoIsr;
       EDIS;   // This is needed to disable write to EALLOW protected registers

    // Step ... Initialize all the Device Peripherals:
    // This function is found in DSP2802x_InitPeripherals.c
    // InitPeripherals(); // Not required for this example
       scia_fifo_init();  // Init SCI-A

    // Step ... User specific code, enable interrupts:

    // Init send data.  After each transmission this data
    // will be updated for the next transmission
       /*for(i = 0; i<2; i++) {  sdataA[i] = i;  }
       rdata_pointA = sdataA[0];*/

    // Enable interrupts required for this example
       PieCtrlRegs.PIECTRL.bit.ENPIE = 1;   // Enable the PIE block
       PieCtrlRegs.PIEIER9.bit.INTx1=1;     // PIE Group 9, INT1
       PieCtrlRegs.PIEIER9.bit.INTx2=1;     // PIE Group 9, INT2
       IER = 0x100;	// Enable CPU INT
       EINT;
#endif //(1==USE_F28027_SCI)

    return E_OK;
}
/* ========================================================================== */



/* ==========================================================================
 * NAME - wrapper_Init_TM1638
 * IN   - void
 * OUT  - void
 * RET  - t_error err
   ========================================================================== */
t_error wrapper_Init_TM1638 (void) {

#if (1==__USE_TM1638__)
	tm1638_init();
#endif //(1==__USE_TM1638__)

    return E_OK;
}
/* ========================================================================== */



/* ==========================================================================
 * NAME - scia_fifo_init
 * IN   - void
 * OUT  - void
 * RET  - void
   ========================================================================== */
void scia_fifo_init (void) {
#if (1==USE_F28027_SCI)

/*	SysCtrlRegs.PCLKCR0.bit.SCIAENCLK  = 1;  // SCI-A
	SciaRegs.SCICCR.all =0x0007;  // 1-Stop bit, No loopback, No parity,
                                  // 8-char bits, async mode, idle-line protocol
	SciaRegs.SCICTL1.all =0x0003; // enable TX, RX, internal SCICLK,
                                  // Disable RX ERR, SLEEP, TXWAKE
	SciaRegs.SCICTL2.bit.TXINTENA   = 1;
	SciaRegs.SCICTL2.bit.RXBKINTENA = 1;
	SciaRegs.SCIHBAUD = 0x0000;
	SciaRegs.SCILBAUD = 15; //15=115200 //SCI_BAUD;
    //SciaRegs.SCICCR.bit.LOOPBKENA = 0; // Enable/Disable loop back
	SciaRegs.SCIFFTX.all = 0xC022;
	SciaRegs.SCIFFRX.all = 0x0022;
	SciaRegs.SCIFFCT.all = 0x00;
	SciaRegs.SCICTL1.all = 0x0023;     // Relinquish SCI from Reset
	SciaRegs.SCIFFTX.bit.TXFIFOXRESET = 1;
	SciaRegs.SCIFFRX.bit.RXFIFORESET = 1;  */

    CLK_enableSciaClock (myClk);

    // 1 stop bit, No loopback, No parity, 8 bits, Async-Mode, Idle-Line protocol
    SCI_disableParity(mySci);
    SCI_setNumStopBits(mySci, SCI_NumStopBits_One);
    SCI_setCharLength(mySci, SCI_CharLength_8_Bits);

    // enable TX, RX, internal SCICLK, Disable RX ERR, SLEEP, TXWAKE
    SCI_enableTx(mySci);
    SCI_enableRx(mySci);
    SCI_enableTxInt(mySci);
    SCI_enableRxInt(mySci);
    //SCI_enableLoopBack(mySci);
    SCI_setTxFifoIntLevel( mySci, SCI_FifoLevel_Empty );
    SCI_setRxFifoIntLevel( mySci, SCI_FifoLevel_1_Word );
    //SCI_disableFifoEnh(mySci);

    // SCI BRR = LSPCLK/(SCI BAUDx8) - 1
#if (CPU_FRQ_50MHZ)
    //SCI_setBaudRate(mySci, SCI_BaudRate_9_6_kBaud);
    SCI_setBaudRate(mySci, /*SCI_BaudRate_115_2_kBaud*/ (SCI_BaudRate_e)13);
#elif (CPU_FRQ_40MHZ)
    SCI_setBaudRate(mySci, (SCI_BaudRate_e)129);
#endif
    SCI_enable(mySci);

#endif //(1==USE_F28027_SCI)
}
/* ========================================================================== */



/* ==========================================================================
 * NAME - sciaTxFifoIsr
 * IN   - void
 * OUT  - void
 * RET  - void
   ========================================================================== */
interrupt void sciaTxFifoIsr (void) {
#if (1==USE_F28027_SCI)
    switch (sys_stat.sci.tx){
    	case SCI_TX_READY:
    	break;

    	case SCI_TX_SENDING:
	        SCI_enableRxInt(mySci);
	        SCI_enableTxInt(mySci);
    		if (i_tx < RX_LEN) {
    	        SciaRegs.SCITXBUF=rdataA[i_tx];     // Send data
    	        //SciaRegs.SCITXBUF=RxTx;              // Send data
        		//SciaRegs.SCICTL2.bit.TXINTENA=1;
        		SciaRegs.SCIFFTX.bit.TXFFINTCLR=1;  // Clear SCI Interrupt flag
        		PieCtrlRegs.PIEACK.all |= 0x100;    // Issue PIE ACK
    			sys_stat.sci.tx = SCI_TX_SENDING;
    			i_tx++;
    		    //EALLOW;	// This is needed to write to EALLOW protected registers
    			//DINT;
    			//IER = 0x100;	// Enable CPU INT
    			//IFR = 0x100;
    			//EINT;
 		        //EDIS;   // This is needed to disable write to EALLOW protected registers
    		} else {
    			sys_stat.sci.tx = SCI_TX_FINISH;
    			i_tx=0;

        		SciaRegs.SCIFFTX.bit.TXFFINTCLR=1;	      // Clear SCI Interrupt flag
        		PieCtrlRegs.PIEACK.all |= 0x100;          // Issue PIE ACK
        	    //SCI_clearTxFifoInt(mySci);              // Clear SCI Interrupt flag
        	    //PIE_clearInt(myPie, PIE_GroupNumber_9); // Issue PIE ACK
        	    SCI_disableTxInt(mySci);
         		}
    	break;

    	case SCI_TX_FINISH:
    	break;
    }
#endif //(1==USE_F28027_SCI)
}
/* ========================================================================== */



/* ==========================================================================
 * NAME - sciaRxFifoIsr
 * IN   - void
 * OUT  - void
 * RET  - void
   ========================================================================== */
interrupt void sciaRxFifoIsr (void) {
#if (1==USE_F28027_SCI)
    static Uint16 i=0;

	if (SciaRegs.SCIRXBUF.bit.RXDT=='_') {
		i=0;
		sys_stat.sci.tx = SCI_TX_READY;
	}

    switch (sys_stat.sci.tx){
    	case SCI_TX_READY:
     	    if ( i < RX_LEN ) {
    	    	rdataA[i++]=SciaRegs.SCIRXBUF.all;	 // Read data

    	    	SciaRegs.SCIFFRX.bit.RXFFOVRCLR=1;   // Clear Overflow flag
    	    	SciaRegs.SCIFFRX.bit.RXFFINTCLR=1;   // Clear Interrupt flag
    	    	PieCtrlRegs.PIEACK.all |= 0x100;     // Issue PIE ack
    	    } else {
    	    	i=0;

    			sys_stat.sci.tx = SCI_TX_SENDING;
    			i_tx=0;
    	        SCI_enableTxInt(mySci);
    	    	RxTx='>';
    	        SciaRegs.SCITXBUF=RxTx;              // Send data

        		SciaRegs.SCIFFTX.bit.TXFFINTCLR=1;   // Clear SCI Interrupt flag
        		PieCtrlRegs.PIEACK.all |= 0x100;     // Issue PIE ACK
    	    }
    	break;

    	case SCI_TX_SENDING:
    	break;

    	case SCI_TX_FINISH:
    	break;
    }
#endif //(1==USE_F28027_SCI)
}
/* ========================================================================== */



//===========================================================================
// No more.
//===========================================================================
