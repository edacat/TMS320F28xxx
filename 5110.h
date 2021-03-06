/* ************************************************************************** */
#ifndef __5110_H_
#define __5110_H_
/* ************************************************************************** */
#include  "init.h"

#include "./include/DSP28x_Project.h"
#include "./include/F2802x_Examples.h"
#include "./include/F2802x_GlobalPrototypes.h"

/* ************************************************************************** */
#define FALSE              0
#define TRUE               1

// ���������� ������� � ��������
//#define LCD_X_RES                  84    // ���������� �� �����������
//#define LCD_Y_RES                  48    // ���������� �� ���������

// ��������� ��� ��������� ������ ��������������� �������� LcdBars ( byte data[], byte numbBars, byte width, byte multiplier )
//#define EMPTY_SPACE_BARS           2     // ���������� ����� ����������������
//#define BAR_X                      30    // ���������� x
//#define BAR_Y                      47    // ���������� y

// ������ ���� ( 84 * 48 ) / 8 = 504 �����
//#define LCD_CACHE_SIZE     ( ( LCD_X_RES * LCD_Y_RES ) / 8 )

// ��� ������������ ��������
#define OK                         0   // ������������ ���������
#define OUT_OF_BORDER              1   // ����� �� ������� �������
#define OK_WITH_WRAP               2   // ������� �� ������ (�������� �������������� ��������� ������� ��� ������ �������� ������)
/* ************************************************************************** */


/* ************************************************************************** */
/*
// ������������
typedef enum {
    LCD_CMD  = 0,     // �������
    LCD_DATA = 1      // ������

} LcdCmdData;

typedef enum {
    PIXEL_OFF =  0,   // �������� ������� �������
    PIXEL_ON  =  1,   // �������� ������� �������
    PIXEL_XOR =  2    // ������������� �������

} LcdPixelMode;

typedef enum {
    FONT_1X = 1,      // ������� ������ ������ 5x7
    FONT_2X = 2       // ����������� ������ ������
} LcdFontSize;
*/
/* ************************************************************************** */


/* ************************************************************************** */
//#define gpio_mux  0x0000
//#define gpio_dir  0x003f

/* *** Version 1 ************************************************************ */
// j6   | pin   |
// -----+-------+---------------------------------------------------------------
// RST  | GPIO0 |   �����
// CE   | GPIO1 |   ���������
// DC   | GPIO2 |   ������� / ������
// SDIN | GPIO3 |   ���������������� ���� ������
// SCLK | GPIO4 |   ���������������� ���� ������
/* ************************************************************************** */

/* *** Version 2 ************************************************************ */
// j6    | pin       |
// ------+-----------+----------------------------------------------------------
// RST   | GPIO7     |   �����
// CE    | GPIO6     |   ���������
// DC    | GPIO17/33 |   ������� / ������
// SDIN  | GPIO16/32 |   ���������������� ���� ������
// SCLK  | GPIO12    |   ���������������� ���� ������
// LIGHT | GPIO12    |   ���������
/* ************************************************************************** */

/* ************************************************************************** */
/*#define rst_h     GpioDataRegs.GPASET.bit.GPIO0=1
#define rst_l     GpioDataRegs.GPACLEAR.bit.GPIO0=1
#define ce_h      GpioDataRegs.GPASET.bit.GPIO1=1
#define ce_l      GpioDataRegs.GPACLEAR.bit.GPIO1=1
#define dc_h      GpioDataRegs.GPASET.bit.GPIO2=1
#define dc_l      GpioDataRegs.GPACLEAR.bit.GPIO2=1
#define sdin_h    GpioDataRegs.GPASET.bit.GPIO3=1
#define sdin_l    GpioDataRegs.GPACLEAR.bit.GPIO3=1
#define sclk_h    GpioDataRegs.GPASET.bit.GPIO4=1
#define sclk_l    GpioDataRegs.GPACLEAR.bit.GPIO4=1*/
/* ************************************************************************** */
/* ************************************************************************** */
#if (1==__TMS320__)
// Version 1
/*#define rst_h     GpioDataRegs.GPASET.bit.GPIO0=1
#define rst_l     GpioDataRegs.GPACLEAR.bit.GPIO0=1
#define ce_h      GpioDataRegs.GPASET.bit.GPIO1=1
#define ce_l      GpioDataRegs.GPACLEAR.bit.GPIO1=1
#define dc_h      GpioDataRegs.GPASET.bit.GPIO2=1
#define dc_l      GpioDataRegs.GPACLEAR.bit.GPIO2=1
#define sdin_h    GpioDataRegs.GPASET.bit.GPIO3=1
#define sdin_l    GpioDataRegs.GPACLEAR.bit.GPIO3=1
#define sclk_h    GpioDataRegs.GPASET.bit.GPIO4=1
#define sclk_l    GpioDataRegs.GPACLEAR.bit.GPIO4=1*/

// Version 2
#define rst_h     GpioDataRegs.GPASET.bit.GPIO7=1
#define rst_l     GpioDataRegs.GPACLEAR.bit.GPIO7=1
#define ce_h      GpioDataRegs.GPASET.bit.GPIO6=1
#define ce_l      GpioDataRegs.GPACLEAR.bit.GPIO6=1
#define dc_h      GpioDataRegs.GPASET.bit.GPIO17=1
#define dc_l      GpioDataRegs.GPACLEAR.bit.GPIO17=1
#define sdin_h    GpioDataRegs.GPASET.bit.GPIO16=1
#define sdin_l    GpioDataRegs.GPACLEAR.bit.GPIO16=1
#define sclk_h    GpioDataRegs.GPASET.bit.GPIO12=1
#define sclk_l    GpioDataRegs.GPACLEAR.bit.GPIO12=1
/* ************************************************************************** */
#endif //(1==__TMS320__)

/* ************************************************************************** */
//extern void init_sys();
extern void DELAY (char);
extern void InitGpio_Conf_HW ();
extern void init_5110 ();
extern void set_row (char);
extern void set_col (char);
extern void write_com (char);
extern void write_data (char);
extern void InitSysCtrl (void);
extern void InitPieCtrl (void);
extern void InitPieVectTable (void);

extern void lcd_clear ();

#if 0
extern void LcdContrast ( byte );
extern void LcdSend ( byte data, LcdCmdData cd );
#endif //0
/* ************************************************************************** */

#endif //__5110_H_
