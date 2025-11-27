#include "usart2.h"
#include "stdarg.h"
#include "uart_driver.h"

/* �?用新UART驱动 (与main.c和stm32f10x_it.c保持一�?) */
#define USE_NEW_UART_DRIVER
 



 UtoSlave_Info UtoSlave;
 Duble5_Info Double5;

 UNION_FLAGS Union_Flag; 	 

LCD10Struct LCD10D;
UNION_GGA AUnionD;

LCD10_JZ_Struct LCD10JZ[13];  //���ڻ�����������?

ALCD10Struct UnionLCD;


LCD_WR  LCD10WR;

LCD4013_Struct LCD4013X;






void U2_send_byte(u8 data)
{
	
	while(USART_GetFlagStatus(USART2,USART_FLAG_TC)!=SET);
		USART_SendData(USART2,data);
	 
}

//����4����s���ַ�
void U2_send_str(u8 *str,u8 s)
{
	u8 i;
	for(i=0;i<s;i++)
	{
		U2_send_byte(*str);
		str++;
	}
}


void u2_printf(char* fmt,...)  
{  
  	int len=0;
	int cnt=0;
	va_list ap;
	va_start(ap,fmt);
	vsprintf((char*)U2_Inf.TX_Data,fmt,ap);
	va_end(ap);
	len = strlen((const char*)U2_Inf.TX_Data);
	while(len--)
	  {
	  while(USART_GetFlagStatus(USART2,USART_FLAG_TC)!=1); //�ȴ����ͽ���
	  USART_SendData(USART2,U2_Inf.TX_Data[cnt++]);
	  }
}




///////////////////////////////////////USART2 ��ʼ�����ò���//////////////////////////////////	    
//���ܣ���ʼ��IO ����2
//�������?
//bound:������
//�������?
//��
//////////////////////////////////////////////////////////////////////////////////////////////	  
void uart2_init(u32 bound)
{  	 		 
	//GPIO�˿�����
    GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	 
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);	//ʹ��USART2
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	//ʹ��GPIOAʱ��
	USART_DeInit(USART2);  //��λ����2

     //USART2_TX   PA.2
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2; //PA.2
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//�����������?
    GPIO_Init(GPIOA, &GPIO_InitStructure);
   
    //USART2_RX	  PA.3
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//��������
    GPIO_Init(GPIOA, &GPIO_InitStructure);  

    //Usart2 NVIC ����
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority= 2;//��ռ���ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;		//�����ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	        //����ָ���Ĳ�����ʼ��VIC�Ĵ���
  
    //USART2 ��ʼ������
	USART_InitStructure.USART_BaudRate = bound;   //һ������Ϊ115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;  //�ֳ�Ϊ8λ���ݸ�ʽ
	USART_InitStructure.USART_StopBits = USART_StopBits_1;  //һ��ֹͣλ
	USART_InitStructure.USART_Parity = USART_Parity_No;  //����żУ��λ
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;  //��Ӳ������������
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	  //�շ�ģʽ

    USART_Init(USART2, &USART_InitStructure);   //��ʼ������
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);  //�����ж�
    USART_Cmd(USART2, ENABLE);                      //ʹ�ܴ��� 
	
//	USART_DMACmd(USART2,USART_DMAReq_Tx,ENABLE);    //ʹ�ܴ���2��DMA����
//	UART_DMA_Config(DMA1_Channel7,(u32)&USART2->DR,(u32)USART2_TX_BUF,1000);  //DMA1ͨ��7,����Ϊ����2,�洢��ΪUSART2_TX_BUF,����1000. 										  	
}



