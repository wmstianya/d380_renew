
	  
#include "main.h"
#include "uart_driver.h"

 



#if 1
#pragma import(__use_no_semihosting)             
////��׼����Ҫ��֧�ֺ���                 
struct __FILE 
{ 
	int handle; 

}; 


FILE __stdout;
// FILE __stdin;
// FILE __stderr;       
//����_sys_exit()�Ա���ʹ�ð�����ģʽ    
_sys_exit(int x) 
{ 
	x = x; 
} 

//_ttywrch(int ch)
//{
//	ch =ch;
//}

//�ض���fputc���� 
int fputc(int ch, FILE *f)
{      
	while((USART1->SR&0X40)==0);//ѭ������,ֱ���������   
    USART1->DR = (u8) ch;      
	return ch;
}




#endif 


 
//����1�жϷ������
//ע��,��ȡUSARTx->SR�ܱ���Ī������Ĵ���   	
u8 USART_RX_BUF[USART_REC_LEN];     //���ջ���,���USART_REC_LEN���ֽ�.
//����״̬
//bit15��	������ɱ�־
//bit14��	���յ�0x0d
//bit13~0��	���յ�����Ч�ֽ���Ŀ
u16 USART_RX_STA=0;       //����״̬���	  


RTU_DATA Rtu_Data ;




//��ʼ��IO ����1 
//bound:������
void uart_init(u32 bound){
    //GPIO�˿�����
    GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	 
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1|RCC_APB2Periph_GPIOA, ENABLE);	//ʹ��USART1��GPIOAʱ��
 	USART_DeInit(USART1);  //��λ����1
	 //USART1_TX   PA.9
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9; //PA.9
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//�����������
    GPIO_Init(GPIOA, &GPIO_InitStructure); //��ʼ��PA9
   
    //USART1_RX	  PA.10
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//��������
    GPIO_Init(GPIOA, &GPIO_InitStructure);  //��ʼ��PA10

   //Usart1 NVIC ����

    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=2 ; 
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;		 
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ���
  
   //USART ��ʼ������

	USART_InitStructure.USART_BaudRate = bound;//һ������Ϊ9600;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//�ֳ�Ϊ8λ���ݸ�ʽ
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//һ��ֹͣλ
	USART_InitStructure.USART_Parity = USART_Parity_No;//����żУ��λ
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//��Ӳ������������
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//�շ�ģʽ

    USART_Init(USART1, &USART_InitStructure); //��ʼ������
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);//�����ж�
    USART_Cmd(USART1, ENABLE);                    //ʹ�ܴ��� 

}

void u1_printf(char* fmt,...)  
{  
  	int len=0;
	int cnt=0;
	va_list ap;
	va_start(ap,fmt);
	vsprintf((char*)U1_Inf.TX_Data,fmt,ap);
	va_end(ap);
	len = strlen((const char*)U1_Inf.TX_Data);
	while(len--)
	  {
	  while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=1); //�ȴ����ͽ���
	  USART_SendData(USART1,U1_Inf.TX_Data[cnt++]);
	  }
}


void Usart_SendByte( USART_TypeDef * pUSARTx, uint8_t ch )
{
	/* ����һ���ֽ����ݵ�USART1 */
	USART_SendData(pUSARTx,ch);
		
	/* �ȴ�������� */
	while (USART_GetFlagStatus(pUSARTx, USART_FLAG_TXE) == RESET);	
}
/*****************  ָ�����ȵķ����ַ��� **********************/
void Usart_SendStr_length( USART_TypeDef * pUSARTx, uint8_t *str,uint32_t strlen )
{
	unsigned int k=0; 
    do 
    {
        Usart_SendByte( pUSARTx, *(str + k) );
        k++;
    } while(k < strlen);
}

/*****************  �����ַ��� **********************/
void Usart_SendString( USART_TypeDef * pUSARTx, uint8_t *str)
{
	unsigned int k=0;
    do 
    {
        Usart_SendByte( pUSARTx, *(str + k) );
        k++;
    } while(*(str + k)!='\0');
}



