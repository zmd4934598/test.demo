//Project: 115.prj
// Device: EN8F152
// Memory: Flash 1KX14b, EEPROM 256X8b, SRAM 64X8b
// Author: 
//Company: 
//Version:
/*               ----------------
*  VDD-----------|1(VDD)    (GND)8|------------GND     
*  D5-------------|2(PA2)    (PA4)7|-------------MDC 
*  D4-------------|3(PA1)    (PA5)6|-------------MDIO
*  NC-------------|4(PA3)     (PA0)5|-------------RESETB
*			      ----------------
*/
//*********************************************************
#include "SYSCFG.h"
#define OSC_16M  0X70
#define OSC_8M   0X60
#define OSC_4M   0X50
#define OSC_2M   0X40
#define OSC_1M   0X30
#define OSC_500K 0X20
#define OSC_250K 0X10
#define OSC_32K  0X00

#define WDT_256K 0X80
#define WDT_32K  0X00//暂时没有用到
//**********************************************************
//***********************宏定义*****************************
#define  unchar     unsigned char 
#define  unint      unsigned int
#define  unlong     unsigned long

#define  D4	RA2   //
#define  D5	RA1   //
#define  RESETB	RA0
#define  MDIO	RA5
#define  MDC	RA4



/***************************
MDC时钟的一个周期
***************************/
void SMI_Clock_01()
{
		MDC=0;
		NOP();
		MDC=1;
		NOP();
}

/***************************
MDIO输出数据0
MDIO在MDC的下降沿输出
***************************/
void SMI_Data_0()
{
		MDC=0;
		//s_nop_();
		MDIO=0;
		NOP();
		MDC=1;
		NOP();
		
}
/***************************
MDIO输出数据1
***************************/

void SMI_Data_1()
{
		MDC=0;
		//s_nop_();
		MDIO=1;
		NOP();
		MDC=1;
		NOP();
}
/***************************
/*START
***************************/
void SMI_Start() 
{
	SMI_Data_0();
	SMI_Data_1();
}
/***************************
/*READ_OP_CODE
***************************/
void SMI_R_opcode()
{
	SMI_Data_1();
	SMI_Data_0();
}
/***************************
/*WRITE_OP_CODE
***************************/
void SMI_W_opcode()
{
	SMI_Data_0();
	SMI_Data_1();
}
/*********************************************
通过SMI写入数据，仅byte的低5位bit
PHYADDER是PHY地址为0，1，2，3，4这;第一个以太网口为0
REGADDER为PHY的寄存器地址,一共32个寄存器
*********************************************/
void SMI_adder_reg(unsigned char byte)
{
   unsigned char i;
   for(i=0;i<5;i++)
   {
    if(byte&0x10)SMI_Data_1();
    else  SMI_Data_0();
    byte=byte<<1;
    }
}
//READ_TURN AROUND
void SMI_R_TA()
{ 
  SMI_Clock_01();
  SMI_Clock_01();
 
}
//WRITE_TURN AROUND
void SMI_W_TA()
{
  SMI_Data_0();
  SMI_Data_1();
}