void Union_ModBus2_Communication(void)
{
		uint8  i = 0;	
		uint8 index = 0;
		uint8 Bytes = 0;

		uint8 Modbus_Address = 0;
		
		uint16 checksum = 0;
		uint8  Cmd_Data = 0;
		uint16 Data_Address = 0;
		uint16 Buffer_Data16 = 0;
		uint16 Data_Length = 0;
		uint8  Index_Address = 0;

		uint16 Buffer_Int1 = 0;

#ifdef USE_NEW_UART_DRIVER
		uint8_t* rxData;
		uint16_t rxLen;
#endif

#ifdef USE_NEW_UART_DRIVER
		/* 新驱�?: 检�?DMA接收完成标志 */
		if (uartIsRxReady(&uartDisplayHandle))
		{
			if (uartGetRxData(&uartDisplayHandle, &rxData, &rxLen) != UART_OK)
				return;
			
			/* 复制数据到U2_Inf以兼容后�?代码 */
			for (i = 0; i < rxLen && i < 300; i++)
				U2_Inf.RX_Data[i] = rxData[i];
			U2_Inf.RX_Length = rxLen;
			U2_Inf.Recive_Ok_Flag = 1;
		}
#endif
		
		if(U2_Inf.Recive_Ok_Flag)
			{
				U2_Inf.Recive_Ok_Flag = 0;
#ifndef USE_NEW_UART_DRIVER
				/* 旧驱�?: 关闭�?�? */
				USART_ITConfig(USART2, USART_IT_RXNE, DISABLE);
#endif
				 
				checksum  = U2_Inf.RX_Data[U2_Inf.RX_Length - 2] * 256 + U2_Inf.RX_Data[U2_Inf.RX_Length - 1];
				
			 	
				if(checksum == ModBusCRC16(U2_Inf.RX_Data,U2_Inf.RX_Length))
					{	
						Modbus_Address = U2_Inf.RX_Data[0];
						Cmd_Data = U2_Inf.RX_Data[1];
						
						sys_flag.LCD10_Connect = OK;
						
						if(Cmd_Data == 0x03 && Modbus_Address == 1)
							{
								
								Data_Address = U2_Inf.RX_Data[2] * 256 + U2_Inf.RX_Data[3];
								Buffer_Data16 = U2_Inf.RX_Data[4] * 256 + U2_Inf.RX_Data[5];
								Data_Length = Buffer_Data16;
								switch (Data_Address)
									{
									 case 0x0000:
									 		
									 
									 			Bytes = sizeof(UnionLCD.Datas);
												U2_Inf.TX_Data[0] = 0x01;
												U2_Inf.TX_Data[1]= 0x03;
									 			U2_Inf.TX_Data[2] = Bytes; //�������ݳ��ȸı�
									 			
											
									 			for(index = 3; index < (Bytes + 3); index ++)
													U2_Inf.TX_Data[index] = UnionLCD.Datas[index -3];
												
											
									 			checksum  = ModBusCRC16(U2_Inf.TX_Data,Bytes + 5);
												U2_Inf.TX_Data[Bytes + 3] = checksum >> 8;
												U2_Inf.TX_Data[Bytes + 4] = checksum & 0x00FF;
												
#ifdef USE_NEW_UART_DRIVER
									 			uartSendDma(&uartDisplayHandle, U2_Inf.TX_Data, Bytes + 5);
#else
									 			Usart_SendStr_length(USART2,U2_Inf.TX_Data,Bytes +5);
#endif

											
									 			break;

									case 0x0063://ȡ����1������ֵ
												Bytes = sizeof(JiZu[1].Datas);
												U2_Inf.TX_Data[0] = 0x01;
												U2_Inf.TX_Data[1]= 0x03;
									 			U2_Inf.TX_Data[2] = Bytes; //�������ݳ��ȸı�

												
									 			for(index = 3; index < (Bytes + 3); index ++)
													U2_Inf.TX_Data[index] = JiZu[1].Datas[index -3]; //�������ַ����?
													
									 			checksum  = ModBusCRC16(U2_Inf.TX_Data,Bytes + 5);
												U2_Inf.TX_Data[Bytes + 3] = checksum >> 8;
												U2_Inf.TX_Data[Bytes + 4] = checksum & 0x00FF;
												
									 			Usart_SendStr_length(USART2,U2_Inf.TX_Data,Bytes +5);

												break;
									case 0x0077://ȡ����2������ֵ
												Jizu_ReadResponse(2);
												break;

									case 0x008B://ȡ����3������ֵ
												Jizu_ReadResponse(3);
												break;

									case 0x009F://ȡ����4������ֵ
												Jizu_ReadResponse(4);
												break;
									case 0x00B3://ȡ����5������ֵ
												Jizu_ReadResponse(5);
												break;
									case 0x00C7://ȡ����6������ֵ
												Jizu_ReadResponse(6);
												break;
									case 0x00DB://ȡ����7������ֵ
												Jizu_ReadResponse(7);
												break;
									case 0x00EF://ȡ����8������ֵ
												Jizu_ReadResponse(8);
												break;
									case 0x0103://ȡ����9������ֵ
												Jizu_ReadResponse(9);
												break;
									case 0x0117://ȡ����10������ֵ
												Jizu_ReadResponse(10);
												break;
									default:
										

											break;
									}
							}

						if(Cmd_Data == 0x10 && Modbus_Address == 1)
							{
								 //01  10  00 A4	00 02  04  00 0F 3D DD 18 EE д32λ����ָ����?
					  			 //��Ӧ�� 01 10 00 A4	  00 02 crc
								Data_Address = U2_Inf.RX_Data[2] * 256 + U2_Inf.RX_Data[3];
								Data_Length = U2_Inf.RX_Data[4] * 256 + U2_Inf.RX_Data[5];
								Buffer_Data16 = U2_Inf.RX_Data[7]  + U2_Inf.RX_Data[8] * 256;  //�ߵ��ֽڵ�˳��ߵ�?  //�ߵ��ֽڵ�˳��ߵ�?
						
								switch (Data_Address)
									{
									 case 0x0000:  //������������ָ��

													switch (Buffer_Data16)
														{
														case 0: //��������
																if(AUnionD.UnionStartFlag == 1)  //�豸�����У���ִ�йر�
																	{
																		AUnionD.UnionStartFlag = Buffer_Data16;
																		UnionLCD.UnionD.UnionStartFlag = Buffer_Data16;
																	}
																if(AUnionD.UnionStartFlag == 3)  //�豸�ֶ�ģʽ����ִ���˳��ֶ������д���
																	{
																		for(Index_Address = 1; Index_Address <= 10; Index_Address ++)
											 								{
											 									JiZu[Index_Address].Slave_D.StartFlag = 0; //������ȫ���������ģ�?
											 									JiZu[Index_Address].Slave_D.Realys_Out = 0;  //��ʼ�����м̵���
											 									SlaveG[Index_Address].Out_Power = 0;
											 								}
																		AUnionD.UnionStartFlag = Buffer_Data16;
																		UnionLCD.UnionD.UnionStartFlag = Buffer_Data16;
																		UnionLCD.UnionD.ZongKong_RelaysOut = 0;
																	}

																break;
														case 1:
																if(AUnionD.UnionStartFlag == 0)  //�豸�ڴ�������ִ����������
																	{
																		AUnionD.UnionStartFlag = Buffer_Data16;
																		UnionLCD.UnionD.UnionStartFlag = Buffer_Data16;
																	}

																break;
														case 3:
																if(AUnionD.UnionStartFlag == 0)  //�豸������ģʽ����ִ�н����ֶ���
																	{
																		for(Index_Address = 1; Index_Address <= 10; Index_Address ++)
											 								{
											 									JiZu[Index_Address].Slave_D.StartFlag = 3; //������ȫ�������ֶ�ģʽ
											 									JiZu[Index_Address].Slave_D.Realys_Out = 0;  //��ʼ�����м̵���
											 									SlaveG[Index_Address].Out_Power = 0;
											 								}
																		AUnionD.UnionStartFlag = Buffer_Data16;
																		UnionLCD.UnionD.UnionStartFlag = Buffer_Data16;
																		UnionLCD.UnionD.ZongKong_RelaysOut = 0;
																	}

																break;
														default:
																break;
														}
									 			
									 			  ModuBus2LCD_Write0x10Response(Data_Address,Data_Length);
									 		
									 			break;
									case 0x0001:  //����ʹ��̨��
												if(Buffer_Data16 <= 10)  //���õĲ�������С��10
									 				UnionLCD.UnionD.Need_Numbers = Buffer_Data16;
												else
													Buffer_Data16 = UnionLCD.UnionD.Need_Numbers;

												AUnionD.Need_Numbers = UnionLCD.UnionD.Need_Numbers;


									 			ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
									 		
									 			break;

									 			break;
									case 0x0005:  //PIDת��ʱ��
												
												if(Buffer_Data16 <= 100 && Buffer_Data16 >= 5)  //���õĲ�������С��100,����5
													UnionLCD.UnionD.PID_Next_Time = Buffer_Data16;
												else
													Buffer_Data16 = UnionLCD.UnionD.PID_Next_Time;
												
												AUnionD.PID_Next_Time = UnionLCD.UnionD.PID_Next_Time;
													
												
									 			ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
									 		
									 			break;

									case 0x0006:  //PIDֵP
												
												if(Buffer_Data16 <= 50)  //���õĲ�������С��50
													UnionLCD.UnionD.PID_Pvalue = Buffer_Data16;
												else
													Buffer_Data16 = UnionLCD.UnionD.PID_Pvalue;
												
												AUnionD.PID_Pvalue = UnionLCD.UnionD.PID_Pvalue;
													
												
									 			ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
									 		
									 			break;

									case 0x0007:  //PIDֵI
												
												if(Buffer_Data16 <= 10)  //���õĲ�������С��10
													UnionLCD.UnionD.PID_Ivalue = Buffer_Data16;
												else
													Buffer_Data16 = UnionLCD.UnionD.PID_Ivalue;
												
												AUnionD.PID_Ivalue = UnionLCD.UnionD.PID_Ivalue;
													
												
									 			ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
									 		
									 			break;
									case 0x0008:  //PIDֵD
												
												if(Buffer_Data16 <= 30)  //���õĲ�������С��30
													UnionLCD.UnionD.PID_Dvalue = Buffer_Data16;
												else
													Buffer_Data16 = UnionLCD.UnionD.PID_Dvalue;
												
												AUnionD.PID_Dvalue = UnionLCD.UnionD.PID_Dvalue;
													
												
									 			ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
									 		
									 			break;

									case 0x0009:  //16̨�豸����ʹ�ñ�־
												
												if(Buffer_Data16 <= 65535)  //���õĲ�������С��65535
													UnionLCD.UnionD.Union16_Flag = Buffer_Data16;
												else
													Buffer_Data16 = UnionLCD.UnionD.Union16_Flag;
												
												AUnionD.Union16_Flag = UnionLCD.UnionD.Union16_Flag;
												
									 			ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
									 		
									 			break;



												
									case 0x000C:  //Ŀ��ѹ�� ������ 01 10 00 0C 00 02 04                14 AE 07 3F     D5 CB 
												if(Data_Length == 0x02)
													{
														Float_Int.byte4.data_LL = U2_Inf.RX_Data[7];
														Float_Int.byte4.data_LH = U2_Inf.RX_Data[8];
														Float_Int.byte4.data_HL = U2_Inf.RX_Data[9];
														Float_Int.byte4.data_HH = U2_Inf.RX_Data[10];
														UnionLCD.UnionD.Target_Value = Float_Int.value;
														
														Buffer_Int1 = UnionLCD.UnionD.Target_Value * 100;
														
														if(Buffer_Int1 >= 10 && Buffer_Int1 <=  Sys_Admin.DeviceMaxPressureSet)
															{
																 sys_config_data.zhuan_huan_temperture_value = Buffer_Int1;
																 UnionLCD.UnionD.Target_Value = Float_Int.value;
																 if(sys_config_data.zhuan_huan_temperture_value > sys_config_data.Auto_stop_pressure)
																 	{
																 		sys_config_data.Auto_stop_pressure = sys_config_data.zhuan_huan_temperture_value + 10;
																		if(sys_config_data.Auto_stop_pressure >= Sys_Admin.DeviceMaxPressureSet )
																			sys_config_data.Auto_stop_pressure = Sys_Admin.DeviceMaxPressureSet - 1;

																		UnionLCD.UnionD.Stop_Value = (float) sys_config_data.Auto_stop_pressure / 100;
																 	}
															
																  Write_Internal_Flash();
																

																 sys_config_data.zhuan_huan_temperture_value = *(uint32 *)(ZHUAN_HUAN_TEMPERATURE);
																 
																
															}
														
														
														ModuBus2LCD_Write0x10Response(Data_Address,Data_Length);	
													}

												break;
									case 0x000E:  //ֹͣѹ�� ������
												if(Data_Length == 0x02)
													{
														Float_Int.byte4.data_LL = U2_Inf.RX_Data[7];
														Float_Int.byte4.data_LH = U2_Inf.RX_Data[8];
														Float_Int.byte4.data_HL = U2_Inf.RX_Data[9];
														Float_Int.byte4.data_HH = U2_Inf.RX_Data[10];
														UnionLCD.UnionD.Stop_Value = Float_Int.value;
														Buffer_Int1 = UnionLCD.UnionD.Stop_Value * 100;

													//	u1_printf("\n*ֹͣѹ��ֵ = %d   \n",Buffer_Int1);

														if(Buffer_Int1 >= sys_config_data.zhuan_huan_temperture_value && Buffer_Int1 <=  Sys_Admin.DeviceMaxPressureSet)
															{
																sys_config_data.Auto_stop_pressure = Buffer_Int1;
															 
																Write_Internal_Flash();
																 sys_config_data.Auto_stop_pressure = *(uint32 *)(AUTO_STOP_PRESSURE_ADDRESS);
															}
														
														ModuBus2LCD_Write0x10Response(Data_Address,Data_Length);	
													}

												break;
									case 0x0010:  //����ѹ�� ������
												if(Data_Length == 0x02)
													{
														Float_Int.byte4.data_LL = U2_Inf.RX_Data[7];
														Float_Int.byte4.data_LH = U2_Inf.RX_Data[8];
														Float_Int.byte4.data_HL = U2_Inf.RX_Data[9];
														Float_Int.byte4.data_HH = U2_Inf.RX_Data[10];
														UnionLCD.UnionD.Start_Value = Float_Int.value;
														
														Buffer_Int1 = UnionLCD.UnionD.Start_Value * 100;
														if( Buffer_Int1 <  sys_config_data.Auto_stop_pressure)
															{
																sys_config_data.Auto_start_pressure = Buffer_Int1;
																UnionLCD.UnionD.Start_Value = Float_Int.value;
																Write_Internal_Flash();
															}
														ModuBus2LCD_Write0x10Response(Data_Address,Data_Length);	
													}

												break;
									case 0x0012:  //�ѹ����������
												if(Data_Length == 0x02)
													{
														Float_Int.byte4.data_LL = U2_Inf.RX_Data[7];
														Float_Int.byte4.data_LH = U2_Inf.RX_Data[8];
														Float_Int.byte4.data_HL = U2_Inf.RX_Data[9];
														Float_Int.byte4.data_HH = U2_Inf.RX_Data[10];
														UnionLCD.UnionD.Max_Pressure = Float_Int.value;
														
														Buffer_Int1 = UnionLCD.UnionD.Max_Pressure * 100;
														
														AUnionD.Max_Pressure = UnionLCD.UnionD.Max_Pressure;
														
														if( Buffer_Int1 <= 250) //���ѹ��С��2.5Mpa
															{
																Sys_Admin.DeviceMaxPressureSet = Buffer_Int1;
																Write_Admin_Flash();
															}
														ModuBus2LCD_Write0x10Response(Data_Address,Data_Length);	
													}

												break;
									case 20:  //A1����ʱ��
												UnionLCD.UnionD.A1_WorkTime = Buffer_Data16;												
												AUnionD.A1_WorkTime = UnionLCD.UnionD.A1_WorkTime;
												SlaveG[1].Work_Time = AUnionD.A1_WorkTime;
									 			ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
									 			break;
									case 21:  //A2����ʱ��
												UnionLCD.UnionD.A2_WorkTime = Buffer_Data16;												
												AUnionD.A2_WorkTime = UnionLCD.UnionD.A2_WorkTime;
												SlaveG[2].Work_Time = AUnionD.A2_WorkTime;
									 			ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
									 			break;
									case 22:  //A3����ʱ��
												UnionLCD.UnionD.A3_WorkTime = Buffer_Data16;												
												AUnionD.A3_WorkTime = UnionLCD.UnionD.A3_WorkTime;
												SlaveG[3].Work_Time = AUnionD.A3_WorkTime;
									 			ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
									 			break;
									case 23:  //A4����ʱ��
												UnionLCD.UnionD.A4_WorkTime = Buffer_Data16;												
												AUnionD.A4_WorkTime = UnionLCD.UnionD.A4_WorkTime;
												SlaveG[4].Work_Time = AUnionD.A4_WorkTime;
									 			ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
									 			break;
									case 24:  //A5����ʱ��
												UnionLCD.UnionD.A5_WorkTime = Buffer_Data16;												
												AUnionD.A5_WorkTime = UnionLCD.UnionD.A5_WorkTime;
												SlaveG[5].Work_Time = AUnionD.A5_WorkTime;
									 			ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
									 			break;
									case 25:  //A6����ʱ��
												UnionLCD.UnionD.A6_WorkTime = Buffer_Data16;												
												AUnionD.A6_WorkTime = UnionLCD.UnionD.A6_WorkTime;
												SlaveG[6].Work_Time = AUnionD.A6_WorkTime;
									 			ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
									 			break;
									case 26:  //A7����ʱ��
												UnionLCD.UnionD.A7_WorkTime = Buffer_Data16;												
												AUnionD.A7_WorkTime = UnionLCD.UnionD.A7_WorkTime;
												SlaveG[7].Work_Time = AUnionD.A7_WorkTime;
									 			ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
									 			break;
									case 27:  //A8����ʱ��
												UnionLCD.UnionD.A8_WorkTime = Buffer_Data16;												
												AUnionD.A8_WorkTime = UnionLCD.UnionD.A8_WorkTime;
												SlaveG[8].Work_Time = AUnionD.A8_WorkTime;
									 			ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
									 			break;
									case 28:  //A9����ʱ��
												UnionLCD.UnionD.A9_WorkTime = Buffer_Data16;												
												AUnionD.A9_WorkTime = UnionLCD.UnionD.A9_WorkTime;
												SlaveG[9].Work_Time = AUnionD.A9_WorkTime;
									 			ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
									 			break;
									case 29:  //A10����ʱ��
												UnionLCD.UnionD.A10_WorkTime = Buffer_Data16;												
												AUnionD.A10_WorkTime = UnionLCD.UnionD.A10_WorkTime;
												SlaveG[10].Work_Time = AUnionD.A10_WorkTime;
									 			ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
									 			break;

									case 30:  //���ϱ�����λָ��
												UnionLCD.UnionD.Error_Reset = Buffer_Data16;												
												AUnionD.Error_Reset = UnionLCD.UnionD.Error_Reset;
												UnionLCD.UnionD.Union_Error = 0;  //�ܿع�����Ҳ��Ҫ����

												for(index = 1;index <= 10; index ++)
													{
														if(JiZu[index].Slave_D.Error_Code)
															{
																SlaveG[index].Command_SendFlag = 3; //����������
																JiZu[index].Slave_D.Error_Reset = OK;
																
															 
															}
													}

												
									 			ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
									 			break;
									case 31:  //���ϱ����������?
												UnionLCD.UnionD.Alarm_OFF = Buffer_Data16;												
												AUnionD.Alarm_OFF = UnionLCD.UnionD.Alarm_OFF;
												
									 			ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
									 			break;
									case 32:  //�豸����
									
												UnionLCD.UnionD.Devive_Style = Buffer_Data16;												
												AUnionD.Devive_Style = UnionLCD.UnionD.Devive_Style;
												for(index = 1;index <= 10; index ++)
													{
														if(SlaveG[index].Alive_Flag)
															{
																if(JiZu[index].Slave_D.Error_Code)
																	{
																		SlaveG[index].Command_SendFlag = 3; //����������
																		JiZu[index].Slave_D.Error_Code = 0;
																	}
																	
															}
														
													}
												
									 			ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
									 			break;
									case 33:  //��豸����
												if(Buffer_Data16 <= 10)
													{
														UnionLCD.UnionD.Max_Address = Buffer_Data16;												
														AUnionD.Max_Address = UnionLCD.UnionD.Max_Address;
													}
												
									 			ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
									 			break;
									case 35:  //ModBusͨ�ŵ�ַ
												if(Buffer_Data16 <= 250)
													{
														UnionLCD.UnionD.ModBus_Address = Buffer_Data16;												
														AUnionD.ModBus_Address = UnionLCD.UnionD.ModBus_Address;
														Sys_Admin.ModBus_Address = UnionLCD.UnionD.ModBus_Address;
													}
												
									 			ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
									 			break;

									case 36:  //��ֹ�豸����
												if(Buffer_Data16 <= 10)
													{
														UnionLCD.UnionD.OFFlive_Numbers = Buffer_Data16;												
														AUnionD.OFFlive_Numbers = UnionLCD.UnionD.OFFlive_Numbers;
													}
												
									 			ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
									 			break;

									case 38:  //�����¶ȱ���ֵ
												if(Buffer_Data16 <= 200 && Buffer_Data16 >= 80)
													{
														UnionLCD.UnionD.PaiYan_AlarmValue = Buffer_Data16;												
														AUnionD.PaiYan_AlarmValue = UnionLCD.UnionD.PaiYan_AlarmValue;
														Sys_Admin.Danger_Smoke_Value = Buffer_Data16 *10;
														 Write_Admin_Flash();
													}
												
									 			ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
									 			break;
									case 40:  //�ܿ���ر����������?
												
												UnionLCD.UnionD.Alarm_Allow_Flag = Buffer_Data16;												
												AUnionD.Alarm_Allow_Flag = UnionLCD.UnionD.Alarm_Allow_Flag;
									
												
									 			ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
									 			break;

									case 41:  //�ܿؼ̵������?
												
												UnionLCD.UnionD.ZongKong_RelaysOut = Buffer_Data16;												
												AUnionD.ZongKong_RelaysOut = UnionLCD.UnionD.ZongKong_RelaysOut;
												if(Buffer_Data16 & 0x0001)   //�����־�?
													{
														
														ZongKong_YanFa_Open();
														
													}
												else
													{
														ZongKong_YanFa_Close();
														 
													}
												
									 			ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
									 			break;
									
									case 101:// A1���ʵ������ָ��?���ֶ�ģʽ����Ч
																					
												 
												SlaveG[1].Out_Power = Buffer_Data16;
												JiZu[1].Slave_D.Power = Buffer_Data16;
												 
												ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
									
											break;
									case 108:// A1Ҫ�������۵�ָ��
																					
												JiZu[1].Slave_D.PaiwuFa_State = Buffer_Data16;
												SlaveG[1].Paiwu_Flag = Buffer_Data16;
												ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
									
											break;
									
									case 113:// A1�̵������ָ��?���ֶ�ģʽ����Ч

												JiZu[1].Slave_D.Realys_Out = Buffer_Data16;
												ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);

											break;
									case 114:// A1����ʱ����?
												if(Buffer_Data16 < 60 &&Buffer_Data16 >=30)
													{
														JiZu[1].Slave_D.DianHuo_Value = Buffer_Data16;
														ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
													}

											break;
									case 115:// A1����ʱ����?
												if(Buffer_Data16 >=30 && Buffer_Data16 <= 100)
													{
														JiZu[1].Slave_D.Max_Power = Buffer_Data16;
														ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
													}

											break;
									case 116:// A1�����¶ȱ���ֵ
												if(Buffer_Data16 < 350)
													{
														JiZu[1].Slave_D.Inside_WenDu_Protect = Buffer_Data16;
														ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
													}

											break;


									//*****************A222222������صĿ���ָ��?*********************8		
									case 121:// A2���ʵ������ָ��?���ֶ�ģʽ����Ч
									
												SlaveG[2].Out_Power = Buffer_Data16;
												JiZu[2].Slave_D.Power = Buffer_Data16;
												ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
									
											break;

									case 128:// A2Ҫ�������۵�ָ��
																					
												JiZu[2].Slave_D.PaiwuFa_State = Buffer_Data16;
												SlaveG[2].Paiwu_Flag = Buffer_Data16;
												ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
									
											break;
									case 133:// A1�̵������ָ��?���ֶ�ģʽ����Ч

												JiZu[2].Slave_D.Realys_Out = Buffer_Data16;
												ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);

											break;
									case 134:// A2����ʱ����?
												if(Buffer_Data16 < 60 &&Buffer_Data16 >=30)
													{
														JiZu[2].Slave_D.DianHuo_Value = Buffer_Data16;
														ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
													}

											break;
									case 135:// A2����ʱ����?
												if(Buffer_Data16 >=30 && Buffer_Data16 <= 100)
													{
														JiZu[2].Slave_D.Max_Power = Buffer_Data16;
														ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
													}

											break;
									case 136:// A2�����¶ȱ���ֵ
												if(Buffer_Data16 < 350)
													{
														JiZu[2].Slave_D.Inside_WenDu_Protect = Buffer_Data16;
														ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
													}

											break;

									//*****************A3333333������صĿ���ָ��?*********************8		
									case 141:// A3���ʵ������ָ��?���ֶ�ģʽ����Ч
									
												SlaveG[3].Out_Power = Buffer_Data16;
												JiZu[3].Slave_D.Power = Buffer_Data16;
												ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
									
											break;

									case 148:// A3Ҫ�������۵�ָ��
																					
												JiZu[3].Slave_D.PaiwuFa_State = Buffer_Data16;
												SlaveG[3].Paiwu_Flag = Buffer_Data16;
												ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
									
											break;
									case 153:// A3�̵������ָ��?���ֶ�ģʽ����Ч

												JiZu[3].Slave_D.Realys_Out = Buffer_Data16;
												ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);

											break;
									
									case 154:// A3����ʱ����?
												if(Buffer_Data16 < 60 &&Buffer_Data16 >=30)
													{
														JiZu[3].Slave_D.DianHuo_Value = Buffer_Data16;
														ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
													}

											break;
									case 155:// A3����ʱ����?
												if(Buffer_Data16 >=30 && Buffer_Data16 <= 100)
													{
														JiZu[3].Slave_D.Max_Power = Buffer_Data16;
														ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
													}

											break;
									case 156:// A3�����¶ȱ���ֵ
												if(Buffer_Data16 < 350)
													{
														JiZu[3].Slave_D.Inside_WenDu_Protect = Buffer_Data16;
														ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
													}

											break;
									
									//*****************A4444444������صĿ���ָ��?*********************8		
									case 161:// A4���ʵ������ָ��?���ֶ�ģʽ����Ч
									
												SlaveG[4].Out_Power = Buffer_Data16;
												JiZu[4].Slave_D.Power = Buffer_Data16;
												ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
									
											break;

									case 168:// A4Ҫ�������۵�ָ��
																					
												JiZu[4].Slave_D.PaiwuFa_State = Buffer_Data16;
												SlaveG[4].Paiwu_Flag = Buffer_Data16;
												ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
									
											break;
									case 173:// A4�̵������ָ��?���ֶ�ģʽ����Ч

												JiZu[4].Slave_D.Realys_Out = Buffer_Data16;
												ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);

											break;
									
									case 174:// A3����ʱ����?
												if(Buffer_Data16 < 60 &&Buffer_Data16 >=30)
													{
														JiZu[4].Slave_D.DianHuo_Value = Buffer_Data16;
														ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
													}

											break;
									case 175:// A3����ʱ����?
												if(Buffer_Data16 >=30 && Buffer_Data16 <= 100)
													{
														JiZu[4].Slave_D.Max_Power = Buffer_Data16;
														ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
													}

											break;
									case 176:// A3�����¶ȱ���ֵ
												if(Buffer_Data16 < 350)
													{
														JiZu[4].Slave_D.Inside_WenDu_Protect = Buffer_Data16;
														ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
													}

											break;
									//*****************A5555555������صĿ���ָ��?*********************8		
									case 181:// A5���ʵ������ָ��?���ֶ�ģʽ����Ч
									
												SlaveG[5].Out_Power = Buffer_Data16;
												JiZu[5].Slave_D.Power = Buffer_Data16;
												ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
									
											break;

									case 188:// A5Ҫ�������۵�ָ��
																					
												JiZu[5].Slave_D.PaiwuFa_State = Buffer_Data16;
												SlaveG[5].Paiwu_Flag = Buffer_Data16;
												ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
									
											break;
									case 193:// A5�̵������ָ��?���ֶ�ģʽ����Ч

												JiZu[5].Slave_D.Realys_Out = Buffer_Data16;
												ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);

											break;
									
									case 194:// A5����ʱ����?
												if(Buffer_Data16 < 60 &&Buffer_Data16 >=30)
													{
														JiZu[5].Slave_D.DianHuo_Value = Buffer_Data16;
														ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
													}

											break;
									case 195:// A5����ʱ����?
												if(Buffer_Data16 >=30 && Buffer_Data16 <= 100)
													{
														JiZu[5].Slave_D.Max_Power = Buffer_Data16;
														ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
													}

											break;
									case 196:// A5�����¶ȱ���ֵ
												if(Buffer_Data16 < 350)
													{
														JiZu[5].Slave_D.Inside_WenDu_Protect = Buffer_Data16;
														ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
													}

											break;
									//*****************A6666666������صĿ���ָ��?*********************8		
									case 201:// A6���ʵ������ָ��?���ֶ�ģʽ����Ч
									
												SlaveG[6].Out_Power = Buffer_Data16;
												JiZu[6].Slave_D.Power = Buffer_Data16;
												ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
									
											break;

									case 208:// A6Ҫ�������۵�ָ��
																					
												JiZu[6].Slave_D.PaiwuFa_State = Buffer_Data16;
												SlaveG[6].Paiwu_Flag = Buffer_Data16;
												ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
									
											break;
									case 213:// A6�̵������ָ��?���ֶ�ģʽ����Ч

												JiZu[6].Slave_D.Realys_Out = Buffer_Data16;
												ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);

											break;
									
									case 214:// A6����ʱ����?
												if(Buffer_Data16 < 60 &&Buffer_Data16 >=30)
													{
														JiZu[6].Slave_D.DianHuo_Value = Buffer_Data16;
														ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
													}

											break;
									case 215:// A6����ʱ����?
												if(Buffer_Data16 >=30 && Buffer_Data16 <= 100)
													{
														JiZu[6].Slave_D.Max_Power = Buffer_Data16;
														ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
													}

											break;
									case 216:// A6�����¶ȱ���ֵ
												if(Buffer_Data16 < 350)
													{
														JiZu[6].Slave_D.Inside_WenDu_Protect = Buffer_Data16;
														ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
													}

											break;

											

									default:

											break;

									}
							}

							
						
						
					}
					
				 
				
			/* 清空接收缓冲 */
				for( i = 0; i < 200;i++ )
					U2_Inf.RX_Data[i] = 0x00;
			