/* 外部读取内部数据信息 - DMA+IDLE模式 */
void ModBus_Communication(void)
{
		
		uint8  i = 0;	
		
		uint16 checksum = 0;
		uint16 Address = 0;
		uint8_t* rxData;
		uint16_t rxLen;

		/* DMA接收: 检查接收完成标志 */
		if (uartIsRxReady(&uartDebugHandle))
		{
			if (uartGetRxData(&uartDebugHandle, &rxData, &rxLen) == UART_OK)
			{
				/* 复制数据到U1_Inf以兼容后续代码 */
				for (i = 0; i < rxLen && i < 100; i++)
					U1_Inf.RX_Data[i] = rxData[i];
				U1_Inf.RX_Length = rxLen;
				U1_Inf.Recive_Ok_Flag = 1;
			}
		}
		 
		if(U1_Inf.Recive_Ok_Flag)
			{
				U1_Inf.Recive_Ok_Flag = 0;
				 
				checksum  = U1_Inf.RX_Data[U1_Inf.RX_Length - 2] * 256 + U1_Inf.RX_Data[U1_Inf.RX_Length - 1];
				
			//�ϰ�������豸���кŽ����޸�
				if(checksum == ModBusCRC16(U1_Inf.RX_Data,U1_Inf.RX_Length))
					{
						//��ȡ���صĵ�ַ��Ϣ����ʱ�׵�ַΪ254��0x03ָ��   �����ݵ�ַΪ1000��ֻҪ�����������ݣ��ͻ�
						if(U1_Inf.RX_Data[0] == 254 && U1_Inf.RX_Data[1] == 0x03)
							{
								Address = U1_Inf.RX_Data[2] *256+ U1_Inf.RX_Data[3];
								if(Address == 1000)
									{
										U1_Inf.TX_Data[0] = U1_Inf.RX_Data[0];
										U1_Inf.TX_Data[1] = 0x03;// 
										U1_Inf.TX_Data[2] = 2; //���ݳ���Ϊ2�� ��������ı�***

										U1_Inf.TX_Data[3] = 0;
										U1_Inf.TX_Data[4] =  Sys_Admin.ModBus_Address;
										checksum  = ModBusCRC16(U1_Inf.TX_Data,7);   //����������ֽ����ı�
										U1_Inf.TX_Data[5]  = checksum >> 8 ;
										U1_Inf.TX_Data[6]  = checksum & 0x00FF;
										Usart_SendStr_length(USART1,U1_Inf.TX_Data,7);
										
									}
							}
					
						switch (Sys_Admin.Device_Style)
							{
								case 0:
								case 1:
										
								case 2:
								case 3:

										Union_New_Server_Cmd_Analyse();


										break;
								default:
									 

										break;
							}
						
					}
					
			
				
			/* 清空接收缓冲 */
				for( i = 0; i < 100;i++ )
					U1_Inf.RX_Data[i] = 0x00;
			
			/* DMA驱动: 清除接收完成标志 */
				uartClearRxFlag(&uartDebugHandle);
				
			}
}








uint16 ModBusCRC16(unsigned char *ptr,uint8 size)
{
		uint16 a,b,temp;
		uint16 CRC16;
		uint16 checksum ;

		CRC16 = 0XFFFF;	

		for(a = 0; a < (size -2) ; a++ )
			{
				CRC16 = *ptr ^ CRC16;
				for(b = 0;b < 8;b++)
					{
						temp = CRC16 & 0X0001;
						CRC16 = CRC16 >> 1;
						if(temp)
							CRC16 = CRC16 ^ 0XA001;	
					}

				*ptr++;
			}

		checksum = ((CRC16 & 0x00FF) << 8) |((CRC16 & 0XFF00) >> 8);


		return checksum;
}

uint16 Lcd_CRC16(unsigned char *ptr,uint8 size)
{
		uint16 a,b,temp;
		uint16 CRC16;
		uint16 checksum ;

		CRC16 = 0XFFFF;	

		
		for(a = 0; a < size ; a++ )
			{
				CRC16 = *ptr ^ CRC16;
				for(b = 0;b < 8;b++)
					{
						temp = CRC16 & 0X0001;
						CRC16 = CRC16 >> 1;
						if(temp)
							CRC16 = CRC16 ^ 0XA001;	
					}

				*ptr++;
			}

		checksum = ((CRC16 & 0x00FF) << 8) |((CRC16 & 0XFF00) >> 8);


		return checksum;
}



void RTU_Server_Cmd_Analyse(void)
{
   
}

void RTU_Mcu_mail_Wifi(void)
{
   
	

}