//IDLE
void SMI_Idle()
{
   unsigned char i;
   for(i=0;i<32;i++)
    SMI_Data_1();
}
/*********************************************
通过SMI读寄存器内容
PHYADDER是PHY地址为0，1，2，3，4这里第一个以太网口为0
REGADDER为PHY的寄存器地址,return一个16位数据
*********************************************/
unsigned int SMI_read(unsigned char PHYADDER,unsigned char REGADDER)
{
    unsigned int value,i;
    TRISA = 0B11001000;	//PA输入输出 0-输出 1-输入
	  //TRISA4 =0;		/PA4->输出PA2->输出PA1->输出
    WPUA  = 0B00110111; //PA端口上拉控制 1-开上拉 0-关上拉
	  //WPUA2 = 1;//    //开PA2上拉开PA4上拉开PA5上拉
	SMI_Idle();//先发送闲位
	SMI_Start();
	SMI_R_opcode();
	SMI_adder_reg(PHYADDER);
	SMI_adder_reg(REGADDER);
	MDIO = 1;
	SMI_R_TA();
	for(i=0;i<16;i++)
	{
	  value=value<<1;
	  MDC=0;
	  TRISA = 0B11101000;	//PA输入输出 0-输出 1-输入
	  //TRISA4 =0;		    //PA4->输出PA3->输入 PA2->输入
	  WPUA = 0B00010111;     //PA端口上拉控制 1-开上拉 0-关上拉
	  //WPUA2 = 1;//		//开PA2上拉开PA4上拉开PA5上拉
	  if(MDIO==1)value=value|0x0001;
	  NOP();
      MDC=1;
      NOP();
	}
	SMI_Idle();
	return value;
}
/*********************************************
通过SMI写寄存器内容
PHYADDER是PHY地址为0，1，2，3，4这里第一个以太网口为0
REGADDER为PHY的寄存器地址,写一个16位数据
*********************************************/
void SMI_write(unsigned char PHYADDER,unsigned char REGADDER,unsigned int value)
{
    unsigned int i;
	TRISA = 0B11001000;	//PA输入输出 0-输出 1-输入
	  //TRISA4 =0;		/PA4->输出PA2->输出PA1->输出
    WPUA  = 0B00110110; //PA端口上拉控制 1-开上拉 0-关上拉
	  //WPUA2 = 1;//    //开PA2上拉开PA4上拉开PA5上拉
	SMI_Idle();
	SMI_Start();
	SMI_W_opcode();
	SMI_adder_reg(PHYADDER);
	SMI_adder_reg(REGADDER);
	SMI_W_TA();
	for(i=0;i<16;i++)
	{
	  if(value&0x8000)
	       SMI_Data_1();
	  else SMI_Data_0();
	  value=value<<1;
	}
	SMI_Idle();
}
/********************************
Speed的寄存器地址为0-13，10M为0，100M为1
inbuf1[1]==2
Auto为0.12
*********************************/

void Set_RTL8305_Speed(unsigned char PHYADDER,unsigned char value)
{
   unsigned int reg_data;
   unsigned char done=0;
   reg_data=SMI_read(PHYADDER,0);
   if(reg_data&0x1000)
	 reg_data&=~(0x1000);

   if(value==0)
   {
     if(reg_data&0x2000)
	 {
	   reg_data&=~(0x2000);
//	  SMI_write(PHYADDER,0,reg_data);
	   done=1;
	 }
   }
   else
   {
     if(~reg_data&0x2000)
	 {
	   reg_data|=0x2000;
//	   SMI_write(PHYADDER,0,reg_data);
	   done=1;
	 }
   }
   if(done==1)SMI_write(PHYADDER,0,reg_data);
   

}
/********************************
Duplex的寄存器地址为0-8，半双工为0，全双工为1
inbuf1[1]==3
Auto为0.12
*********************************/

void Set_RTL8305_Duplex(unsigned char PHYADDER,unsigned char value)
{
   unsigned int reg_data;
   unsigned char done=0;                        //进行速率设置抄作为1
   reg_data=SMI_read(PHYADDER,0);
   if(reg_data&0x1000)
	 reg_data&=~(0x1000);

   if(value==0)
   {
     if(reg_data&0x0100)
	 {
	   reg_data&=~(0x0100);
//	   SMI_write(PHYADDER,0,reg_data);
	   done=1;
	 }
   }
   else
   {
     if(~reg_data&0x0100)
	 {
	   reg_data|=0x0100;
//	   SMI_write(PHYADDER,0,reg_data);
	   done=1;
	 }
   }
   if(done==1)SMI_write(PHYADDER,0,reg_data);
   

}

/********************************
Auto的寄存器地址为0-12，禁用为0，使能为1
inbuf1[1]==4
*********************************/

void Set_RTL8305_Auto(unsigned char PHYADDER,unsigned char value)
{
   unsigned int reg_data;
   unsigned char done=0;                        //进行速率设置抄作为1
   reg_data=SMI_read(PHYADDER,0);
   
   if(value==0)
   {
     if(reg_data&0x1000)
	 {
	   reg_data&=~(0x1000);
//	   SMI_write(PHYADDER,0,reg_data);
	   done=1;
	 }
   }
   else
   {
     if(~reg_data&0x1000)
	 {
	   reg_data|=0x1000;
//	   SMI_write(PHYADDER,0,reg_data);
	   done=1;
	 }
   }
   if(done==1)SMI_write(PHYADDER,0,reg_data);
   

}

/********************************
reset的寄存器地址为0-15，正常为0，复位为1
inbuf1[1]==5
*********************************/
void Set_RTL8305_reset(unsigned char PHYADDER)
{
	unsigned int reg_data;
	reg_data = SMI_read(PHYADDER, 0);
	reg_data |= (1<<15);
	SMI_write(PHYADDER, 0, reg_data);
	do                                       
	{
		reg_data = SMI_read(PHYADDER, 0);	
	}while(reg_data & (1<<15));         //检查是否复位结束
}