#ifdef USE_NEW_UART_DRIVER
			/* 新驱�?: 清除接收完成标志 */
				uartClearRxFlag(&uartDisplayHandle);
#else
			/* 旧驱�?: 重新开�?�?�? */
				USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
#endif
				
			}

		
}


void ModBus2LCD_Communication(void)
{
		
		
		
}

uint8 ModuBus2LCD_Write0x10Response(uint16 address,uint16 Data16)
{
	uint16 check_sum = 0;
	
	U2_Inf.TX_Data[0] = 0x01;
	U2_Inf.TX_Data[1]= 0x10;

	
	U2_Inf.TX_Data[2] = address >> 8;    // ��ַ���ֽ�
	U2_Inf.TX_Data[3] = address & 0x00FF;  //��ַ���ֽ�
	

	U2_Inf.TX_Data[4] = Data16 >> 8;  //���ݸ��ֽ�
	U2_Inf.TX_Data[5] = Data16 & 0x00ff;   //���ݵ��ֽ�

	check_sum  = ModBusCRC16(U2_Inf.TX_Data,8);   //����������ֽ����ı�?
	U2_Inf.TX_Data[6]  = check_sum >> 8 ;
	U2_Inf.TX_Data[7]  = check_sum & 0x00FF;

	Usart_SendStr_length(USART2,U2_Inf.TX_Data,8);

	return 0;
}

