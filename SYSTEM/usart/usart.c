
	  
#include "main.h"

 



#if 1
#pragma import(__use_no_semihosting)             
////标准库需要的支持函数                 
struct __FILE 
{ 
	int handle; 

}; 


FILE __stdout;
// FILE __stdin;
// FILE __stderr;       
//定义_sys_exit()以避免使用半主机模式    
_sys_exit(int x) 
{ 
	x = x; 
} 

//_ttywrch(int ch)
//{
//	ch =ch;
//}

//重定义fputc函数 
int fputc(int ch, FILE *f)
{      
	while((USART1->SR&0X40)==0);//循环发送,直到发送完毕   
    USART1->DR = (u8) ch;      
	return ch;
}




#endif 


 
//串口1中断服务程序
//注意,读取USARTx->SR能避免莫名其妙的错误   	
u8 USART_RX_BUF[USART_REC_LEN];     //接收缓冲,最大USART_REC_LEN个字节.
//接收状态
//bit15，	接收完成标志
//bit14，	接收到0x0d
//bit13~0，	接收到的有效字节数目
u16 USART_RX_STA=0;       //接收状态标记	  


RTU_DATA Rtu_Data ;




//初始化IO 串口1 
//bound:波特率
void uart_init(u32 bound){
    //GPIO端口设置
    GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	 
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1|RCC_APB2Periph_GPIOA, ENABLE);	//使能USART1，GPIOA时钟
 	USART_DeInit(USART1);  //复位串口1
	 //USART1_TX   PA.9
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9; //PA.9
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//复用推挽输出
    GPIO_Init(GPIOA, &GPIO_InitStructure); //初始化PA9
   
    //USART1_RX	  PA.10
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//浮空输入
    GPIO_Init(GPIOA, &GPIO_InitStructure);  //初始化PA10

   //Usart1 NVIC 配置

    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=2 ; 
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;		 
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器
  
   //USART 初始化设置

	USART_InitStructure.USART_BaudRate = bound;//一般设置为9600;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式

    USART_Init(USART1, &USART_InitStructure); //初始化串口
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);//开启中断
    USART_Cmd(USART1, ENABLE);                    //使能串口 

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
	  while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=1); //等待发送结束
	  USART_SendData(USART1,U1_Inf.TX_Data[cnt++]);
	  }
}


void Usart_SendByte( USART_TypeDef * pUSARTx, uint8_t ch )
{
	/* 发送一个字节数据到USART1 */
	USART_SendData(pUSARTx,ch);
		
	/* 等待发送完毕 */
	while (USART_GetFlagStatus(pUSARTx, USART_FLAG_TXE) == RESET);	
}
/*****************  指定长度的发送字符串 **********************/
void Usart_SendStr_length( USART_TypeDef * pUSARTx, uint8_t *str,uint32_t strlen )
{
	unsigned int k=0; 
    do 
    {
        Usart_SendByte( pUSARTx, *(str + k) );
        k++;
    } while(k < strlen);
}

/*****************  发送字符串 **********************/
void Usart_SendString( USART_TypeDef * pUSARTx, uint8_t *str)
{
	unsigned int k=0;
    do 
    {
        Usart_SendByte( pUSARTx, *(str + k) );
        k++;
    } while(*(str + k)!='\0');
}



