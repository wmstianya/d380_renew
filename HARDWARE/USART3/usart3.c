#include "usart3.h"
#include "stdarg.h"
#include "uart_driver.h"

/* 启用新UART驱动 */
//#define USE_NEW_UART_DRIVER

  



  	 
///////////////////////////////////////USART3 printf֧�ֲ���//////////////////////////////////
//����2,u2_printf ����
//ȷ��һ�η������ݲ�����USART3_MAX_SEND_LEN�ֽ�
////////////////////////////////////////////////////////////////////////////////////////////////
void u3_printf(char* fmt,...)  
{  
  
}
///////////////////////////////////////USART2 ��ʼ�����ò���//////////////////////////////////	    
//���ܣ���ʼ��IO ����2
//�������
//bound:������
//�������
//��
//////////////////////////////////////////////////////////////////////////////////////////////	  
void uart3_init(u32 bound)
{  	 		 
	//GPIO�˿�����
    GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	 
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);	//ʹ��USART3
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	//ʹ��GPIOBʱ��
	USART_DeInit(USART3);  //��λ����3

     //USART3_TX   PB.10
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10; //PB.10
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//�����������
    GPIO_Init(GPIOB, &GPIO_InitStructure);
   
    //USART3_RX	  PB.11
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//��������
    GPIO_Init(GPIOB, &GPIO_InitStructure);  

    //Usart3 NVIC ����
    NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority= 0;//��ռ���ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;		//�����ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	        //����ָ���Ĳ�����ʼ��VIC�Ĵ���
  
    //USART3 ��ʼ������
	USART_InitStructure.USART_BaudRate = bound;   //һ������Ϊ9600;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;  //�ֳ�Ϊ8λ���ݸ�ʽ
	USART_InitStructure.USART_StopBits = USART_StopBits_1;  //һ��ֹͣλ
	USART_InitStructure.USART_Parity = USART_Parity_No;  //����żУ��λ
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;  //��Ӳ������������
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	  //�շ�ģʽ

    USART_Init(USART3, &USART_InitStructure);   //��ʼ������
    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);  //�����ж�
    USART_Cmd(USART3, ENABLE);                      //ʹ�ܴ��� 
	
//	USART_DMACmd(USART2,USART_DMAReq_Tx,ENABLE);    //ʹ�ܴ���2��DMA����
//	UART_DMA_Config(DMA1_Channel7,(u32)&USART2->DR,(u32)USART2_TX_BUF,1000);  //DMA1ͨ��7,����Ϊ����2,�洢��ΪUSART2_TX_BUF,����1000. 										  	
}




uint8 FuNiSen_Read_WaterDevice_Function(void)
{
	static uint8  Jump_Index = 0;
	static uint8 Jump_Count = 0;
	uint8 Address = 8; //Ĭ�ϱ����ڵ�ַΪ 8 �����ڽ�ˮ���ڷ�
	uint16 check_sum = 0;
	uint16 Percent = 0;


	//200ms����һ��
	if(sys_flag.WaterAJ_Flag == 0)
		return 0 ;

	sys_flag.WaterAJ_Flag = 0;
	sys_flag.waterSend_Flag = OK;

	if(sys_flag.Water_Percent <= 100)
		Percent =  sys_flag.Water_Percent * 10;
	else
		Percent = 0 ; //ֵ������ر�

	Jump_Count ++;
	if(Jump_Count > 5)
		{
			Jump_Index = 0;
			Jump_Count = 0;
		}
	else
		Jump_Index = 1;
		
	
	switch (Jump_Index)
			{
			case 0: //��ȡʵʱ�Ŀ�����ֵ
					U3_Inf.TX_Data[0] = Address;
					U3_Inf.TX_Data[1] = 0x03;// 
					
					U3_Inf.TX_Data[2] = 0x00;
					U3_Inf.TX_Data[3] = 0x00;//��ַ

					U3_Inf.TX_Data[4] = 0x00;
					U3_Inf.TX_Data[5] = 0x01;//��ȡ���ݸ���
				 
					check_sum  = ModBusCRC16(U3_Inf.TX_Data,8);
					U3_Inf.TX_Data[6]  = check_sum >> 8 ;
					U3_Inf.TX_Data[7]  = check_sum & 0x00FF;
					
#ifdef USE_NEW_UART_DRIVER
					uartSendDma(&uartSlaveHandle, U3_Inf.TX_Data, 8);
#else
					Usart_SendStr_length(USART3,U3_Inf.TX_Data,8);
#endif
				   
					//Jump_Index = 1;
					break;
			case 1://д�뿪�ȵ�ֵ
					//Jump_Index = 0;

					U3_Inf.TX_Data[0] = Address;
					U3_Inf.TX_Data[1] = 0x06;// 
					
					U3_Inf.TX_Data[2] = 0x00;
					U3_Inf.TX_Data[3] = 0x01;//��ַ

					U3_Inf.TX_Data[4] = Percent >> 8;
					U3_Inf.TX_Data[5] = Percent & 0x00FF;//д��Ŀ���ֵ
				 
					check_sum  = ModBusCRC16(U3_Inf.TX_Data,8);
					U3_Inf.TX_Data[6]  = check_sum >> 8 ;
					U3_Inf.TX_Data[7]  = check_sum & 0x00FF;			
#ifdef USE_NEW_UART_DRIVER
					uartSendDma(&uartSlaveHandle, U3_Inf.TX_Data, 8);
#else
					Usart_SendStr_length(USART3,U3_Inf.TX_Data,8);
#endif
					break;
			default:
					Jump_Index = 0;
					break;
			}
	
	return 0;
}

