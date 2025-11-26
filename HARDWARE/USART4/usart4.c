#include "usart4.h"
#include "stdarg.h"

//串口发送缓存区 	
uint8 USART4_TX_BUF[256]; 	             //发送缓冲,最大USART3_MAX_SEND_LEN字节	 1024 






PROTOCOL_COMMAND lcd_command;

//RECE_LDATA	Rece_Lcd_Data;

uint8_t CMD_RXFRMOK = 0;

UNION_GG  UnionD;


SLAVE_GG  SlaveG[13];

USlave_Struct JiZu[12];



uint8 cmd_get_time[] ={0x5A, 0xA5, 0x03,0x81,0x20,0x10};//用于在开机时发命令给LCD，同步时间信息

uint8 cmd_get_set_time[] ={0x5A,0XA5,0X04,0X83,0X00,0XB0,0X0B};//用于读取用户设置时间变量
///////////////////////////////////////USART4 printf支持部分//////////////////////////////////
//串口2,u2_printf 函数
//确保一次发送数据不超过USART4_MAX_SEND_LEN字节
////////////////////////////////////////////////////////////////////////////////////////////////
void u4_printf(char* fmt,...)  
{  
  int len=0;
	int cnt=0;
//	unsigned  char i;
//	unsigned  char Frame_Info[5]; //指令长度
	va_list ap;
	va_start(ap,fmt);
	vsprintf((char*)USART4_TX_BUF,fmt,ap);
	va_end(ap);
	len = strlen((const char*)USART4_TX_BUF);
	while(len--)
	  {
	  while(USART_GetFlagStatus(UART4,USART_FLAG_TC)!=1); //等待发送结束
	  USART_SendData(UART4,USART4_TX_BUF[cnt++]);
	  }
}






//串口4发送s个字符

///////////////////////////////////////USART2 初始化配置部分//////////////////////////////////	    
//功能：初始化IO 串口2
//输入参数
//bound:波特率
//输出参数
//无
//////////////////////////////////////////////////////////////////////////////////////////////	  
void uart4_init(u32 bound)
{  	 		 
	//GPIO端口设置
    GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	 
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4, ENABLE);	//使能USART4
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);	//使能GPIOC时钟
	USART_DeInit(UART4);  //复位串口4

     //USART4_TX   PC.10
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10; //PC.10
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//复用推挽输出
    GPIO_Init(GPIOC, &GPIO_InitStructure);
   
    //USART4_RX	  PC.11
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//浮空输入
    GPIO_Init(GPIOC, &GPIO_InitStructure);  

    //Usart4 NVIC 配置
    NVIC_InitStructure.NVIC_IRQChannel = UART4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority= 2;//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	        //根据指定的参数初始化VIC寄存器
  
    //USART4 初始化设置
	USART_InitStructure.USART_BaudRate = bound;   //一般设置为9600;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;  //字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;  //一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;  //无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;  //无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	  //收发模式

    USART_Init(UART4, &USART_InitStructure);   //初始化串口
    USART_ITConfig(UART4, USART_IT_RXNE, ENABLE);  //开启中断
    USART_Cmd(UART4, ENABLE);                      //使能串口 
	
//	USART_DMACmd(USART2,USART_DMAReq_Tx,ENABLE);    //使能串口2的DMA发送
//	UART_DMA_Config(DMA1_Channel7,(u32)&USART2->DR,(u32)USART2_TX_BUF,1000);  //DMA1通道7,外设为串口2,存储器为USART2_TX_BUF,长度1000. 										  	
}