//用于外部获取内部数据信息
void ModBus_Communication(void)
{
		
		uint8  i = 0;	
		
		uint16 checksum = 0;
		uint16 Address = 0;

		 
		if(U1_Inf.Recive_Ok_Flag)
			{
				U1_Inf.Recive_Ok_Flag = 0;//不能少哦
				 //关闭中断
				USART_ITConfig(USART1, USART_IT_RXNE, DISABLE); 
				 
				checksum  = U1_Inf.RX_Data[U1_Inf.RX_Length - 2] * 256 + U1_Inf.RX_Data[U1_Inf.RX_Length - 1];
				
			//老板键，对设备序列号进行修改
				if(checksum == ModBusCRC16(U1_Inf.RX_Data,U1_Inf.RX_Length))
					{
						//获取本地的地址信息，这时首地址为254，0x03指令   ，数据地址为1000，只要读这个点的数据，就会
						if(U1_Inf.RX_Data[0] == 254 && U1_Inf.RX_Data[1] == 0x03)
							{
								Address = U1_Inf.RX_Data[2] *256+ U1_Inf.RX_Data[3];
								if(Address == 1000)
									{
										U1_Inf.TX_Data[0] = U1_Inf.RX_Data[0];
										U1_Inf.TX_Data[1] = 0x03;// 
										U1_Inf.TX_Data[2] = 2; //数据长度为2， 根据情况改变***

										U1_Inf.TX_Data[3] = 0;
										U1_Inf.TX_Data[4] =  Sys_Admin.ModBus_Address;
										checksum  = ModBusCRC16(U1_Inf.TX_Data,7);   //这个根据总字节数改变
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
					
			
				
			//对接收缓冲器清零
				for( i = 0; i < 100;i++ )
					U1_Inf.RX_Data[i] = 0x00;
			
			//重新开启中断
				USART_ITConfig(USART1, USART_IT_RXNE, ENABLE); 
				
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
	U1_Inf.TX_Data[2] = Bytes; //根据数据长度改编


	for(index = 3; index < (Bytes + 3); index = index + 2)
		{
			U1_Inf.TX_Data[index] = JiZu[address].Datas[index + 1 -3]; //调换高低端循序
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
   uint8  Device_ID = 1; //身份地址，不能错 ************88
   float  value_buffer = 0;
   uint16 check_sum = 0;
   uint16 Data1 = 0;
   uint16 Data2 = 0;
   uint16 Data3 = 0;
   uint32 Data32_Buffer = 0;
   uint32 Int_CharH = 0;
   uint32 Int_CHarL = 0;
   uint8 Code[6] = {0};
   


	//Float_Int.value  用于单精度的转换

   
   Cmd = U1_Inf.RX_Data[1];
   Address = U1_Inf.RX_Data[2] *256+ U1_Inf.RX_Data[3];
   Length = U1_Inf.RX_Data[4] *256+ U1_Inf.RX_Data[5];
   Value = U1_Inf.RX_Data[4] *256+ U1_Inf.RX_Data[5];	//06格式下，value = length

    

   if(U1_Inf.RX_Data[0] > 1)
	   {
		   //当查询地址大于1时，自动切换为对方询问的地址
		    
		   Device_ID = Sys_Admin.ModBus_Address;
		
		   if(U1_Inf.RX_Data[0] != Sys_Admin.ModBus_Address)
			   {
				   return 0;   //非本机地址直接退出
			   }
	   }
   
   
   switch (Cmd)
	   {
		   case 03:  //00 03 01 F3 00 0F F5 D0	   

				   switch (Address)
					   {
						   case 0:
								   if(Length == 11)  //这边的数据长度也根据总变化，不是字节长度的变化
									   {
										   U1_Inf.TX_Data[0] = Device_ID;
										   U1_Inf.TX_Data[1] = 0x03;// 
										   U1_Inf.TX_Data[2] = 22; //数据长度为6， 根据情况改变***
								   
										   U1_Inf.TX_Data[3] = 0;
										   U1_Inf.TX_Data[4] = AUnionD.UnionStartFlag;// 1当前的运行状态
											Data1 = AUnionD.Big_Pressure *100;
										   U1_Inf.TX_Data[5] = Data1 >> 8 ;
										   U1_Inf.TX_Data[6] = Data1 & 0x00FF;//2当前蒸汽的压力值当前的水位状态
										   
										   U1_Inf.TX_Data[7] = 0x00;
										   U1_Inf.TX_Data[8] = AUnionD.AliveOK_Numbers;//3  //在线设备数量	

										   U1_Inf.TX_Data[9] = 0x00;
										   U1_Inf.TX_Data[10] =sys_flag.Device_ErrorNumbers ;//4故障设备数量

										   U1_Inf.TX_Data[11] = 0x00;
										   U1_Inf.TX_Data[12] = sys_flag.Already_WorkNumbers;//5在运行设备数量

										   U1_Inf.TX_Data[13] = 0;
										   U1_Inf.TX_Data[14] = 0;//6 总控故障码

										   U1_Inf.TX_Data[15] = 0;
										   U1_Inf.TX_Data[16] = 0;//7 故障复位指令

										   U1_Inf.TX_Data[17] = 0;
										   U1_Inf.TX_Data[18] = AUnionD.Target_Value *100;//8 //目标压力

										   U1_Inf.TX_Data[19] = 0;
										   U1_Inf.TX_Data[20] = AUnionD.Stop_Value *100;//9 停机压力

										   U1_Inf.TX_Data[21] = 0;
										   U1_Inf.TX_Data[22] = AUnionD.Start_Value *100;//10 启动压力

										   U1_Inf.TX_Data[23] = 0;
										   U1_Inf.TX_Data[24] = 0;//11  未使用
										   

										   
										
										   check_sum  = ModBusCRC16(U1_Inf.TX_Data,27);   //这个根据总字节数改变
										   U1_Inf.TX_Data[25]  = check_sum >> 8 ;
										   U1_Inf.TX_Data[26]  = check_sum & 0x00FF;
										   
										   Usart_SendStr_length(USART1,U1_Inf.TX_Data,27);

										   
										   
									   }

								   break;
							case 0x0063://取机组1的数据值
										A1B1_Jizu_ReadResponse(1);

										break;
							case 0x0077://取机组2的数据值
										A1B1_Jizu_ReadResponse(2);
										break;

							case 0x008B://取机组3的数据值
										//A1B1_Jizu_ReadResponse(3);
										break;

							case 0x009F://取机组4的数据值
										//A1B1_Jizu_ReadResponse(4);
										break;
							case 0x00B3://取机组5的数据值
										//A1B1_Jizu_ReadResponse(5);
										break;
							case 0x00C7://取机组6的数据值
										//A1B1_Jizu_ReadResponse(6);
										break;
							
						   
							   

						   default :
								   break;
					   }
				   

				   //针对压力做浮点数转换
				   
				   break;
		   case 06://单个寄存器的写入
				   
				   switch (Address)
					   {
						   

						   case 0x00://设备启动或关闭，根据值来区分
										
									   if(Value <= 1)
							 				{
							 					AUnionD.UnionStartFlag = Value;
												
							 					UnionD.UnionStartFlag = Value;

												ModuBus1RTU_WriteResponse(Address,Value);
							 				}
											   
									   
									   break;

						   case 0x0006:  //故障复位处理
									   
									  UnionLCD.UnionD.Error_Reset = Value;												
										AUnionD.Error_Reset = UnionLCD.UnionD.Error_Reset;
										UnionLCD.UnionD.Union_Error = 0;  //总控故障码也需要清零

										for(index = 1;index <= 10; index ++)
											{
												if(JiZu[index].Slave_D.Error_Code)
													{
														SlaveG[index].Command_SendFlag = 3; //连续发三次
														JiZu[index].Slave_D.Error_Reset = OK;
														
													 
													}
											}

									   ModuBus1RTU_WriteResponse(Address,Value);

									   
										   
									   break;

						   case 7:  //目标蒸汽压力设置
									   
									   if(Value < sys_config_data.Auto_stop_pressure)
										   {
											   if(Value >= 20)
												   {
													   sys_config_data.zhuan_huan_temperture_value = Value;
													   Write_Internal_Flash();//对相关参数进行保存
													   ModuBus1RTU_WriteResponse(Address,Value);
													   
													   
												   }
										   }
									   
									   break;
						   case 8:  //停机蒸汽压力设置
									   
											   if(Value < Sys_Admin.DeviceMaxPressureSet)
												   {
													   if(Value > sys_config_data.zhuan_huan_temperture_value)
														   {
															   sys_config_data.Auto_stop_pressure = Value;
															   Write_Internal_Flash();//对相关参数进行保存
															   ModuBus1RTU_WriteResponse(Address,Value);
															   
														   }
												   }

										   
									   
									   break;
						   case 9: //启动蒸汽压力
									   
											   if(Value < sys_config_data.Auto_stop_pressure)
												   {
													   sys_config_data.Auto_start_pressure = Value;
													   Write_Internal_Flash();//对相关参数进行保存
													   ModuBus1RTU_WriteResponse(Address,Value);
													   
														   
												   }

									   break;
						   
							   

						   default:
								   break;
					   }
				   
				   break;

		   default:
				   //无效指令
				   break;
	   }


   return 0 ;
}


uint8 ModuBus1RTU_WriteResponse(uint16 address,uint16 Data16)
{
	uint16 check_sum = 0;
	
	U1_Inf.TX_Data[0] =  Sys_Admin.ModBus_Address;
	U1_Inf.TX_Data[1]= 0x06;

	
	U1_Inf.TX_Data[2] = address >> 8;    // 地址高字节
	U1_Inf.TX_Data[3] = address & 0x00FF;  //地址低字节
	

	U1_Inf.TX_Data[4] = Data16 & 0x00FF;  //数据低字节
	U1_Inf.TX_Data[5] = Data16 >> 8;   //数据高字节

	check_sum  = ModBusCRC16(U1_Inf.TX_Data,8);   //这个根据总字节数改变
	U1_Inf.TX_Data[6]  = check_sum >> 8 ;
	U1_Inf.TX_Data[7]  = check_sum & 0x00FF;

	Usart_SendStr_length(USART1,U1_Inf.TX_Data,8);

	return 0;
}