void RTL8305_Set()
{
    unsigned char DataBuff[2];
	unsigned int reg_data;
//	if(inbuf1[1]==2)
//	     Set_RTL8305_Speed(inbuf1[2],inbuf1[3]);
//	else if(inbuf1[1]==3)
//	     Set_RTL8305_Duplex(inbuf1[2],inbuf1[3]);
//	else if(inbuf1[1]==4) 
//	     Set_RTL8305_Auto(inbuf1[2],inbuf1[3]);
//	else  Set_RTL8305_reset(inbuf1[2]);

 //   reg_data=SMI_read(inbuf1[2],0);
    DataBuff[1]=reg_data;
    DataBuff[0]=reg_data>>8;
   // send_string(DataBuff,2);
}
/*-------------------------------------------------
 *  函数名：POWER_INITIAL
 *	功能：  上电系统初始化
 *  输入：  无
 *  输出：  无
 --------------------------------------------------*/	
void POWER_INITIAL (void) 
{ 
	OSCCON = WDT_32K|OSC_16M|0X01;//INROSC
///	OSCCON = 0B01110001;//WDT 32KHZ IRCF=111=16MHZ/4=4MHZ,0.25US/T
					 //Bit0=1,系统时钟为内部振荡器
					 //Bit0=0,时钟源由FOSC<2：0>决定即编译选项时选择

	INTCON = 0;  //暂禁止所有中断
	PORTA = 0B00000000;//
	TRISA = 0B11001000;	//PA输入输出 0-输出 1-输入
	//TRISA4 =0;		/PA4->输出PA1->输出 PA0->输入
	WPUA = 0B00110111;     //PA端口上拉控制 1-开上拉 0-关上拉
	//WPUA2 = 1;//		//开PA2上拉开PA4上拉开PA5上拉
//	OPTION = 0B00001000;//Bit4=1 WDT MODE,PS=000=1:1 WDT RATE
//					 //Bit7(RAPU)=0 ENABLED PULL UP PA
//	MSCKCON = 0B00000000;//6B->0,禁止PA4，PC5稳压输出
//					  //5B->0,TIMER2时钟为Fosc
//					  //4B->0,禁止LVR       
	 
}
/*-------------------------------------------------
 *  函数名称：DelayUs
 *  功能：    短延时函数 --16M-4T--大概快1%左右.
 *  输入参数：Time  延时时间长度   延时时长Time*2Us
 * 	返回参数：无 
 -------------------------------------------------*/
void DelayUs(unsigned char Time)
{
	unsigned char a;
	for(a=0;a<Time;a++)
	{
		NOP();
	}
}                  
/*------------------------------------------------- 
 * 	函数名称：DelayMs
 * 	功能：    短延时函数
 * 	输入参数：Time  延时时间长度   延时时长Timems
 * 	返回参数：无 
 -------------------------------------------------*/
void DelayMs(unsigned char Time)
{
	unsigned char a,b;
	for(a=0;a<Time;a++)
	{
		for(b=0;b<5;b++)
		{
		 	DelayUs(98); //快1%
		}
	}
}
/*------------------------------------------------- 
 * 	函数名称：DelayS
 * 	功能：    短延时函数
 * 	输入参数：Time  延时时间长度   延时时长TimeS
 * 	返回参数：无 
 -------------------------------------------------*/
void DelayS(unsigned char Time)
{
	unsigned char a,b;
	for(a=0;a<Time;a++)
	{
		for(b=0;b<10;b++)
		{
		 	DelayMs(100); //
		}
	}
}
/*-------------------------------------------------
 *  函数名:  main 
 *	功能：  主函数
 *  输入：  无
 *  输出：  无
 --------------------------------------------------*/