void ModBus_Uart4_Local_Communication(void)
{
	/**************分机接收主机数据并反馈***********************/	
		uint8  i = 0;	
		
		uint16 checksum = 0;

		if(Sys_Admin.Device_Style == 0 || Sys_Admin.Device_Style == 1) //相变机型或多拼机型，跟主机失联情况处理
			{
				if(sys_flag.Address_Number >= 1)
					{
						if(sys_flag.UnTalk_Time > 15) //15秒
							{
								LCD4013X.DLCD.UnionControl_Flag = 0;  //设备在联机状态
								if(sys_data.Data_10H == 2)
									{
										//sys_close_cmd();
										// GetOut_Mannual_Function();
										
									}
								if(sys_data.Data_10H == 3)
									{
										//如果没有分屏在线
										 if(sys_flag.Lcd4013_OnLive_Flag == 0)
										 	{
										 		 sys_data.Data_10H = 0;
										 		 GetOut_Mannual_Function();
										 	}
										 
									}
							
								
							}
					}
			}

		

		

	
		if(U4_Inf.Recive_Ok_Flag)
			{
				U4_Inf.Recive_Ok_Flag = 0;//不能少哦
				 //关闭中断
				USART_ITConfig(UART4, USART_IT_RXNE, DISABLE); 
				 
				checksum  = U4_Inf.RX_Data[U4_Inf.RX_Length - 2] * 256 + U4_Inf.RX_Data[U4_Inf.RX_Length - 1];

				

				
				
			//老板键，对设备序列号进行修改
				if( checksum == ModBusCRC16(U4_Inf.RX_Data,U4_Inf.RX_Length))
					{
						//u4_printf("\n*地址参数= %d\n",Sys_Admin.ModBus_Address);
						
						if(sys_flag.Address_Number == 0)
							{
								//根据屏幕设定地址
								//if(U4_Inf.RX_Data[0] ==Sys_Admin.ModBus_Address)
								//	UART4_Server_Cmd_Analyse();
							}
						else
							{
								//根据主板拨码地址
								if(U4_Inf.RX_Data[0] == sys_flag.Address_Number)
									{
										sys_flag.UnTalk_Time = 0;
										LCD4013X.DLCD.UnionControl_Flag = OK;  //设备在联机状态
										UART4_Server_Cmd_Analyse();
									}
									
							}
						
						
					
					}
					
			 
				
			//对接收缓冲器清零
				for( i = 0; i < 200;i++ )
					U4_Inf.RX_Data[i] = 0x00;
			
			//重新开启中断
				USART_ITConfig(UART4, USART_IT_RXNE, ENABLE); 
				
			}
}