uint8 ModuBus2LCD_WriteAdress0x0000Response(uint16 Buffer_Data16)
{

		switch (Sys_Admin.Device_Style)
			{
				case 0:
				case 1:  //����ģʽ
						
						if(Buffer_Data16 == 1) 
							{
								if(sys_data.Data_10H == 0)
									sys_start_cmd();
								 
							}
						if(Buffer_Data16 == 0) 
							{
								if(sys_data.Data_10H == 2)
									sys_close_cmd();
			
								if(sys_data.Data_10H == 3)
									{
										sys_data.Data_10H = 0;
										GetOut_Mannual_Function(); 
									}
								 
							}
						if(Buffer_Data16 == 3) 
							{
								if(sys_data.Data_10H == 0)
									{
										sys_data.Data_10H = 3 ;
										GetOut_Mannual_Function();
									}
							}
			
						break;
				case 2: //˫ƴģʽ   Ҫ���� �ֶ�ģʽ�� �������ص�����
				case 3:
						if(Buffer_Data16 == 1) 
							{
								//��������
								UnionD.UnionStartFlag = OK;
								
								//if(sys_data.Data_10H == 0)
								//	sys_start_cmd();
								//if(LCD10JZ[2].DLCD.Device_State == 1)
								//	{
								//		SlaveG[2].Startclose_Sendflag = 3;
								//		SlaveG[2].Startclose_Data = Buffer_Data16;
								//	}
								 
							}
						if(Buffer_Data16 == 0) 
							{
								if(sys_data.Data_10H == 2)
									sys_close_cmd();

								
								//�ر�����
								UnionD.UnionStartFlag = FALSE;
								
								SlaveG[2].Startclose_Sendflag = 3;
								SlaveG[2].Startclose_Data = Buffer_Data16;
									
								
								if(sys_data.Data_10H == 3)
									{
										sys_data.Data_10H = 0;
										GetOut_Mannual_Function(); 
									}
							}
					

						if(Buffer_Data16 == 3) 
							{
								UnionD.UnionStartFlag  = 3;
								
								if(sys_data.Data_10H == 0)
									{
										sys_data.Data_10H = 3 ;
										GetOut_Mannual_Function();
									}
								SlaveG[2].Startclose_Sendflag = 3;
								SlaveG[2].Startclose_Data = Buffer_Data16;
							}
			
						break;
				default:
			
						break;
			}


	return 0;
}