uint8 FuNiSen_RecData_WaterDevice_Processs(void)
{
	uint16 checksum = 0;
	uint8 address = 0;
	uint8 i = 0;
	uint8 command = 0;
	uint8 Length = 0;
	uint16 Value_Buffer = 0;

	if(sys_flag.waterSend_Count >= 10)
	   sys_flag.Error_Code = Error19_SupplyWater_UNtalk;
	
	if(U3_Inf.Recive_Ok_Flag)
		{
			U3_Inf.Recive_Ok_Flag = 0;//������Ŷ
			 //�ر��ж�
			USART_ITConfig(USART3, USART_IT_RXNE, DISABLE); 
			 
			checksum  = U3_Inf.RX_Data[U3_Inf.RX_Length - 2] * 256 + U3_Inf.RX_Data[U3_Inf.RX_Length - 1];

			address = U3_Inf.RX_Data[0]; 
			command = U3_Inf.RX_Data[1];
			Length = U3_Inf.RX_Data[2];
		
			//01 03 06 07 CF 07 CF 00 00 45 99
			//01 06 00 02 07 CF 6A 6E 
			if(address == 0x08 &&checksum == ModBusCRC16(U3_Inf.RX_Data,U3_Inf.RX_Length) )
				{
					 sys_flag.waterSend_Flag = FALSE;
					 sys_flag.waterSend_Count = 0;
					if(command == 0x03 && Length == 0x02)
						{
						   //��ȡ��ǰ����״̬
						  
						   
							Value_Buffer = U3_Inf.RX_Data[3] * 256 + U3_Inf.RX_Data[4];
							if(Value_Buffer == 0xEA)
								sys_flag.Water_Error_Code = OK; //����״̬
							else
							   sys_flag.Water_Error_Code = 0;

							if(sys_flag.Water_Error_Code == OK)
							   sys_flag.Error_Code = Error18_SupplyWater_Error;
							
						}
					
				}
				

		//�Խ��ջ���������
			for( i = 0; i < 100;i++ )
				U3_Inf.RX_Data[i] = 0x00;
		
		//���¿����ж�
			USART_ITConfig(USART3, USART_IT_RXNE, ENABLE); 
			
		}
	
	return 0;	
}



void ModBus_Uart3_LocalRX_Communication(void)
{
	uint8  i = 0;	
	uint8  Target_Address = 0;
	uint8 command = 0;
	uint8 Length = 0;
	uint16 Value_Buffer = 0;
	uint16 checksum = 0;
#ifdef USE_NEW_UART_DRIVER
	uint8_t* rxData;
	uint16_t rxLen;
#endif

	if(Sys_Admin.Water_BianPin_Enabled)
	{
		if(sys_flag.waterSend_Count >= 10)
			sys_flag.Error_Code = Error19_SupplyWater_UNtalk;
	}
	else
	{
		sys_flag.waterSend_Count = 0;	
	}

#ifdef USE_NEW_UART_DRIVER
	/* 新驱动: 检查DMA接收完成标志 */
	if (uartIsRxReady(&uartSlaveHandle))
	{
		if (uartGetRxData(&uartSlaveHandle, &rxData, &rxLen) == UART_OK)
		{
			for (i = 0; i < rxLen && i < 300; i++)
				U3_Inf.RX_Data[i] = rxData[i];
			U3_Inf.RX_Length = rxLen;
			U3_Inf.Recive_Ok_Flag = 1;
		}
	}
#endif

	if(U3_Inf.Recive_Ok_Flag)
	{
		U3_Inf.Recive_Ok_Flag = 0;
#ifndef USE_NEW_UART_DRIVER
		/* 旧驱动: 关闭中断 */
		USART_ITConfig(USART3, USART_IT_RXNE, DISABLE);
#endif
		 
		checksum = U3_Inf.RX_Data[U3_Inf.RX_Length - 2] * 256 + U3_Inf.RX_Data[U3_Inf.RX_Length - 1];
		command = U3_Inf.RX_Data[1];
		Length = U3_Inf.RX_Data[2];
		
		if(checksum == ModBusCRC16(U3_Inf.RX_Data, U3_Inf.RX_Length))
		{
			Target_Address = U3_Inf.RX_Data[0];

			if(Target_Address == 8) /* 变频供水阀 */
			{
				sys_flag.waterSend_Flag = FALSE;
				sys_flag.waterSend_Count = 0;
				if(command == 0x03 && Length == 0x02)
				{
					/* 获取当前故障状态 */
					Value_Buffer = U3_Inf.RX_Data[3] * 256 + U3_Inf.RX_Data[4];
					if(Value_Buffer == 0xEA)
						sys_flag.Water_Error_Code = OK; /* 故障状态 */
					else
						sys_flag.Water_Error_Code = 0;

					if(sys_flag.Water_Error_Code == OK)
						sys_flag.Error_Code = Error18_SupplyWater_Error;
				}
			}
		}
		
		/* 清空接收缓冲 */
		for(i = 0; i < 200; i++)
			U3_Inf.RX_Data[i] = 0x00;
		
#ifdef USE_NEW_UART_DRIVER
		/* 新驱动: 清除接收完成标志 */
		uartClearRxFlag(&uartSlaveHandle);
#else
		/* 旧驱动: 重新开启中断 */
		USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
#endif
	}
}