void UART4_Server_Cmd_Analyse(void)
 {
 	uint8 Cmd = 0;
	uint8 index = 0;
	uint8 Length = 0;
	uint16 Value = 0;
	uint16 Address = 0;
	uint16 Data = 0; 
	uint8  Device_ID = Sys_Admin.ModBus_Address; //身份地址，不能错 ************88
	float  value_buffer = 0;
	uint16 check_sum = 0;
	uint16 Data1 = 0;
	uint16 Data2 = 0;
	uint16 Data3 = 0;
	uint16 Data_Address = 0;
	uint16 Buffer_Data16 = 0;
	uint16 Data_Length = 0;

	uint8    Save_Flag1 = 0;
	uint8    Save_Flag2 = 0;
	


	
	Device_ID = sys_flag.Address_Number;
	
	Cmd = U4_Inf.RX_Data[1];
	Address = U4_Inf.RX_Data[2] *256+ U4_Inf.RX_Data[3];
	Length = U4_Inf.RX_Data[4] *256+ U4_Inf.RX_Data[5];
	Value = U4_Inf.RX_Data[4] *256+ U4_Inf.RX_Data[5]; //06格式下， 
	
	
	switch (Cmd)
		{
			case 03:  //00 03 01 F3 00 0F F5 D0		

					switch (Address)
						{
							case 100:
									if( Length == 18)  //这边的数据长度也根据总变化，不是字节长度的变化
										{
											
											U4_Inf.TX_Data[0] = Device_ID;
											U4_Inf.TX_Data[1] = 0x03;// 
											U4_Inf.TX_Data[2] = 36; //数据长度为6， 根据情况改变***
									
											U4_Inf.TX_Data[3] = 0x00;
											U4_Inf.TX_Data[4] = sys_data.Data_10H;// 1当前的运行状态

											U4_Inf.TX_Data[5] = Temperature_Data.Pressure_Value >> 8 ;
											U4_Inf.TX_Data[6] = Temperature_Data.Pressure_Value & 0X00FF;//2 各模块不具备压力变送器
											
											U4_Inf.TX_Data[7] = 0x00;
											U4_Inf.TX_Data[8] = 0;//3当前蒸汽的温度

											U4_Inf.TX_Data[9] = 0x00;
											U4_Inf.TX_Data[10] = LCD10D.DLCD.Water_State;//4当前水位的状态

											
											U4_Inf.TX_Data[11] = 0;
											U4_Inf.TX_Data[12] = sys_data.Data_12H;//5 异常状态反馈给控制


											 
											U4_Inf.TX_Data[13] = sys_flag.Protect_WenDu >> 8;
											U4_Inf.TX_Data[14] = sys_flag.Protect_WenDu & 0x00FF;//6内部烟气的温度

											U4_Inf.TX_Data[15] = 0;
											U4_Inf.TX_Data[16] = sys_flag.Error_Code;//7故障代码

											U4_Inf.TX_Data[17] = 0;
											U4_Inf.TX_Data[18] = sys_flag.flame_state;//8火焰状态 

											U4_Inf.TX_Data[19] = sys_flag.Fan_Rpm >> 8;
											U4_Inf.TX_Data[20] = sys_flag.Fan_Rpm & 0x00FF;//9风机转速

											U4_Inf.TX_Data[21] = Temperature_Data.Inside_High_Pressure >> 8;
											U4_Inf.TX_Data[22] = Temperature_Data.Inside_High_Pressure & 0x00FF;      //10 相变机组内部压力

 											U4_Inf.TX_Data[23] = 0;
 											U4_Inf.TX_Data[24] = sys_data.Data_1FH ;//11风机功率 

											U4_Inf.TX_Data[25] = 0;
											U4_Inf.TX_Data[26] = Switch_Inf.air_on_flag; //12风机运行状态


											U4_Inf.TX_Data[27] = 0;
											U4_Inf.TX_Data[28] = Switch_Inf.Water_Valve_Flag;//13进水阀

											U4_Inf.TX_Data[29] = 0;
											U4_Inf.TX_Data[30] = Switch_Inf.pai_wu_flag;//14排污阀状态

											U4_Inf.TX_Data[31] = 0;
											U4_Inf.TX_Data[32] = Switch_Inf.LianXu_PaiWu_flag;//15连续排污阀状态
											
											
											U4_Inf.TX_Data[33] = 0;
											U4_Inf.TX_Data[34] = 0;//16  未使用
											
											U4_Inf.TX_Data[35] = sys_flag.TimeDianHuo_Flag;
											U4_Inf.TX_Data[36] = sys_flag.Paiwu_Flag; //17  自动排污已经开启标志， 

											U4_Inf.TX_Data[37] = sys_flag.Inputs_State >> 8;
											U4_Inf.TX_Data[38] = sys_flag.Inputs_State & 0x00FF;//18 

											
											
										
										 
											check_sum  = ModBusCRC16(U4_Inf.TX_Data,41);   //这个根据总字节数改变
											U4_Inf.TX_Data[39]  = check_sum >> 8 ;
											U4_Inf.TX_Data[40]  = check_sum & 0x00FF;
											
											Usart_SendStr_length(UART4,U4_Inf.TX_Data,41);
											
										}

									break;
						
							
							
								

							default :
									break;
						}
					

					//针对压力做浮点数转换
					
					break;
			
			case 0x10:   //多个寄存器写入
						
						Data_Address = U4_Inf.RX_Data[2] * 256 + U4_Inf.RX_Data[3];
						Data_Length = U4_Inf.RX_Data[4] * 256 + U4_Inf.RX_Data[5];
						Buffer_Data16 = U4_Inf.RX_Data[7] *256 + U4_Inf.RX_Data[8];  //高低字节的顺序颠倒
						
						switch (Data_Address)
							{
							case 0xC8:   //处理多个参数的写入问题
										// u1_printf("\n* 收到数据 = %d\n",Buffer_Data16);

										//1、指令模式，启动，停止，手动
										switch (Buffer_Data16)
											{
											case 0:
													if(sys_data.Data_10H == 2)
														{
															sys_close_cmd();
														}
													if(sys_data.Data_10H == 3)
														{
															//退出手动模式
															sys_data.Data_10H = 0;
															GetOut_Mannual_Function();
														}

													break;
											case 1: //启动命令
													if(sys_data.Data_10H == 0)
														{
															sys_start_cmd();
														}

													break;
											case 3://手动模式
													//只能在待机情况下进入手动
													if(sys_data.Data_10H == 0)
														{
															GetOut_Mannual_Function();
															sys_data.Data_10H = 3;
														}

													break;
											default:
													break;
											}
										

										Data1 = U4_Inf.RX_Data[9]*256 + U4_Inf.RX_Data[10];  //故障复位指令
										if(Data1 == OK)
											{
												sys_flag.Error_Code = 0; //故障复位
												
											}

									
										Data1 = U4_Inf.RX_Data[15]*256 + U4_Inf.RX_Data[16];  //5 继电器16路控制，按位获取
										if(sys_data.Data_10H == 3)
											{
												//在手动模式下，根据标志位开相应的继电器
												
												if(Data1 & 0x0001)   //风机标志位
													{
														
														Send_Air_Open();
														
													}
												else
													{
														Send_Air_Close();
														 
													}

												if(Data1 & 0x0002)   //水泵，补水阀标志位
													{
														Feed_Main_Pump_ON();
														Second_Water_Valve_Open();
													}
												else
													{
														 Feed_Main_Pump_OFF();
														 Second_Water_Valve_Close();
													}

												if(Data1 & 0x0004)   //排水阀
													{
														Pai_Wu_Door_Open();
													}
												else
													{
														Pai_Wu_Door_Close();
													}
												if(Data1 & 0x0008)   //明火阀
													{
														WTS_Gas_One_Open();
													}
												else
													{
														WTS_Gas_One_Close();
													}

												
												
											}

										
										//风机的功率传输，分设备状态
										if(sys_data.Data_10H  ==  3)
											{
												if(U4_Inf.RX_Data[14] <= 100)
													{
														PWM_Adjust(U4_Inf.RX_Data[14]);
													}
											}

										if(sys_data.Data_10H == 2)
											{
												//在运行中，且有火焰
												if(sys_flag.flame_state == FLAME_OK)
													{
														if(U4_Inf.RX_Data[14] <= 100)
															{
																sys_flag.AirPower_Need = U4_Inf.RX_Data[14];
															}
														else
															{
																sys_flag.AirPower_Need = 100;
															}
													}
												else
													{
														sys_flag.AirPower_Need = 0;
													}
											}

										//在待机过程中，风机需要保持一定功率运行的启动信号
										sys_flag.Idle_AirWork_Flag = U4_Inf.RX_Data[18];
										
										if(sys_flag.Paiwu_Flag == 0)
											{
												//防止，已经接收到指令，几秒种后又被清除
												sys_flag.Paiwu_Flag = U4_Inf.RX_Data[20];
											}
										
										 
										 if(sys_config_data.zhuan_huan_temperture_value != U4_Inf.RX_Data[22])
										 	{
										 		sys_config_data.zhuan_huan_temperture_value = U4_Inf.RX_Data[22];
												Save_Flag1 = OK;
										 	}
										
										if(sys_config_data.Auto_stop_pressure != U4_Inf.RX_Data[24])
											{
												sys_config_data.Auto_stop_pressure = U4_Inf.RX_Data[24];
												Save_Flag1 = OK;
											}

										if(sys_config_data.Auto_start_pressure != U4_Inf.RX_Data[26])
											{
												sys_config_data.Auto_start_pressure = U4_Inf.RX_Data[26];
												Save_Flag1 = OK;
											}

										if(Sys_Admin.DeviceMaxPressureSet != U4_Inf.RX_Data[28])
											{
												if(Sys_Admin.DeviceMaxPressureSet >= 80)
													{
														Sys_Admin.DeviceMaxPressureSet = U4_Inf.RX_Data[28];
														Save_Flag2 = OK;
													}
												
												
											}

										 
										if(Sys_Admin.Dian_Huo_Power != U4_Inf.RX_Data[30])
											{
												if(U4_Inf.RX_Data[30] >= 30 && U4_Inf.RX_Data[30] <= 60)
													{
														Sys_Admin.Dian_Huo_Power = U4_Inf.RX_Data[30];
														Save_Flag2 = OK;
													}
											}

										if(Sys_Admin.Max_Work_Power != U4_Inf.RX_Data[32])
											{
												if(U4_Inf.RX_Data[32] >= 30 && U4_Inf.RX_Data[32] <= 100)
													{
														Sys_Admin.Max_Work_Power = U4_Inf.RX_Data[32];
														Save_Flag2 = OK;
													}
											}
		

										
										Data1 = U4_Inf.RX_Data[33]*256 + U4_Inf.RX_Data[34];  //本体温度的保护值
										if(Sys_Admin.Inside_WenDu_ProtectValue != Data1)
											{
												if(Data1 > 240 && Data1 <= 350 )
													{
														Sys_Admin.Inside_WenDu_ProtectValue = Data1;
														Save_Flag2 = OK;
													}
											}


										sys_flag.Union_DianHuo_Flag = U4_Inf.RX_Data[36];

										

										if(Save_Flag1)
											{
												Write_Internal_Flash();
												BEEP_TIME(100); 
											}

										if(Save_Flag2)
											{
												Write_Admin_Flash();
												BEEP_TIME(100); 
											}
										ModuBus4_Write0x10Response(Device_ID,Data_Address,Data_Length);
										break;
							default:

									break;
							}

						break;

			


			default:
					//无效指令
					break;
		}
 }