void main()
{
     unsigned int phy_flag=0;
     unsigned int phy_flag2=0;
    
	POWER_INITIAL();	//系统初始化
    D5=1;
    D4= 1; //
    RESETB=0;
    DelayMs(200); 
    RESETB=1;//复位脚
    DelayMs(200); 
    D5=0;
    D4= 0; //

    SMI_write(0x08,0x1f,0x8000);
    SMI_write(0x02,0x1F,0x0A);//D′2ù×÷
	SMI_write(0x08,0x1f,0x10);//D′2ù×÷
	SMI_write(0x02,0x1F,0xa5);//D′2ù×÷
	SMI_read(0X02,0X1F);//?á2ù×÷
    
    SMI_write(0x08,0x1f,0x8000);//D′2ù×÷
	SMI_write(0x00,0x1f,0x0000);//D′2ù×÷
	SMI_read(0X00,0X04);//?á2ù×÷
	SMI_write(0x00,0x04,0x05E1);//D′2ù×÷
	SMI_read(0X00,0X00);//?á2ù×÷
	SMI_write(0x00,0x00,0x3B00);//D′2ù×÷

	SMI_write(0x08,0x1f,0x8000);//D′2ù×÷
	SMI_write(0x05,0x1f,0x00);//D′2ù×÷
	SMI_read(0X05,0X04);//?á2ù×÷
	SMI_write(0x05,0x04,0x05E1);//D′2ù×÷
	SMI_read(0X05,0X00);//?á2ù×÷
	SMI_write(0x05,0x00,0x3B00);//D′2ù×÷

	SMI_write(0x08,0x1f,0x8000);//D′2ù×÷
	SMI_write(0x06,0x1f,0x00);//D′2ù×÷
	SMI_read(0X06,0X04);//?á2ù×÷
	SMI_write(0x06,0x04,0x05E1);//D′2ù×÷

	SMI_write(0x02,0x1f,0x00);//D′2ù×÷
	SMI_read(0X02,0X1C);//?á2ù×÷
	SMI_write(0x02,0x1C,0x40C6);//D′2ù×÷


	SMI_write(0x08,0x1f,0x8000);//D′2ù×÷
	SMI_write(0x07,0x1F,0x0a);//D′2ù×÷
	SMI_write(0x08,0x1F,0x05);//D′2ù×÷
	SMI_read(0X07,0X10);//?á2ù×÷
	SMI_write(0x07,0x10,0x00);//D′2ù×÷

	SMI_write(0x08,0x1f,0x8000);//D′2ù×÷
	SMI_write(0x07,0x1F,0x00);//D′2ù×÷
	SMI_read(0X07,0X04);//?á2ù×÷
	SMI_write(0x07,0x1F,0x5E1);//D′2ù×÷
	SMI_read(0X07,0X00);//?á2ù×÷
	SMI_write(0x07,0x00,0x3300);//D′2ù×÷
		

	SMI_write(0x08,0x1f,0x8000);//D′2ù×÷
	SMI_write(0x06,0x11,0x0a);//D′2ù×÷
	SMI_write(0x08,0x1f,0x0f);//D′2ù×÷
	SMI_read(0X06,0X10);//?á2ù×÷
	SMI_write(0x06,0x11,0x39a8);//D′2ù×÷

	SMI_write(0x08,0x1f,0x8000);//D′2ù×÷
	SMI_write(0x02,0x1f,0x00);//D′2ù×÷
	SMI_read(0X02,0X1c);//?á2ù×÷
	SMI_write(0x02,0x1c,0x40e6);//D′2ù×÷

	SMI_write(0x08,0x1f,0x8000);//D′2ù×÷
	SMI_write(0x02,0x1f,0x0A);//D′2ù×÷
	SMI_write(0x08,0x1f,0x005);//D′2ù×÷
	SMI_read(0X02,0X10);//?á2ù×÷
	SMI_write(0x02,0x10,0x00);//D′2ù×÷

	SMI_write(0x08,0x1f,0x8000);//D′2ù×÷
	SMI_write(0x02,0x1f,0x00);//D′2ù×÷
	SMI_read(0X02,0X00);//?á2ù×÷
	SMI_write(0x02,0x00,0x2200);//D′2ù×÷
    
	while(1)
	{
           SMI_write(0x08,0x1f,0x8000);//D′2ù×÷	
			phy_flag=SMI_read(0X07,0X01);//?á2ù×÷
	  	    phy_flag=SMI_read(0X07,0X01);//?á2ù×÷
            SMI_write(0x08,0x1f,0x8000);//D′2ù×÷
			phy_flag2=SMI_read(0X02,0X01);//?á2ù×÷
		    phy_flag2=SMI_read(0X02,0X01);//?á2ù×÷
        if(phy_flag==0x782d)
        {
            D5=1;
        }
        else
        {
            D5=0;
        }
         if(phy_flag2==0x780d)
        {
            D4=1;
        }
        else
        {
            D4=0;
        }
		DelayMs(200); 

	}
}