uint8 Modbus3_UnionTx_Communication(void)
{
	static uint8 Address = 1;
	//	SlaveG[1].Alive_Flag  
	//	JiZu[1].Slave_D.Device_State  //�����ڸ������ݵ�ͬ��


	if(Sys_Admin.Device_Style == 0 || Sys_Admin.Device_Style == 1)
		{
			switch(Address)
				{
					case 1:
							if(Sys_Admin.Water_BianPin_Enabled)
			 					{
				 					if(sys_flag.Special_100msFlag)
					 					{
					 						FuNiSen_Read_WaterDevice_Function();
											sys_flag.Special_100msFlag = 0;
											Address++;
					 					}
								}
							else
								{
									Address++;
								}

							break;
					case 2:
							if(sys_flag.Special_100msFlag)
								{
									sys_flag.Special_100msFlag = 0;
									Address = 1;
								}

							break;

					default:
							Address = 1;

							break;
				}
		}

	
		
	
		

		return 0;
}

uint8 Modbus3_UnionRx_DataProcess(uint8 Cmd,uint8 address)
{
		

		return 0;
}




uint8 ModBus3_RTU_Read03(uint8 Target_Address,uint16 Data_Address,uint8 Data_Length )
{
		uint16 Check_Sum = 0;
		U3_Inf.TX_Data[0] = Target_Address;
		U3_Inf.TX_Data[1] = 0x03;

		U3_Inf.TX_Data[2]= Data_Address >> 8;
		U3_Inf.TX_Data[3]= Data_Address & 0x00FF;

		
		U3_Inf.TX_Data[4]= Data_Length >> 8;
		U3_Inf.TX_Data[5]= Data_Length & 0x00FF;

		
		Check_Sum = ModBusCRC16(U3_Inf.TX_Data,8);
		U3_Inf.TX_Data[6]= Check_Sum >> 8;
		U3_Inf.TX_Data[7]= Check_Sum & 0x00FF;
		
#ifdef USE_NEW_UART_DRIVER
		uartSendDma(&uartSlaveHandle, U3_Inf.TX_Data, 8);
#else
		Usart_SendStr_length(USART3,U3_Inf.TX_Data,8);
#endif


	return 0;
}

uint8 ModBus3_RTU_Write06(uint8 Target_Address,uint16 Data_Address,uint16 Data16)
{
		uint16 Check_Sum = 0;
		U3_Inf.TX_Data[0] = Target_Address;
		U3_Inf.TX_Data[1] = 0x06;

		U3_Inf.TX_Data[2]= Data_Address >> 8;
		U3_Inf.TX_Data[3]= Data_Address & 0x00FF;

		U3_Inf.TX_Data[4]= Data16 >> 8;
		U3_Inf.TX_Data[5]= Data16 & 0x00FF;

		Check_Sum = ModBusCRC16(U3_Inf.TX_Data,8);
		U3_Inf.TX_Data[6]= Check_Sum >> 8;
		U3_Inf.TX_Data[7]= Check_Sum & 0x00FF;

#ifdef USE_NEW_UART_DRIVER
		uartSendDma(&uartSlaveHandle, U3_Inf.TX_Data, 8);
#else
		Usart_SendStr_length(USART3,U3_Inf.TX_Data,8);
#endif

		

		return 0;
}

uint8 ModBus3_RTU_Write10(uint8 Target_Address,uint16 Data_Address)
{
		

		

		return 0;
}


uint8 Union_MuxJiZu_Control_Function(void)
{
	
	

		return 0;
}