uint8 ModuBus4RTU_WriteResponse(uint16 address,uint16 Data16)
{
	uint16 check_sum = 0;
	
	U4_Inf.TX_Data[0] = U4_Inf.RX_Data[0];
	U4_Inf.TX_Data[1]= 0x06;

	
	U4_Inf.TX_Data[2] = address >> 8;    // 地址高字节
	U4_Inf.TX_Data[3] = address & 0x00FF;  //地址低字节
	

	U4_Inf.TX_Data[4] = Data16 >> 8;  //数据高字节
	U4_Inf.TX_Data[5] = Data16 & 0x00FF;   //数据低字节

	check_sum  = ModBusCRC16(U4_Inf.TX_Data,8);   //这个根据总字节数改变
	U4_Inf.TX_Data[6]  = check_sum >> 8 ;
	U4_Inf.TX_Data[7]  = check_sum & 0x00FF;

	Usart_SendStr_length(UART4,U4_Inf.TX_Data,8);

	return 0;
}







uint8 ModBus4_RTU_Read03(uint8 Target_Address,uint16 Data_Address,uint8 Data_Length )
{
		uint16 Check_Sum = 0;
		U4_Inf.TX_Data[0] = Target_Address;
		U4_Inf.TX_Data[1] = 0x03;

		U4_Inf.TX_Data[2]= Data_Address >> 8;
		U4_Inf.TX_Data[3]= Data_Address & 0x00FF;

		
		U4_Inf.TX_Data[4]= Data_Length >> 8;
		U4_Inf.TX_Data[5]= Data_Length & 0x00FF;

		
		Check_Sum = ModBusCRC16(U4_Inf.TX_Data,8);
		U4_Inf.TX_Data[6]= Check_Sum >> 8;
		U4_Inf.TX_Data[7]= Check_Sum & 0x00FF;
		
		Usart_SendStr_length(UART4,U4_Inf.TX_Data,8);


	return 0;
}

