#include "main.h"
#ifndef __USART4_H
#define __USART4_H	  	     
 
#define USART4_MAX_RECV_LEN		255				//最大接收缓存字节数
#define USART4_MAX_SEND_LEN		255				//最大发送缓存字节数

#define    Sys_Begin_Check_Page  110  //开机启动跟屏通信检查页面，否则停留在该页面
#define    Sys_Main_Page    1    //系统主界面
#define    Sys_Start_Confirm_Page  10 //系统启动前确认界面
#define    Sys_Time_Set_Page  6  //系统时间修改界面
#define    Sys_Warning_Page  3  //系统故障提示界面
#define    Sys_Admin_SetPage  11  //管理员界面



#define    Sys_Manual_Page    5    //手动测试界面
#define    Sys_Air_Blow_Warning_Page    111    //系统后吹扫提示界面



#define  LAdmin_Main_Page 11 //管理员界面
#define  LAdmin_Sys_Set_Page 4 //系统设置界面  **
  




//动画变量地址
#define  Movie_Start  1
#define  Movie_Close  0

#define   Air_Movie_Address  0x0210
#define   Pump_Movie_Address 0x0212
#define   Flame_Movie_Address 0x0214






#define FRM_H 0x5A
#define FRM_L 0xA5

typedef struct _CMD
{
	uint8_t header_h;
	uint8_t header_l;
	uint8_t data_len;
	uint8_t cmd;
	uint8_t data[32];
}PROTOCOL_COMMAND;


typedef struct _CMD_UNIS
{
	uint16  UnionStartFlag;
	uint16  Need_Numbers;
	uint16  Same_ValueFlag; //同步参数设置标志
	uint16  Target_Value;
	uint16   Stop_Value;
	
	uint16  Start_Value;
	uint16  Big_Pressure;
	uint16  Data2;
	uint16  Data3;
	uint16  Data4;
	
	
}UNION_GG;


typedef struct _SLAVE_G
{
	uint8 Dev_State;
volatile	uint8 Alive_Flag;   //处于连接状态的标志
volatile	uint8 Ok_Flag;  //存在切无故障标志
volatile	uint16 Send_Flag;   //主机发送标志
volatile	uint8 UnBack_time;  //未返回通信时间


			uint8 Idle_AirWork_Flag; //用于待机风机启动，以一定的功率长时间吹扫

	uint8   ForceSend_Count;  //用于命令连发的次数

			uint8 Key_Power;  //使用权限

volatile	uint8 UnSuccessTalk;

			uint8 Error_Code;
			uint8 Lock_Error;

			uint8 Send_Count;


			

			uint8 Startclose_Sendflag;
			uint8 Startclose_Data;  //双双配合使用

			
			uint8 Command_SendFlag;
			uint8 Send_Index;
			uint8 Out_Power;

			

			uint8 ResetError_Flag;
			uint8 ResetError_Data;

			uint8 AirOpen_Flag;
			uint8 AirOpen_Data;

			uint8 PumpOpen_Flag;
			uint8 PumpOpen_Data;

			uint8 PaiWuOpen_Flag;
			uint8 PaiWuOpen_Data;    

			uint8 LianxuFa_Flag;
			uint8 LianxuFa_Data;

			uint8 AirPower_Flag;
			uint8 AirPower_Data;

			uint8 Paiwu_Flag;
			uint8 Paiwu_Data;

			uint8 TimeDianHuo_Flag;
			
			uint8 ResetError_SendFlag;

			uint16 Big_time;
			uint16 Small_time;
			uint16 Zero_time;

			uint32 Work_Time;




			uint8 First_Blow;
			uint8 Last_Blow;
			uint8 Fire_Power;
			uint8 Max_Power;
			uint8 Smoke_Protect;//冷凝后的烟气温度

			uint16 Inside_WenDu_ProtectValue;
	
}SLAVE_GG;//从机的控制命令台


typedef struct _CCMD_UNIS
{
	uint16  Flame; //同步参数设置标志
	uint16  Error_Code;  //故障代码
	uint16  Power;   //风机功率
	uint16  UnionOn_Flag;	//联控标志
	uint16  StartFlag; //设备运行标志
	
	uint16  Error_Reset;	//复位指令	
	uint16  Device_State; //设备状态（四状态，未连接，待机，运行，故障）
	uint16  Pump_State;//水泵或进水阀状态
	uint16  Water_State;  //液位状态
	uint16  PaiwuFa_State;  //排污阀

	float   Dpressure;  //相变高压侧压力

	uint16  Fan_Rpm;  //风机转速
	uint16  Inside_WenDu;//本体温度
	uint16  Realys_Out; //16路继电器输出，用在手动模式下
	uint16  DianHuo_Value;  //点火功率
	uint16  Max_Power; //最大运行功率                 //总字节数34

	uint16  Inside_WenDu_Protect;   
	uint16  Data7;//总字节数38
	     

	
}Slave_GG;




typedef union  _SLAVE_MsESS
{  //
	uint8 Datas[38];
	
	Slave_GG Slave_D;
	
}USlave_Struct;




extern PROTOCOL_COMMAND lcd_command;

extern UNION_GG  UnionD;


extern SLAVE_GG  SlaveG[13];
extern USlave_Struct JiZu[12];





///////////////////////////////////////USART2 printf支持部分//////////////////////////////////
//串口2,u2_printf 函数
//确保一次发送数据不超过USART2_MAX_SEND_LEN字节
////////////////////////////////////////////////////////////////////////////////////////////////
void u4_printf(char* fmt, ...);
///////////////////////////////////////USART2 初始化配置部分//////////////////////////////////	    
//功能：初始化IO 串口2
//输入参数
//bound:波特率
//输出参数
//无
//////////////////////////////////////////////////////////////////////////////////////////////
void uart4_init(u32 bound);	

void ModBus_Uart4_Local_Communication(void);


uint8 Union_Modbus4_UnionTx_Communication(void);
void Union_ModBus_Uart4_Local_Communication(void);
uint8 Modbus4_UnionRx_DataProcess(uint8 Cmd,uint8 address);


void UART4_Server_Cmd_Analyse(void);


uint8 ModuBus4RTU_WriteResponse(uint16 address,uint16 Data16);

uint8 ModuBus4_Write0x10Response(uint8 Target_Address,uint16 Data_address,uint16 Data16);


uint8 ModBus4RTU_Write0x10Function(uint8 Target_Address,uint16 Data_Address,uint16 Length);


uint8 JiZu_ReadAndWrite_Function(uint8 address);


#endif