uint8 A1B1_Jizu_ReadResponse(uint8 address)
{
	uint8 Bytes = 0;
	uint8 index = 0;
	uint16 checksum = 0;
	Bytes = sizeof(JiZu[address].Datas);
	U1_Inf.TX_Data[0] = Sys_Admin.ModBus_Address;
	U1_Inf.TX_Data[1]= 0x03;
	U1_Inf.TX_Data[2] = Bytes; //�������ݳ��ȸı�


	for(index = 3; index < (Bytes + 3); index = index + 2)
		{
			U1_Inf.TX_Data[index] = JiZu[address].Datas[index + 1 -3]; //�����ߵͶ�ѭ��
			U1_Inf.TX_Data[index + 1] = JiZu[address].Datas[index -3]; 
		}
		
		
	checksum  = ModBusCRC16(U1_Inf.TX_Data,Bytes + 5);
	U1_Inf.TX_Data[Bytes + 3] = checksum >> 8;
	U1_Inf.TX_Data[Bytes + 4] = checksum & 0x00FF;
	Usart_SendStr_length(USART1,U1_Inf.TX_Data,Bytes +5);

		return 0;
}


uint8  Union_New_Server_Cmd_Analyse(void)
{
   uint8 Cmd = 0;
   uint8 index = 0;
   uint8 Length = 0;
   uint16 Value = 0;
   uint16 Address = 0;
   uint16 Data = 0; 
   uint8  Device_ID = 1; //���ݵ�ַ�����ܴ� ************88
   float  value_buffer = 0;
   uint16 check_sum = 0;
   uint16 Data1 = 0;
   uint16 Data2 = 0;
   uint16 Data3 = 0;
   uint32 Data32_Buffer = 0;
   uint32 Int_CharH = 0;
   uint32 Int_CHarL = 0;
   uint8 Code[6] = {0};
   


	//Float_Int.value  ���ڵ����ȵ�ת��

   
   Cmd = U1_Inf.RX_Data[1];
   Address = U1_Inf.RX_Data[2] *256+ U1_Inf.RX_Data[3];
   Length = U1_Inf.RX_Data[4] *256+ U1_Inf.RX_Data[5];
   Value = U1_Inf.RX_Data[4] *256+ U1_Inf.RX_Data[5];	//06��ʽ�£�value = length

    

   if(U1_Inf.RX_Data[0] > 1)
	   {
		   //����ѯ��ַ����1ʱ���Զ��л�Ϊ�Է�ѯ�ʵĵ�ַ
		    
		   Device_ID = Sys_Admin.ModBus_Address;
		
		   if(U1_Inf.RX_Data[0] != Sys_Admin.ModBus_Address)
			   {
				   return 0;   //�Ǳ�����ֱַ���˳�
			   }
	   }
   
   
   switch (Cmd)
	   {
		   case 03:  //00 03 01 F3 00 0F F5 D0	   

				   switch (Address)
					   {
						   case 0:
								   if(Length == 11)  //��ߵ����ݳ���Ҳ�����ܱ仯�������ֽڳ��ȵı仯
									   {
										   U1_Inf.TX_Data[0] = Device_ID;
										   U1_Inf.TX_Data[1] = 0x03;// 
										   U1_Inf.TX_Data[2] = 22; //���ݳ���Ϊ6�� ��������ı�***
								   
										   U1_Inf.TX_Data[3] = 0;
										   U1_Inf.TX_Data[4] = AUnionD.UnionStartFlag;// 1��ǰ������״̬
											Data1 = AUnionD.Big_Pressure *100;
										   U1_Inf.TX_Data[5] = Data1 >> 8 ;
										   U1_Inf.TX_Data[6] = Data1 & 0x00FF;//2��ǰ������ѹ��ֵ��ǰ��ˮλ״̬
										   
										   U1_Inf.TX_Data[7] = 0x00;
										   U1_Inf.TX_Data[8] = AUnionD.AliveOK_Numbers;//3  //�����豸����	

										   U1_Inf.TX_Data[9] = 0x00;
										   U1_Inf.TX_Data[10] =sys_flag.Device_ErrorNumbers ;//4�����豸����

										   U1_Inf.TX_Data[11] = 0x00;
										   U1_Inf.TX_Data[12] = sys_flag.Already_WorkNumbers;//5�������豸����

										   U1_Inf.TX_Data[13] = 0;
										   U1_Inf.TX_Data[14] = 0;//6 �ܿع�����

										   U1_Inf.TX_Data[15] = 0;
										   U1_Inf.TX_Data[16] = 0;//7 ���ϸ�λָ��

										   U1_Inf.TX_Data[17] = 0;
										   U1_Inf.TX_Data[18] = AUnionD.Target_Value *100;//8 //Ŀ��ѹ��

										   U1_Inf.TX_Data[19] = 0;
										   U1_Inf.TX_Data[20] = AUnionD.Stop_Value *100;//9 ͣ��ѹ��

										   U1_Inf.TX_Data[21] = 0;
										   U1_Inf.TX_Data[22] = AUnionD.Start_Value *100;//10 ����ѹ��

										   U1_Inf.TX_Data[23] = 0;
										   U1_Inf.TX_Data[24] = 0;//11  δʹ��
										   

										   
										
										   check_sum  = ModBusCRC16(U1_Inf.TX_Data,27);   //����������ֽ����ı�
										   U1_Inf.TX_Data[25]  = check_sum >> 8 ;
										   U1_Inf.TX_Data[26]  = check_sum & 0x00FF;
										   
										   Usart_SendStr_length(USART1,U1_Inf.TX_Data,27);

										   
										   
									   }

								   break;
							case 0x0063://ȡ����1������ֵ
										A1B1_Jizu_ReadResponse(1);

										break;
							case 0x0077://ȡ����2������ֵ
										A1B1_Jizu_ReadResponse(2);
										break;

							case 0x008B://ȡ����3������ֵ
										//A1B1_Jizu_ReadResponse(3);
										break;

							case 0x009F://ȡ����4������ֵ
										//A1B1_Jizu_ReadResponse(4);
										break;
							case 0x00B3://ȡ����5������ֵ
										//A1B1_Jizu_ReadResponse(5);
										break;
							case 0x00C7://ȡ����6������ֵ
										//A1B1_Jizu_ReadResponse(6);
										break;
							
						   
							   

						   default :
								   break;
					   }
				   

				   //���ѹ����������ת��
				   
				   break;
		   case 06://�����Ĵ�����д��
				   
				   switch (Address)
					   {
						   

						   case 0x00://�豸������رգ�����ֵ������
										
									   if(Value <= 1)
							 				{
							 					AUnionD.UnionStartFlag = Value;
												
							 					UnionD.UnionStartFlag = Value;

												ModuBus1RTU_WriteResponse(Address,Value);
							 				}
											   
									   
									   break;

						   case 0x0006:  //���ϸ�λ����
									   
									  UnionLCD.UnionD.Error_Reset = Value;												
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

									   ModuBus1RTU_WriteResponse(Address,Value);

									   
										   
									   break;

						   case 7:  //Ŀ������ѹ������
									   
									   if(Value < sys_config_data.Auto_stop_pressure)
										   {
											   if(Value >= 20)
												   {
													   sys_config_data.zhuan_huan_temperture_value = Value;
													   Write_Internal_Flash();//����ز������б���
													   ModuBus1RTU_WriteResponse(Address,Value);
													   
													   
												   }
										   }
									   
									   break;
						   case 8:  //ͣ������ѹ������
									   
											   if(Value < Sys_Admin.DeviceMaxPressureSet)
												   {
													   if(Value > sys_config_data.zhuan_huan_temperture_value)
														   {
															   sys_config_data.Auto_stop_pressure = Value;
															   Write_Internal_Flash();//����ز������б���
															   ModuBus1RTU_WriteResponse(Address,Value);
															   
														   }
												   }

										   
									   
									   break;
						   case 9: //��������ѹ��
									   
											   if(Value < sys_config_data.Auto_stop_pressure)
												   {
													   sys_config_data.Auto_start_pressure = Value;
													   Write_Internal_Flash();//����ز������б���
													   ModuBus1RTU_WriteResponse(Address,Value);
													   
														   
												   }

									   break;
						   
							   

						   default:
								   break;
					   }
				   
				   break;

		   default:
				   //��Чָ��
				   break;
	   }


   return 0 ;
}


uint8 ModuBus1RTU_WriteResponse(uint16 address,uint16 Data16)
{
	uint16 check_sum = 0;
	
	U1_Inf.TX_Data[0] =  Sys_Admin.ModBus_Address;
	U1_Inf.TX_Data[1]= 0x06;

	
	U1_Inf.TX_Data[2] = address >> 8;    // ��ַ���ֽ�
	U1_Inf.TX_Data[3] = address & 0x00FF;  //��ַ���ֽ�
	

	U1_Inf.TX_Data[4] = Data16 & 0x00FF;  //���ݵ��ֽ�
	U1_Inf.TX_Data[5] = Data16 >> 8;   //���ݸ��ֽ�

	check_sum  = ModBusCRC16(U1_Inf.TX_Data,8);   //����������ֽ����ı�
	U1_Inf.TX_Data[6]  = check_sum >> 8 ;
	U1_Inf.TX_Data[7]  = check_sum & 0x00FF;

	Usart_SendStr_length(USART1,U1_Inf.TX_Data,8);

	return 0;
}