uint8 ModBus4_RTU_Write06(uint8 Target_Address,uint16 Data_Address,uint16 Data16)
{
		uint16 Check_Sum = 0;
		U4_Inf.TX_Data[0] = Target_Address;
		U4_Inf.TX_Data[1] = 0x06;

		U4_Inf.TX_Data[2]= Data_Address >> 8;
		U4_Inf.TX_Data[3]= Data_Address & 0x00FF;

		U4_Inf.TX_Data[4]= Data16 >> 8;
		U4_Inf.TX_Data[5]= Data16 & 0x00FF;

		Check_Sum = ModBusCRC16(U4_Inf.TX_Data,8);
		U4_Inf.TX_Data[6]= Check_Sum >> 8;
		U4_Inf.TX_Data[7]= Check_Sum & 0x00FF;

		Usart_SendStr_length(UART4,U4_Inf.TX_Data,8);

		

		return 0;
}

uint8 ModuBus4_Write0x10Response(uint8 Target_Address,uint16 Data_address,uint16 Data16)
{
	uint16 check_sum = 0;
	
	U4_Inf.TX_Data[0] = Target_Address;
	U4_Inf.TX_Data[1]= 0x10;

	
	U4_Inf.TX_Data[2] = Data_address >> 8;    // 地址高字节
	U4_Inf.TX_Data[3] = Data_address & 0x00FF;  //地址低字节
	

	U4_Inf.TX_Data[4] = Data16 >> 8;  //数据高字节
	U4_Inf.TX_Data[5] = Data16 & 0x00ff;   //数据低字节

	check_sum  = ModBusCRC16(U4_Inf.TX_Data,8);   //这个根据总字节数改变
	U4_Inf.TX_Data[6]  = check_sum >> 8 ;
	U4_Inf.TX_Data[7]  = check_sum & 0x00FF;

	Usart_SendStr_length(UART4,U4_Inf.TX_Data,8);

	return 0;
}