uint8 Jizu_ReadResponse(uint8 address)
{
	uint8 Bytes = 0;
	uint8 index = 0;
	uint16 checksum = 0;
	Bytes = sizeof(JiZu[address].Datas);
	U2_Inf.TX_Data[0] = 0x01;
	U2_Inf.TX_Data[1]= 0x03;
	U2_Inf.TX_Data[2] = Bytes; //�������ݳ��ȸı�
	for(index = 3; index < (Bytes + 3); index ++)
		U2_Inf.TX_Data[index] = JiZu[address].Datas[index -3]; //�������ַ����?
		
	checksum  = ModBusCRC16(U2_Inf.TX_Data,Bytes + 5);
	U2_Inf.TX_Data[Bytes + 3] = checksum >> 8;
	U2_Inf.TX_Data[Bytes + 4] = checksum & 0x00FF;
	Usart_SendStr_length(USART2,U2_Inf.TX_Data,Bytes +5);

		return 0;
}



uint8 LCD4013_MmodBus2_Communicastion( )
{
	uint8 LCD4013_Address = 2;  //��4013�ĵ�ַ
	uint8 Bytes = 0;
	uint8 index = 0;
	uint16 checksum = 0;
	
	uint8 Cmd_Data = 0;
	uint16 Data_Address = 0;
	uint16 Buffer_Data16 = 0;
	uint16 Data_Length = 0;
	Cmd_Data  = U2_Inf.RX_Data[1];

	switch (Cmd_Data)
		{
			case 0x03:
				Data_Address = U2_Inf.RX_Data[2] * 256 + U2_Inf.RX_Data[3];
				Buffer_Data16 = U2_Inf.RX_Data[4] * 256 + U2_Inf.RX_Data[5];
				switch (Data_Address)
					{
					case 0x0000:
							Bytes = sizeof(LCD4013X.Datas);
							U2_Inf.TX_Data[0] = LCD4013_Address;
							U2_Inf.TX_Data[1]= 0x03;
				 			U2_Inf.TX_Data[2] = Bytes; //�������ݳ��ȸı�,���ܳ���200
				 			
						
				 			for(index = 3; index < (Bytes + 3); index ++)
								U2_Inf.TX_Data[index] = LCD4013X.Datas[index -3];
							
						
				 			checksum  = ModBusCRC16(U2_Inf.TX_Data,Bytes + 5);
							U2_Inf.TX_Data[Bytes + 3] = checksum >> 8;
							U2_Inf.TX_Data[Bytes + 4] = checksum & 0x00FF;
							
				 			Usart_SendStr_length(USART2,U2_Inf.TX_Data,Bytes +5);

							break;
					default:
							break;
								
								
					}

				
				break;
			case 0x10:
					Data_Address = U2_Inf.RX_Data[2] * 256 + U2_Inf.RX_Data[3];
					Data_Length = U2_Inf.RX_Data[4] * 256 + U2_Inf.RX_Data[5];
					Buffer_Data16 = U2_Inf.RX_Data[7]  + U2_Inf.RX_Data[8] * 256;
					switch (Data_Address)
						{
						case 0x0000:
									
									switch (Buffer_Data16)
										{
										case 0:
												if(sys_data.Data_10H == 2)
													{
														sys_close_cmd();
														LCD4013X.DLCD.Relays_Out = 0;
														LCD4013X.DLCD.Start_Close_Cmd = 0;
													}
												if(sys_data.Data_10H == 3)
													{
														//�˳��ֶ�ģʽ
														sys_data.Data_10H = 0;
														LCD4013X.DLCD.Relays_Out = 0;
														GetOut_Mannual_Function();
														LCD4013X.DLCD.Start_Close_Cmd = 0;
													}
												ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);

												break;
										case 1: //��������
												if(sys_data.Data_10H == 0)
													{
														sys_start_cmd();
														LCD4013X.DLCD.Start_Close_Cmd = 2;
													}
												ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);

												break;
										case 3://�ֶ�ģʽ
												//ֻ���ڴ�������½����ֶ�?
												if(sys_data.Data_10H == 0)
													{
														LCD4013X.DLCD.Relays_Out = 0;
														GetOut_Mannual_Function();
														sys_data.Data_10H = 3;
														LCD4013X.DLCD.Start_Close_Cmd = 3;
													}
												ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);

												break;
										default:
												break;
										}
									 

									break;
						case 0x0003: //���ϸ�λ
									if(Buffer_Data16)
										{
											LCD4013X.DLCD.Error_Code = 0;
											sys_flag.Error_Code = 0; //���ϸ�λ 
											 
											ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
										}

									break;

						case 0x0004:  //�ֶ��������?
									if(Buffer_Data16 <= 100)
										{
											LCD4013X.DLCD.Air_Power = Buffer_Data16;
											PWM_Adjust(Buffer_Data16);
											 
											ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
										}

									break;
						case 0x000E: //�����?
									if(Buffer_Data16 < 60 &&Buffer_Data16 >=30)
										{
											LCD4013X.DLCD.Dian_Huo_Power = Buffer_Data16;
											Sys_Admin.Dian_Huo_Power = Buffer_Data16;
											Write_Admin_Flash();
											ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
										}

									break;
						case 0x000F: //�����?
									if(Buffer_Data16 <= 100 &&Buffer_Data16 >=30)
										{
											LCD4013X.DLCD.Max_Work_Power = Buffer_Data16;
											Sys_Admin.Max_Work_Power = Buffer_Data16;
											Write_Admin_Flash();
											ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
										}

									break;
						case 0x0012: //�����¶ȱ���ֵ
									if(Buffer_Data16 <= 350 &&Buffer_Data16 >=200)
										{
											LCD4013X.DLCD.Inside_WenDu_ProtectValue = Buffer_Data16;
											Sys_Admin.Inside_WenDu_ProtectValue = Buffer_Data16;
											Write_Admin_Flash();
											ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
										}

									break;
						case 0x0016: //�ֶ�ģʽ�£��̵����������?
									
									//���ֶ�ģʽ�£����ݱ�־λ����Ӧ�ļ̵���
										LCD4013X.DLCD.Relays_Out = 	Buffer_Data16;										
										if(Buffer_Data16 & 0x0001)	 //�����־�?
											{
												
												Send_Air_Open();
												
											}
										else
											{
												Send_Air_Close();
												 
											}

										if(Buffer_Data16 & 0x0002)	 //ˮ�ã���ˮ����־λ
											{
												Feed_Main_Pump_ON();
												Second_Water_Valve_Open();
											}
										else
											{
												 Feed_Main_Pump_OFF();
												 Second_Water_Valve_Close();
											}

										if(Buffer_Data16 & 0x0004)	 //��ˮ��
											{
												Pai_Wu_Door_Open();
											}
										else
											{
												Pai_Wu_Door_Close();
											}
										if(Buffer_Data16 & 0x0008)	 //����
											{
												WTS_Gas_One_Open();
											}
										else
											{
												WTS_Gas_One_Close();
											}

									break;
						case 0x0020: //�����ַ����?
									if(Buffer_Data16 <= 6 &&Buffer_Data16 >=1)
										{
											LCD4013X.DLCD.Address = Buffer_Data16;
											Sys_Admin.ModBus_Address = Buffer_Data16;
											Write_Admin_Flash();
											ModuBus2LCD_Write0x10Response(Data_Address,Buffer_Data16);
										}
									break;
						case 0x002A:
							
							if(Data_Length == 0x02)
								{
									Float_Int.byte4.data_LL = U2_Inf.RX_Data[7];
									Float_Int.byte4.data_LH = U2_Inf.RX_Data[8];
									Float_Int.byte4.data_HL = U2_Inf.RX_Data[9];
									Float_Int.byte4.data_HH = U2_Inf.RX_Data[10];
									LCD4013X.DLCD.Target_Pressure = Float_Int.value;
									
									Buffer_Data16 = LCD4013X.DLCD.Target_Pressure * 100;
									
									if(Buffer_Data16 >= 10 && Buffer_Data16 <=	Sys_Admin.DeviceMaxPressureSet)
										{
											sys_config_data.zhuan_huan_temperture_value = Buffer_Data16;
											LCD4013X.DLCD.Target_Pressure = Float_Int.value;
											 if(sys_config_data.zhuan_huan_temperture_value > sys_config_data.Auto_stop_pressure)
												{
													sys_config_data.Auto_stop_pressure = sys_config_data.zhuan_huan_temperture_value + 10;
													if(sys_config_data.Auto_stop_pressure >= Sys_Admin.DeviceMaxPressureSet )
														sys_config_data.Auto_stop_pressure = Sys_Admin.DeviceMaxPressureSet - 1;

													LCD4013X.DLCD.Stop_Pressure = (float) sys_config_data.Auto_stop_pressure / 100;
												}
										
											  Write_Internal_Flash();
											 sys_config_data.zhuan_huan_temperture_value = *(uint32 *)(ZHUAN_HUAN_TEMPERATURE);
										}
									
									
									ModuBus2LCD_Write0x10Response(Data_Address,Data_Length);	
								}

							break;
						case 0x002C:  //ֹͣѹ�� ������
									if(Data_Length == 0x02)
										{
											Float_Int.byte4.data_LL = U2_Inf.RX_Data[7];
											Float_Int.byte4.data_LH = U2_Inf.RX_Data[8];
											Float_Int.byte4.data_HL = U2_Inf.RX_Data[9];
											Float_Int.byte4.data_HH = U2_Inf.RX_Data[10];
											LCD4013X.DLCD.Stop_Pressure = Float_Int.value;
											Buffer_Data16 = LCD4013X.DLCD.Stop_Pressure * 100;

											if(Buffer_Data16 >= sys_config_data.zhuan_huan_temperture_value && Buffer_Data16 <  Sys_Admin.DeviceMaxPressureSet)
												{
													sys_config_data.Auto_stop_pressure = Buffer_Data16;
													Write_Internal_Flash();
													sys_config_data.Auto_stop_pressure = *(uint32 *)(AUTO_STOP_PRESSURE_ADDRESS);
												}
											
											ModuBus2LCD_Write0x10Response(Data_Address,Data_Length);	
										}

									break;
						case 0x002E:  //����ѹ�� ������
									if(Data_Length == 0x02)
										{
											Float_Int.byte4.data_LL = U2_Inf.RX_Data[7];
											Float_Int.byte4.data_LH = U2_Inf.RX_Data[8];
											Float_Int.byte4.data_HL = U2_Inf.RX_Data[9];
											Float_Int.byte4.data_HH = U2_Inf.RX_Data[10];
											LCD4013X.DLCD.Start_Pressure = Float_Int.value;
											Buffer_Data16 = LCD4013X.DLCD.Start_Pressure * 100;
											if( Buffer_Data16 <  sys_config_data.Auto_stop_pressure)
												{
													sys_config_data.Auto_start_pressure = Buffer_Data16;
													LCD4013X.DLCD.Start_Pressure = Float_Int.value;
													Write_Internal_Flash();
												}
											ModuBus2LCD_Write0x10Response(Data_Address,Data_Length);	
										}
									break;
						case 0x0030:  //�ѹ����������
										if(Data_Length == 0x02)
											{
												Float_Int.byte4.data_LL = U2_Inf.RX_Data[7];
												Float_Int.byte4.data_LH = U2_Inf.RX_Data[8];
												Float_Int.byte4.data_HL = U2_Inf.RX_Data[9];
												Float_Int.byte4.data_HH = U2_Inf.RX_Data[10];
												LCD4013X.DLCD.Max_Pressure = Float_Int.value;
												Buffer_Data16 = LCD4013X.DLCD.Max_Pressure * 100;
												
												if( Buffer_Data16 <= 250) //���ѹ��С��2.5Mpa
													{
														Sys_Admin.DeviceMaxPressureSet = Buffer_Data16;
														Write_Admin_Flash();
													}
												ModuBus2LCD_Write0x10Response(Data_Address,Data_Length);	
											}

										break;
							
							default:

									break;
						}
				break;
			default:

					break;
		}


	return 0;
}

