#include "usart4.h"
#include "stdarg.h"
#include "uart_driver.h"

//���ڷ��ͻ����� 	
uint8 USART4_TX_BUF[256]; 	             //���ͻ���,���USART3_MAX_SEND_LEN�ֽ�	 1024 






PROTOCOL_COMMAND lcd_command;

//RECE_LDATA	Rece_Lcd_Data;

uint8_t CMD_RXFRMOK = 0;

UNION_GG  UnionD;


SLAVE_GG  SlaveG[13];

USlave_Struct JiZu[12];



uint8 cmd_get_time[] ={0x5A, 0xA5, 0x03,0x81,0x20,0x10};//�����ڿ���ʱ�������LCD��ͬ��ʱ����Ϣ

uint8 cmd_get_set_time[] ={0x5A,0XA5,0X04,0X83,0X00,0XB0,0X0B};//���ڶ�ȡ�û�����ʱ�����
///////////////////////////////////////USART4 printf֧�ֲ���//////////////////////////////////
//����2,u2_printf ����
//ȷ��һ�η������ݲ�����USART4_MAX_SEND_LEN�ֽ�
////////////////////////////////////////////////////////////////////////////////////////////////
void u4_printf(char* fmt,...)  
{  
  int len=0;
	int cnt=0;
//	unsigned  char i;
//	unsigned  char Frame_Info[5]; //ָ���
	va_list ap;
	va_start(ap,fmt);
	vsprintf((char*)USART4_TX_BUF,fmt,ap);
	va_end(ap);
	len = strlen((const char*)USART4_TX_BUF);
	while(len--)
	  {
	  while(USART_GetFlagStatus(UART4,USART_FLAG_TC)!=1); //�ȴ����ͽ���
	  USART_SendData(UART4,USART4_TX_BUF[cnt++]);
	  }
}






//����4����s���ַ�

///////////////////////////////////////USART2 ��ʼ�����ò���//////////////////////////////////	    
//���ܣ���ʼ��IO ����2
//�������
//bound:������
//�������
//��
//////////////////////////////////////////////////////////////////////////////////////////////	  
void uart4_init(u32 bound)
{  	 		 
	//GPIO�˿�����
    GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	 
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4, ENABLE);	//ʹ��USART4
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);	//ʹ��GPIOCʱ��
	USART_DeInit(UART4);  //��λ����4

     //USART4_TX   PC.10
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10; //PC.10
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//�����������
    GPIO_Init(GPIOC, &GPIO_InitStructure);
   
    //USART4_RX	  PC.11
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//��������
    GPIO_Init(GPIOC, &GPIO_InitStructure);  

    //Usart4 NVIC ����
    NVIC_InitStructure.NVIC_IRQChannel = UART4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority= 2;//��ռ���ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;		//�����ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	        //����ָ���Ĳ�����ʼ��VIC�Ĵ���
  
    //USART4 ��ʼ������
	USART_InitStructure.USART_BaudRate = bound;   //һ������Ϊ9600;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;  //�ֳ�Ϊ8λ���ݸ�ʽ
	USART_InitStructure.USART_StopBits = USART_StopBits_1;  //һ��ֹͣλ
	USART_InitStructure.USART_Parity = USART_Parity_No;  //����żУ��λ
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;  //��Ӳ������������
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	  //�շ�ģʽ

    USART_Init(UART4, &USART_InitStructure);   //��ʼ������
    USART_ITConfig(UART4, USART_IT_RXNE, ENABLE);  //�����ж�
    USART_Cmd(UART4, ENABLE);                      //ʹ�ܴ��� 
	
//	USART_DMACmd(USART2,USART_DMAReq_Tx,ENABLE);    //ʹ�ܴ���2��DMA����
//	UART_DMA_Config(DMA1_Channel7,(u32)&USART2->DR,(u32)USART2_TX_BUF,1000);  //DMA1ͨ��7,����Ϊ����2,�洢��ΪUSART2_TX_BUF,����1000. 										  	
}