void Union_ModBus_Uart4_Local_Communication(void)
{
		
		uint8  i = 0;	
		uint8  Target_Address = 0;
		
		uint16 checksum = 0;

		for(i = 1; i < 13; i++)
			{
				if(SlaveG[i].Send_Flag > 20)//发送6次未回应，则失联
					{
						SlaveG[i].Alive_Flag = FALSE;
						JiZu[i].Slave_D.Device_State = 0;   //一会移植到外部去
						memset(&JiZu[i].Datas,0,sizeof(JiZu[i].Datas)); //然后对数据清零
					}
			}
		
		
		if(U4_Inf.Recive_Ok_Flag)
			{
				U4_Inf.Recive_Ok_Flag = 0;//不能少哦
				 //关闭中断
				USART_ITConfig(UART4, USART_IT_RXNE, DISABLE); 
				 
				checksum  = U4_Inf.RX_Data[U4_Inf.RX_Length - 2] * 256 + U4_Inf.RX_Data[U4_Inf.RX_Length - 1];
				
			
				if(checksum == ModBusCRC16(U4_Inf.RX_Data,U4_Inf.RX_Length))
					{
						Target_Address = U4_Inf.RX_Data[0];

						/**********************检查锁机运行的数量***************************************/
						if(AUnionD.OFFlive_Numbers)
							{
								//当有禁止设备的数量时，小于禁止数量，则不通信
								if(Target_Address > AUnionD.OFFlive_Numbers)
									{
										Modbus4_UnionRx_DataProcess(U4_Inf.RX_Data[1],Target_Address);
									}
							}
						else
							{
								Modbus4_UnionRx_DataProcess(U4_Inf.RX_Data[1],Target_Address);
							}
						

					
					}
					
			 
				
			//对接收缓冲器清零
				for( i = 0; i < 200;i++ )
					U4_Inf.RX_Data[i] = 0x00;
			
			//重新开启中断
				USART_ITConfig(UART4, USART_IT_RXNE, ENABLE); 
				
			}
}


uint8 Union_Modbus4_UnionTx_Communication(void)
{
	static uint8 Address = 1;
	uint8 Max_Adress = 0;
	//	SlaveG[1].Alive_Flag  
	//	JiZu[1].Slave_D.Device_State  //这用于跟屏数据的同步
	if(AUnionD.Max_Address == 0)
		{
			Max_Adress = 4;
		}

	if(AUnionD.Max_Address >= 1)
		{
			Max_Adress = 5;
		}
	
		
		
		switch (Address)
			{
			case 1:
					//UnionD.OFFlive_Numbers
					if(JiZu_ReadAndWrite_Function(Address))
						{
							Address++;
						}
						
					
					break;
			case 2:
				
					if(JiZu_ReadAndWrite_Function(Address))
						Address++;
					
					break;
			case 3:
				
					if(JiZu_ReadAndWrite_Function(Address))
						Address++;
					
					break;
			case 4:
				
					if(JiZu_ReadAndWrite_Function(Address))
						{
							Address++;
							

							
						}
						
					
					break;
			case 5:
				
					if(JiZu_ReadAndWrite_Function(Address))
						{
							Address = 1;
						}
						 
					
					break;
			case 6:
				
					if(JiZu_ReadAndWrite_Function(Address))
						{
							Address++;
							if(Address > Max_Adress)
								{
									Address = 1;
								}
						}
						
					
					break;
			case 7:
				
					if(JiZu_ReadAndWrite_Function(Address))
						Address++;
					
					break;
			case 8:
				
					if(JiZu_ReadAndWrite_Function(Address))
						{
							Address++;
							if(Address > Max_Adress)
								{
									Address = 1;
								}
						}
						 
					
					break;
			case 9:
				
					if(JiZu_ReadAndWrite_Function(Address))
						Address++;
					
					break;
			case 10:
				
					if(JiZu_ReadAndWrite_Function(Address))
						{
							Address = 1;
						}
						
					
					break;

			

			default :
					Address = 1;
					break;
			}
	
		

		return 0;
}