uint8  ModBus2LCD4013_Lcd7013_Communication(void)
{
	uint8 Index = 0;
	uint8 Modbus_Address = 0;
	uint16 checksum = 0;
#ifdef USE_NEW_UART_DRIVER
	uint8_t* rxData;
	uint16_t rxLen;
#endif

	LCD4013_Data_Check_Function();

#ifdef USE_NEW_UART_DRIVER
	/* 新驱�?: 检�?DMA接收完成标志 */
	if (uartIsRxReady(&uartDisplayHandle))
	{
		if (uartGetRxData(&uartDisplayHandle, &rxData, &rxLen) == UART_OK)
		{
			for (Index = 0; Index < rxLen && Index < 300; Index++)
				U2_Inf.RX_Data[Index] = rxData[Index];
			U2_Inf.RX_Length = rxLen;
			U2_Inf.Recive_Ok_Flag = 1;
		}
	}
#endif

	if(U2_Inf.Recive_Ok_Flag)
	{
		U2_Inf.Recive_Ok_Flag = 0;
#ifndef USE_NEW_UART_DRIVER
		/* 旧驱�?: 关闭�?�? */
		USART_ITConfig(USART2, USART_IT_RXNE, DISABLE);
#endif
		 
		checksum = U2_Inf.RX_Data[U2_Inf.RX_Length - 2] * 256 + U2_Inf.RX_Data[U2_Inf.RX_Length - 1];
			
		if(checksum == ModBusCRC16(U2_Inf.RX_Data, U2_Inf.RX_Length))
		{
			Modbus_Address = U2_Inf.RX_Data[0];
			if(Modbus_Address == 2)
			{	
				sys_flag.Lcd4013_OnLive_Flag = OK;
				LCD4013_MmodBus2_Communicastion();
			}
		}

		/* 清空接收缓冲 */
		for(Index = 0; Index < 200; Index++)
			U2_Inf.RX_Data[Index] = 0x00;
		
#ifdef USE_NEW_UART_DRIVER
		/* 新驱�?: 清除接收完成标志 */
		uartClearRxFlag(&uartDisplayHandle);
#else
		/* 旧驱�?: 重新开�?�?�? */
		USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
#endif
	}

	return 0;
}