void ModBus_Uart4_Local_Communication(void)
{
	/* 从机本地数据处理 - DMA+IDLE模式 */	
		uint8  i = 0;	
		
		uint16 checksum = 0;
		uint8_t* rxData;
		uint16_t rxLen;

		if(Sys_Admin.Device_Style == 0 || Sys_Admin.Device_Style == 1)
			{
				if(sys_flag.Address_Number >= 1)
					{
						if(sys_flag.UnTalk_Time > 15) //15秒
							{
								LCD4013X.DLCD.UnionControl_Flag = 0;  //设备联控状态
								if(sys_data.Data_10H == 2)
									{
										//sys_close_cmd();
										// GetOut_Mannual_Function();
										
									}
								if(sys_data.Data_10H == 3)
									{
										//如果没有分机连接
										 if(sys_flag.Lcd4013_OnLive_Flag == 0)
										 	{
										 		 sys_data.Data_10H = 0;
										 		 GetOut_Mannual_Function();
										 	}
										 
									}
							
								
							}
					}
			}

		/* DMA接收: 检查接收完成标志 */
		if (uartIsRxReady(&uartUnionHandle))
		{
			if (uartGetRxData(&uartUnionHandle, &rxData, &rxLen) == UART_OK)
			{
				/* 复制数据到U4_Inf以兼容后续代码 */
				for (i = 0; i < rxLen && i < 250; i++)
					U4_Inf.RX_Data[i] = rxData[i];
				U4_Inf.RX_Length = rxLen;
				U4_Inf.Recive_Ok_Flag = 1;
			}
		}

	
		if(U4_Inf.Recive_Ok_Flag)
			{
				U4_Inf.Recive_Ok_Flag = 0;//������Ŷ
				 //�ر��ж�
				USART_ITConfig(UART4, USART_IT_RXNE, DISABLE); 
				 
				checksum  = U4_Inf.RX_Data[U4_Inf.RX_Length - 2] * 256 + U4_Inf.RX_Data[U4_Inf.RX_Length - 1];

				

				
				
			//�ϰ�������豸���кŽ����޸�
				if( checksum == ModBusCRC16(U4_Inf.RX_Data,U4_Inf.RX_Length))
					{
						//u4_printf("\n*��ַ����= %d\n",Sys_Admin.ModBus_Address);
						
						if(sys_flag.Address_Number == 0)
							{
								//������Ļ�趨��ַ
								//if(U4_Inf.RX_Data[0] ==Sys_Admin.ModBus_Address)
								//	UART4_Server_Cmd_Analyse();
							}
						else
							{
								//�������岦���ַ
								if(U4_Inf.RX_Data[0] == sys_flag.Address_Number)
									{
										sys_flag.UnTalk_Time = 0;
										LCD4013X.DLCD.UnionControl_Flag = OK;  //�豸������״̬
										UART4_Server_Cmd_Analyse();
									}
									
							}
						
						
					
					}
					
			 
				
			/* 清空接收缓冲 */
				for( i = 0; i < 200;i++ )
					U4_Inf.RX_Data[i] = 0x00;
			
			/* DMA驱动: 清除接收完成标志 */
				uartClearRxFlag(&uartUnionHandle);
				
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
	uint8  Device_ID = Sys_Admin.ModBus_Address; //���ݵ�ַ�����ܴ� ************88
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
	Value = U4_Inf.RX_Data[4] *256+ U4_Inf.RX_Data[5]; //06��ʽ�£� 
	
	
	switch (Cmd)
		{
			case 03:  //00 03 01 F3 00 0F F5 D0		

					switch (Address)
						{
							case 100:
									if( Length == 18)  //��ߵ����ݳ���Ҳ�����ܱ仯�������ֽڳ��ȵı仯
										{
											
											U4_Inf.TX_Data[0] = Device_ID;
											U4_Inf.TX_Data[1] = 0x03;// 
											U4_Inf.TX_Data[2] = 36; //���ݳ���Ϊ6�� ��������ı�***
									
											U4_Inf.TX_Data[3] = 0x00;
											U4_Inf.TX_Data[4] = sys_data.Data_10H;// 1��ǰ������״̬

											U4_Inf.TX_Data[5] = Temperature_Data.Pressure_Value >> 8 ;
											U4_Inf.TX_Data[6] = Temperature_Data.Pressure_Value & 0X00FF;//2 ��ģ�鲻�߱�ѹ��������
											
											U4_Inf.TX_Data[7] = 0x00;
											U4_Inf.TX_Data[8] = 0;//3��ǰ�������¶�

											U4_Inf.TX_Data[9] = 0x00;
											U4_Inf.TX_Data[10] = LCD10D.DLCD.Water_State;//4��ǰˮλ��״̬

											
											U4_Inf.TX_Data[11] = 0;
											U4_Inf.TX_Data[12] = sys_data.Data_12H;//5 �쳣״̬����������


											 
											U4_Inf.TX_Data[13] = sys_flag.Protect_WenDu >> 8;
											U4_Inf.TX_Data[14] = sys_flag.Protect_WenDu & 0x00FF;//6�ڲ��������¶�

											U4_Inf.TX_Data[15] = 0;
											U4_Inf.TX_Data[16] = sys_flag.Error_Code;//7���ϴ���

											U4_Inf.TX_Data[17] = 0;
											U4_Inf.TX_Data[18] = sys_flag.flame_state;//8����״̬ 

											U4_Inf.TX_Data[19] = sys_flag.Fan_Rpm >> 8;
											U4_Inf.TX_Data[20] = sys_flag.Fan_Rpm & 0x00FF;//9���ת��

											U4_Inf.TX_Data[21] = Temperature_Data.Inside_High_Pressure >> 8;
											U4_Inf.TX_Data[22] = Temperature_Data.Inside_High_Pressure & 0x00FF;      //10 �������ڲ�ѹ��

 											U4_Inf.TX_Data[23] = 0;
 											U4_Inf.TX_Data[24] = sys_data.Data_1FH ;//11������� 

											U4_Inf.TX_Data[25] = 0;
											U4_Inf.TX_Data[26] = Switch_Inf.air_on_flag; //12�������״̬


											U4_Inf.TX_Data[27] = 0;
											U4_Inf.TX_Data[28] = Switch_Inf.Water_Valve_Flag;//13��ˮ��

											U4_Inf.TX_Data[29] = 0;
											U4_Inf.TX_Data[30] = Switch_Inf.pai_wu_flag;//14���۷�״̬

											U4_Inf.TX_Data[31] = 0;
											U4_Inf.TX_Data[32] = Switch_Inf.LianXu_PaiWu_flag;//15�������۷�״̬
											
											
											U4_Inf.TX_Data[33] = 0;
											U4_Inf.TX_Data[34] = 0;//16  δʹ��
											
											U4_Inf.TX_Data[35] = sys_flag.TimeDianHuo_Flag;
											U4_Inf.TX_Data[36] = sys_flag.Paiwu_Flag; //17  �Զ������Ѿ�������־�� 

											U4_Inf.TX_Data[37] = sys_flag.Inputs_State >> 8;
											U4_Inf.TX_Data[38] = sys_flag.Inputs_State & 0x00FF;//18 

											
											
										
										 
											check_sum  = ModBusCRC16(U4_Inf.TX_Data,41);   //����������ֽ����ı�
											U4_Inf.TX_Data[39]  = check_sum >> 8 ;
											U4_Inf.TX_Data[40]  = check_sum & 0x00FF;
											
											uartSendDma(&uartUnionHandle, U4_Inf.TX_Data,41);
											
										}

									break;
						
							
							
								

							default :
									break;
						}
					

					//���ѹ����������ת��
					
					break;
			
			case 0x10:   //����Ĵ���д��
						
						Data_Address = U4_Inf.RX_Data[2] * 256 + U4_Inf.RX_Data[3];
						Data_Length = U4_Inf.RX_Data[4] * 256 + U4_Inf.RX_Data[5];
						Buffer_Data16 = U4_Inf.RX_Data[7] *256 + U4_Inf.RX_Data[8];  //�ߵ��ֽڵ�˳��ߵ�
						
						switch (Data_Address)
							{
							case 0xC8:   //�������������д������
										// u1_printf("\n* �յ����� = %d\n",Buffer_Data16);

										//1��ָ��ģʽ��������ֹͣ���ֶ�
										switch (Buffer_Data16)
											{
											case 0:
													if(sys_data.Data_10H == 2)
														{
															sys_close_cmd();
														}
													if(sys_data.Data_10H == 3)
														{
															//�˳��ֶ�ģʽ
															sys_data.Data_10H = 0;
															GetOut_Mannual_Function();
														}

													break;
											case 1: //��������
													if(sys_data.Data_10H == 0)
														{
															sys_start_cmd();
														}

													break;
											case 3://�ֶ�ģʽ
													//ֻ���ڴ�������½����ֶ�
													if(sys_data.Data_10H == 0)
														{
															GetOut_Mannual_Function();
															sys_data.Data_10H = 3;
														}

													break;
											default:
													break;
											}
										

										Data1 = U4_Inf.RX_Data[9]*256 + U4_Inf.RX_Data[10];  //���ϸ�λָ��
										if(Data1 == OK)
											{
												sys_flag.Error_Code = 0; //���ϸ�λ
												
											}

									
										Data1 = U4_Inf.RX_Data[15]*256 + U4_Inf.RX_Data[16];  //5 �̵���16·���ƣ���λ��ȡ
										if(sys_data.Data_10H == 3)
											{
												//���ֶ�ģʽ�£����ݱ�־λ����Ӧ�ļ̵���
												
												if(Data1 & 0x0001)   //�����־λ
													{
														
														Send_Air_Open();
														
													}
												else
													{
														Send_Air_Close();
														 
													}

												if(Data1 & 0x0002)   //ˮ�ã���ˮ����־λ
													{
														Feed_Main_Pump_ON();
														Second_Water_Valve_Open();
													}
												else
													{
														 Feed_Main_Pump_OFF();
														 Second_Water_Valve_Close();
													}

												if(Data1 & 0x0004)   //��ˮ��
													{
														Pai_Wu_Door_Open();
													}
												else
													{
														Pai_Wu_Door_Close();
													}
												if(Data1 & 0x0008)   //����
													{
														WTS_Gas_One_Open();
													}
												else
													{
														WTS_Gas_One_Close();
													}

												
												
											}

										
										//����Ĺ��ʴ��䣬���豸״̬
										if(sys_data.Data_10H  ==  3)
											{
												if(U4_Inf.RX_Data[14] <= 100)
													{
														PWM_Adjust(U4_Inf.RX_Data[14]);
													}
											}

										if(sys_data.Data_10H == 2)
											{
												//�������У����л���
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

										//�ڴ��������У������Ҫ����һ���������е������ź�
										sys_flag.Idle_AirWork_Flag = U4_Inf.RX_Data[18];
										
										if(sys_flag.Paiwu_Flag == 0)
											{
												//��ֹ���Ѿ����յ�ָ������ֺ��ֱ����
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
		

										
										Data1 = U4_Inf.RX_Data[33]*256 + U4_Inf.RX_Data[34];  //�����¶ȵı���ֵ
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
					//��Чָ��
					break;
		}
 }


uint8 ModuBus4RTU_WriteResponse(uint16 address,uint16 Data16)
{
	uint16 check_sum = 0;
	
	U4_Inf.TX_Data[0] = U4_Inf.RX_Data[0];
	U4_Inf.TX_Data[1]= 0x06;

	
	U4_Inf.TX_Data[2] = address >> 8;    // ��ַ���ֽ�
	U4_Inf.TX_Data[3] = address & 0x00FF;  //��ַ���ֽ�
	

	U4_Inf.TX_Data[4] = Data16 >> 8;  //���ݸ��ֽ�
	U4_Inf.TX_Data[5] = Data16 & 0x00FF;   //���ݵ��ֽ�

	check_sum  = ModBusCRC16(U4_Inf.TX_Data,8);   //����������ֽ����ı�
	U4_Inf.TX_Data[6]  = check_sum >> 8 ;
	U4_Inf.TX_Data[7]  = check_sum & 0x00FF;

	uartSendDma(&uartUnionHandle, U4_Inf.TX_Data,8);

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
		
		uartSendDma(&uartUnionHandle, U4_Inf.TX_Data,8);


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

		uartSendDma(&uartUnionHandle, U4_Inf.TX_Data,8);

		

		return 0;
}

uint8 ModuBus4_Write0x10Response(uint8 Target_Address,uint16 Data_address,uint16 Data16)
{
	uint16 check_sum = 0;
	
	U4_Inf.TX_Data[0] = Target_Address;
	U4_Inf.TX_Data[1]= 0x10;

	
	U4_Inf.TX_Data[2] = Data_address >> 8;    // ��ַ���ֽ�
	U4_Inf.TX_Data[3] = Data_address & 0x00FF;  //��ַ���ֽ�
	

	U4_Inf.TX_Data[4] = Data16 >> 8;  //���ݸ��ֽ�
	U4_Inf.TX_Data[5] = Data16 & 0x00ff;   //���ݵ��ֽ�

	check_sum  = ModBusCRC16(U4_Inf.TX_Data,8);   //����������ֽ����ı�
	U4_Inf.TX_Data[6]  = check_sum >> 8 ;
	U4_Inf.TX_Data[7]  = check_sum & 0x00FF;

	uartSendDma(&uartUnionHandle, U4_Inf.TX_Data,8);

	return 0;
}




void Union_ModBus_Uart4_Local_Communication(void)
{
		
		uint8  i = 0;	
		uint8  Target_Address = 0;
		
		uint16 checksum = 0;

		for(i = 1; i < 13; i++)
			{
				if(SlaveG[i].Send_Flag > 20)//����6��δ��Ӧ����ʧ��
					{
						SlaveG[i].Alive_Flag = FALSE;
						JiZu[i].Slave_D.Device_State = 0;   //һ����ֲ���ⲿȥ
						memset(&JiZu[i].Datas,0,sizeof(JiZu[i].Datas)); //Ȼ�����������
					}
			}
		
		
		if(U4_Inf.Recive_Ok_Flag)
			{
				U4_Inf.Recive_Ok_Flag = 0;//������Ŷ
				 //�ر��ж�
				USART_ITConfig(UART4, USART_IT_RXNE, DISABLE); 
				 
				checksum  = U4_Inf.RX_Data[U4_Inf.RX_Length - 2] * 256 + U4_Inf.RX_Data[U4_Inf.RX_Length - 1];
				
			
				if(checksum == ModBusCRC16(U4_Inf.RX_Data,U4_Inf.RX_Length))
					{
						Target_Address = U4_Inf.RX_Data[0];

						/**********************����������е�����***************************************/
						if(AUnionD.OFFlive_Numbers)
							{
								//���н�ֹ�豸������ʱ��С�ڽ�ֹ��������ͨ��
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
					
			 
				
			//�Խ��ջ���������
				for( i = 0; i < 200;i++ )
					U4_Inf.RX_Data[i] = 0x00;
			
			//���¿����ж�
				USART_ITConfig(UART4, USART_IT_RXNE, ENABLE); 
				
			}
}


uint8 Union_Modbus4_UnionTx_Communication(void)
{
	static uint8 Address = 1;
	uint8 Max_Adress = 0;
	//	SlaveG[1].Alive_Flag  
	//	JiZu[1].Slave_D.Device_State  //�����ڸ������ݵ�ͬ��
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
	//���յ��ӻ������ݽ��д���
		
	SlaveG[address].Send_Flag = 0;  //������ͱ�־
	SlaveG[address].Alive_Flag = OK;
	if(Cmd == 0x03)
		{
			Data_Length = U4_Inf.RX_Data[2];

			if(Data_Length == 36)  //һ�ζ�ȡ18������
				{
					if(SlaveG[address].Startclose_Sendflag == 0) //��������·���ʵ�ʲ�һ�£��ȷ��ʹ������
						{
							//if(U4_Inf.RX_Data[4]  == 2)   //����������ı�־
							//	JiZu[address].Slave_D.StartFlag = OK;
							//else
							//	JiZu[address].Slave_D.StartFlag = 0;  
						}
					

					if(SlaveG[address].Send_Flag > 20)//����6��δ��Ӧ����ʧ��
						{
							JiZu[address].Slave_D.Device_State = 0;   //һ����ֲ���ⲿȥ
						}
					else
						{
							if(U4_Inf.RX_Data[4]  == 2)
								JiZu[address].Slave_D.Device_State = 2;  //����״̬���л�
							if(U4_Inf.RX_Data[4]  == 0 )
								JiZu[address].Slave_D.Device_State = 1;  //����ģʽ
							if(U4_Inf.RX_Data[16])  //������ʾ
								JiZu[address].Slave_D.Device_State = 3; //����״̬
							if(U4_Inf.RX_Data[4]  == 3)
								JiZu[address].Slave_D.Device_State = 1;//�ֶ�״̬
						}

					
					Buffer_Float = U4_Inf.RX_Data[5] *255 + U4_Inf.RX_Data[6];
				 
					 
				 
					
					JiZu[address].Slave_D.Dpressure = Buffer_Float / 100;
					 
					

					JiZu[address].Slave_D.Water_State = U4_Inf.RX_Data[10] ;
						
					
					JiZu[address].Slave_D.Inside_WenDu = U4_Inf.RX_Data[13] * 256  + U4_Inf.RX_Data[14]; 

					 
					JiZu[address].Slave_D.Error_Code = U4_Inf.RX_Data[16] ;//��ȡ���ϴ���
					
					
					if(JiZu[address].Slave_D.Error_Code == 0)
						{
							JiZu[address].Slave_D.Error_Reset = 0;  //û�й���ʱ��ȡ����λ����
						}

					

					JiZu[address].Slave_D.Flame = U4_Inf.RX_Data[18] ;

					JiZu[address].Slave_D.Fan_Rpm = U4_Inf.RX_Data[19] * 256  + U4_Inf.RX_Data[20];  //��ȡ�����ת��


						
					
					Buffer_Float = U4_Inf.RX_Data[21] * 256 + U4_Inf.RX_Data[22];
					//JiZu[address].Slave_D.Dpressure = Buffer_Float / 100;  //����ѹ��

					 
					JiZu[address].Slave_D.Power = U4_Inf.RX_Data[24] ;//9�������

					JiZu[address].Slave_D.Pump_State = U4_Inf.RX_Data[28] ;
					

					SlaveG[address].TimeDianHuo_Flag = U4_Inf.RX_Data[35];

					

					if(SlaveG[address].Paiwu_Flag)
						{
							if(U4_Inf.RX_Data[36]) //�����鷴�����Ƿ������۵ı�־
								{
									SlaveG[address].Paiwu_Flag = 0; //���ٷ���Ҫ�������۵�ָ��
								}
						}
					
				 
					
				}

		
		}

	if(Cmd == 0x10)
		{
				Data_Address = U4_Inf.RX_Data[2]*256 + U4_Inf.RX_Data[3] ;
				if(Data_Address == 0x00C8)
					{
						SlaveG[address].Command_SendFlag = 0; //����Ӧ�ı�־λȡ��
					}

				
		}
		


		return 0;
}



uint8 ModBus4RTU_Write0x10Function(uint8 Target_Address,uint16 Data_Address,uint16 Length)
{
	//01  10  00 A4	00 02  04  00 0F 3D DD 18 EE д32λ����ָ���ʽ
	//��Ӧ�� 01 10 00 A4	  00 02 crc
		uint16 Check_Sum = 0;
		U4_Inf.TX_Data[0] = Target_Address;
		U4_Inf.TX_Data[1] = 0x10;

		U4_Inf.TX_Data[2]= Data_Address >> 8;
		U4_Inf.TX_Data[3]= Data_Address & 0x00FF;

		U4_Inf.TX_Data[4]= Length >> 8;
		U4_Inf.TX_Data[5]= Length & 0x00FF;

		U4_Inf.TX_Data[6]= Length * 2;  //���ֽ���

		U4_Inf.TX_Data[7]= 0;
		U4_Inf.TX_Data[8]= JiZu[Target_Address].Slave_D.StartFlag;  //1������ر�����

		U4_Inf.TX_Data[9]= 0;
		U4_Inf.TX_Data[10]= JiZu[Target_Address].Slave_D.Error_Reset;  //2���ϸ�λ����

		U4_Inf.TX_Data[11]= 0;
		U4_Inf.TX_Data[12]= AUnionD.Devive_Style;  // 3�豸���� 

		
		U4_Inf.TX_Data[13]= 0;
		U4_Inf.TX_Data[14]= SlaveG[Target_Address].Out_Power;	//4 �豸��Ҫ���еĹ���

		U4_Inf.TX_Data[15]= JiZu[Target_Address].Slave_D.Realys_Out >> 8;
		U4_Inf.TX_Data[16]= JiZu[Target_Address].Slave_D.Realys_Out & 0x00FF;	//5 �̵���16·���ƣ���λ��ȡ

		U4_Inf.TX_Data[17]= 0;
		U4_Inf.TX_Data[18]= SlaveG[Target_Address].Idle_AirWork_Flag;	//6 ������������ź�

		U4_Inf.TX_Data[19]= 0;
		U4_Inf.TX_Data[20]= SlaveG[Target_Address].Paiwu_Flag;	//7 �Զ����ۿ�������

		U4_Inf.TX_Data[21]= 0;
		U4_Inf.TX_Data[22]= sys_config_data.zhuan_huan_temperture_value;	//8 Ŀ��ѹ��

		U4_Inf.TX_Data[23]= 0;
		U4_Inf.TX_Data[24]= sys_config_data.Auto_stop_pressure;	//9 ͣ��ѹ��

		U4_Inf.TX_Data[25]= 0;
		U4_Inf.TX_Data[26]= sys_config_data.Auto_start_pressure;	//10 ����ѹ��

		U4_Inf.TX_Data[27]= 0;
		U4_Inf.TX_Data[28]= Sys_Admin.DeviceMaxPressureSet;	//11�ѹ��

		U4_Inf.TX_Data[29]= 0;
		U4_Inf.TX_Data[30]= JiZu[Target_Address].Slave_D.DianHuo_Value;	//12 �����

		U4_Inf.TX_Data[31]= 0;
		U4_Inf.TX_Data[32]= JiZu[Target_Address].Slave_D.Max_Power;	//13 ������й���

		U4_Inf.TX_Data[33]= JiZu[Target_Address].Slave_D.Inside_WenDu_Protect >> 8;
		U4_Inf.TX_Data[34]= JiZu[Target_Address].Slave_D.Inside_WenDu_Protect & 0x00FF;	//14 ¯�±���ֵ

		U4_Inf.TX_Data[35]= 0;
		U4_Inf.TX_Data[36]= sys_flag.DianHuo_Alive;	//15 Ԥ������2    ���Ƶ������У�������ʱ����Ҫ���������

		U4_Inf.TX_Data[37]= 0;
		U4_Inf.TX_Data[38]= 0;	//16 Ԥ������3

		U4_Inf.TX_Data[39]= 0;
		U4_Inf.TX_Data[40]= 0;	//17 Ԥ������4

		U4_Inf.TX_Data[41]= 0;
		U4_Inf.TX_Data[42]= 0;	//18 Ԥ������5
		
		Check_Sum = ModBusCRC16(U4_Inf.TX_Data,45);
		U4_Inf.TX_Data[43]= Check_Sum >> 8;
		U4_Inf.TX_Data[44]= Check_Sum & 0x00FF;

		uartSendDma(&uartUnionHandle, U4_Inf.TX_Data,45);

			

		return 0;
}


uint8 JiZu_ReadAndWrite_Function(uint8 address)
{
		
		static uint8 index  = 0;
		uint8 return_value = 0;
		static uint8 Write_Index = 0; //�����л���ͬ������
		
		switch (index)
			{
			case 0:
					if(SlaveG[address].Alive_Flag == 0)
						{
							//����豸�����ߣ���ֻ���ֶ���״̬
							SlaveG[address].Send_Index = 0;
						}
					
					if(SlaveG[address].Send_Index)
						{
							//SlaveG[address].Command_SendFlag--; //�Է��ʹ����ݼ�
							
							SlaveG[address].Send_Index = 0;
							ModBus4RTU_Write0x10Function(address,200,18);//18������һ����д��
						}
					else
						{
							SlaveG[address].Send_Index  = OK;
							ModBus4_RTU_Read03(address,100,18); //�����Ĭ�϶�ȡ���ݵ�ָ��
							
						}
									
					SlaveG[address].Send_Flag ++; //�Է��ͽ��м���
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