uint8 Modbus4_UnionRx_DataProcess(uint8 Cmd,uint8 address)
{
		uint8 Data_Length = 0;
		uint16 Data_Address = 0;
		uint16 Buffer_Data = 0;
		float  Buffer_Float = 0;
	//接收到从机的数据进行处理
		
	SlaveG[address].Send_Flag = 0;  //清楚发送标志
	SlaveG[address].Alive_Flag = OK;
	if(Cmd == 0x03)
		{
			Data_Length = U4_Inf.RX_Data[2];

			if(Data_Length == 36)  //一次读取18个数据
				{
					if(SlaveG[address].Startclose_Sendflag == 0) //解决命令下发和实际不一致，等发送次数完成
						{
							//if(U4_Inf.RX_Data[4]  == 2)   //启动或待机的标志
							//	JiZu[address].Slave_D.StartFlag = OK;
							//else
							//	JiZu[address].Slave_D.StartFlag = 0;  
						}
					

					if(SlaveG[address].Send_Flag > 20)//发送6次未回应，则失联
						{
							JiZu[address].Slave_D.Device_State = 0;   //一会移植到外部去
						}
					else
						{
							if(U4_Inf.RX_Data[4]  == 2)
								JiZu[address].Slave_D.Device_State = 2;  //四种状态的切换
							if(U4_Inf.RX_Data[4]  == 0 )
								JiZu[address].Slave_D.Device_State = 1;  //待机模式
							if(U4_Inf.RX_Data[16])  //故障显示
								JiZu[address].Slave_D.Device_State = 3; //故障状态
							if(U4_Inf.RX_Data[4]  == 3)
								JiZu[address].Slave_D.Device_State = 1;//手动状态
						}

					
					Buffer_Float = U4_Inf.RX_Data[5] *255 + U4_Inf.RX_Data[6];
				 
					 
				 
					
					JiZu[address].Slave_D.Dpressure = Buffer_Float / 100;
					 
					

					JiZu[address].Slave_D.Water_State = U4_Inf.RX_Data[10] ;
						
					
					JiZu[address].Slave_D.Inside_WenDu = U4_Inf.RX_Data[13] * 256  + U4_Inf.RX_Data[14]; 

					 
					JiZu[address].Slave_D.Error_Code = U4_Inf.RX_Data[16] ;//获取故障代码
					
					
					if(JiZu[address].Slave_D.Error_Code == 0)
						{
							JiZu[address].Slave_D.Error_Reset = 0;  //没有故障时，取消复位命令
						}

					

					JiZu[address].Slave_D.Flame = U4_Inf.RX_Data[18] ;

					JiZu[address].Slave_D.Fan_Rpm = U4_Inf.RX_Data[19] * 256  + U4_Inf.RX_Data[20];  //获取风机的转速


						
					
					Buffer_Float = U4_Inf.RX_Data[21] * 256 + U4_Inf.RX_Data[22];
					//JiZu[address].Slave_D.Dpressure = Buffer_Float / 100;  //相变高压侧

					 
					JiZu[address].Slave_D.Power = U4_Inf.RX_Data[24] ;//9风机功率

					JiZu[address].Slave_D.Pump_State = U4_Inf.RX_Data[28] ;
					

					SlaveG[address].TimeDianHuo_Flag = U4_Inf.RX_Data[35];

					

					if(SlaveG[address].Paiwu_Flag)
						{
							if(U4_Inf.RX_Data[36]) //各机组反馈的是否在排污的标志
								{
									SlaveG[address].Paiwu_Flag = 0; //则不再发送要进行排污的指令
								}
						}
					
				 
					
				}

		
		}

	if(Cmd == 0x10)
		{
				Data_Address = U4_Inf.RX_Data[2]*256 + U4_Inf.RX_Data[3] ;
				if(Data_Address == 0x00C8)
					{
						SlaveG[address].Command_SendFlag = 0; //将相应的标志位取消
					}

				
		}
		


		return 0;
}