uint8  LCD4013_Data_Check_Function(void)
{
	float ResData = 0.0;


	LCD4013X.DLCD.Soft_Version = Soft_Version;  //�����汾


	if(sys_data.Data_10H == 0)
		{
			LCD4013X.DLCD.Start_Close_Cmd = 0;
		}
	else
		{
			LCD4013X.DLCD.Start_Close_Cmd = 1;
		}
	

	LCD4013X.DLCD.Address = Sys_Admin.ModBus_Address;

	sys_flag.Address_Number = LCD4013X.DLCD.Address;
	
	LCD4013X.DLCD.Device_State = sys_data.Data_10H;
	//������־�ڴ���4������Ƿ��յ��������?

	LCD4013X.DLCD.Error_Code = sys_flag.Error_Code;

	LCD4013X.DLCD.Air_Power = sys_data.Data_1FH;
	LCD4013X.DLCD.Flame_State = sys_flag.flame_state;
	LCD4013X.DLCD.Pump_State = Switch_Inf.Water_Valve_Flag;
	LCD4013X.DLCD.Water_State = LCD10D.DLCD.Water_State;
	LCD4013X.DLCD.Paiwu_State = Switch_Inf.pai_wu_flag;
	LCD4013X.DLCD.Air_State = Switch_Inf.air_on_flag;
	LCD4013X.DLCD.Air_Speed = sys_flag.Fan_Rpm;
	LCD4013X.DLCD.Dian_Huo_Power = Sys_Admin.Dian_Huo_Power;
	LCD4013X.DLCD.Max_Work_Power = Sys_Admin.Max_Work_Power;
	LCD4013X.DLCD.Inside_WenDu_ProtectValue = Sys_Admin.Inside_WenDu_ProtectValue;
	LCD4013X.DLCD.Inside_WenDu = sys_flag.Protect_WenDu;

	

	
	ResData = Temperature_Data.Pressure_Value;
	LCD4013X.DLCD.Steam_Pressure = ResData / 100; //��ǰ������ѹ��

	ResData = sys_config_data.zhuan_huan_temperture_value;
	LCD4013X.DLCD.Target_Pressure = ResData / 100; //Ŀ��ѹ��

	ResData = sys_config_data.Auto_stop_pressure;
	LCD4013X.DLCD.Stop_Pressure = ResData / 100; //ͣ��ѹ��

	ResData = sys_config_data.Auto_start_pressure;
	LCD4013X.DLCD.Start_Pressure = ResData / 100; //����ѹ��

	ResData = Sys_Admin.DeviceMaxPressureSet;
	LCD4013X.DLCD.Max_Pressure = ResData / 100; //�����ѹ��

	return 0 ;
}