uint8 ModBus4RTU_Write0x10Function(uint8 Target_Address,uint16 Data_Address,uint16 Length)
{
	//01  10  00 A4	00 02  04  00 0F 3D DD 18 EE 写32位数据指令格式
	//响应： 01 10 00 A4	  00 02 crc
		uint16 Check_Sum = 0;
		U4_Inf.TX_Data[0] = Target_Address;
		U4_Inf.TX_Data[1] = 0x10;

		U4_Inf.TX_Data[2]= Data_Address >> 8;
		U4_Inf.TX_Data[3]= Data_Address & 0x00FF;

		U4_Inf.TX_Data[4]= Length >> 8;
		U4_Inf.TX_Data[5]= Length & 0x00FF;

		U4_Inf.TX_Data[6]= Length * 2;  //总字节数

		U4_Inf.TX_Data[7]= 0;
		U4_Inf.TX_Data[8]= JiZu[Target_Address].Slave_D.StartFlag;  //1启动或关闭命令

		U4_Inf.TX_Data[9]= 0;
		U4_Inf.TX_Data[10]= JiZu[Target_Address].Slave_D.Error_Reset;  //2故障复位命令

		U4_Inf.TX_Data[11]= 0;
		U4_Inf.TX_Data[12]= AUnionD.Devive_Style;  // 3设备类型 

		
		U4_Inf.TX_Data[13]= 0;
		U4_Inf.TX_Data[14]= SlaveG[Target_Address].Out_Power;	//4 设备需要运行的功率

		U4_Inf.TX_Data[15]= JiZu[Target_Address].Slave_D.Realys_Out >> 8;
		U4_Inf.TX_Data[16]= JiZu[Target_Address].Slave_D.Realys_Out & 0x00FF;	//5 继电器16路控制，按位获取

		U4_Inf.TX_Data[17]= 0;
		U4_Inf.TX_Data[18]= SlaveG[Target_Address].Idle_AirWork_Flag;	//6 风机待机启动信号

		U4_Inf.TX_Data[19]= 0;
		U4_Inf.TX_Data[20]= SlaveG[Target_Address].Paiwu_Flag;	//7 自动排污控制命令

		U4_Inf.TX_Data[21]= 0;
		U4_Inf.TX_Data[22]= sys_config_data.zhuan_huan_temperture_value;	//8 目标压力

		U4_Inf.TX_Data[23]= 0;
		U4_Inf.TX_Data[24]= sys_config_data.Auto_stop_pressure;	//9 停机压力

		U4_Inf.TX_Data[25]= 0;
		U4_Inf.TX_Data[26]= sys_config_data.Auto_start_pressure;	//10 启动压力

		U4_Inf.TX_Data[27]= 0;
		U4_Inf.TX_Data[28]= Sys_Admin.DeviceMaxPressureSet;	//11额定压力

		U4_Inf.TX_Data[29]= 0;
		U4_Inf.TX_Data[30]= JiZu[Target_Address].Slave_D.DianHuo_Value;	//12 点火功率

		U4_Inf.TX_Data[31]= 0;
		U4_Inf.TX_Data[32]= JiZu[Target_Address].Slave_D.Max_Power;	//13 最大运行功率

		U4_Inf.TX_Data[33]= JiZu[Target_Address].Slave_D.Inside_WenDu_Protect >> 8;
		U4_Inf.TX_Data[34]= JiZu[Target_Address].Slave_D.Inside_WenDu_Protect & 0x00FF;	//14 炉温保护值

		U4_Inf.TX_Data[35]= 0;
		U4_Inf.TX_Data[36]= sys_flag.DianHuo_Alive;	//15 预留参数2    控制点火过程中，在运行时，需要控制最大功率

		U4_Inf.TX_Data[37]= 0;
		U4_Inf.TX_Data[38]= 0;	//16 预留参数3

		U4_Inf.TX_Data[39]= 0;
		U4_Inf.TX_Data[40]= 0;	//17 预留参数4

		U4_Inf.TX_Data[41]= 0;
		U4_Inf.TX_Data[42]= 0;	//18 预留参数5
		
		Check_Sum = ModBusCRC16(U4_Inf.TX_Data,45);
		U4_Inf.TX_Data[43]= Check_Sum >> 8;
		U4_Inf.TX_Data[44]= Check_Sum & 0x00FF;

		Usart_SendStr_length(UART4,U4_Inf.TX_Data,45);

			

		return 0;
}


uint8 JiZu_ReadAndWrite_Function(uint8 address)
{
		
		static uint8 index  = 0;
		uint8 return_value = 0;
		static uint8 Write_Index = 0; //用于切换不同的命令
		
		switch (index)
			{
			case 0:
					if(SlaveG[address].Alive_Flag == 0)
						{
							//如果设备不在线，则只保持读的状态
							SlaveG[address].Send_Index = 0;
						}
					
					if(SlaveG[address].Send_Index)
						{
							//SlaveG[address].Command_SendFlag--; //对发送次数递减
							
							SlaveG[address].Send_Index = 0;
							ModBus4RTU_Write0x10Function(address,200,18);//18个数据一次性写入
						}
					else
						{
							SlaveG[address].Send_Index  = OK;
							ModBus4_RTU_Read03(address,100,18); //这个是默认读取数据的指令
							
						}
									
					SlaveG[address].Send_Flag ++; //对发送进行计数
					U4_Inf.Flag_100ms = 0;
					
					index++;

					break;
			case 1:
					if(U4_Inf.Flag_100ms)
						{
							U4_Inf.Flag_100ms = 0;
							
							index = 0;
							return_value = OK;
							
						}

					break;
			default:
					index = 0;
					break;
			}


		return return_value;
}






