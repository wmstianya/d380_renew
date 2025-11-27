



/***********************************************************************************/

//2025年3月25日11:00:16 正式使用V1.0.1

//2025年3月28日10:28:18 修改风机的最大转速为10000转

//2025年4月3日14:59:43 版本升级为V1.0.2,修改手动模式下总控风阀的测试，给各分机组增加指示灯

//2025年4月3日17:28:50 版本升级到V1.0.3,修改手动模式下，设置风机功率，功率自动清零的问题

//2025年4月12日14:40:03 版本升级到V1.0.3,修改联控设置，当禁止时，设备在运行状态则关闭，另外修正二次启动，设备保持30%功率燃烧的问题

//2025年4月14日13:39:57 版本升级到V1.0.4,修改首次启动上水的时间，由30秒改为60秒，超压停机后，风机要保持33%的功率持续运行

//2025年4月18日09:53:22  版本升级到V1.0.5,修正联控风阀不能开启的问题

//2025年4月21日14:31:14 版本升级到V1.0.6, 修正联控过程中，检查最小需求量和在线数量冲突的问题，导致，只开一台，不进行下个阶段     if(AUnionD.AliveOK_Numbers > Already_WorkNumbers)

//2025年5月7日15:00:23 版本升级到V1.0.7  增加五拼的界面，修正程序支持5台设备联控，补水等待时间由60秒，变为100秒

//2025年5月19日13:41:52 版本升级到V1.0.8,升级解决主机排烟温度报警后，调节排烟温度报警值，多次复位不掉后，主机二次启动的问题

//2025年11月26日 UART驱动重构: DMA + 双缓冲 + IDLE中断
#include "main.h"
#include "uart_driver.h"
#include "uart_test.h"

/* 启用新UART驱动 (与stm32f10x_it.c中的宏保持一致) */
//#define USE_NEW_UART_DRIVER

extern volatile uint32_t sysTickCount; /* 引用系统时钟计数器 */

/* 回环测试开关: 1=启用测试模式, 0=正常运行模式 */
#define UART_LOOPBACK_TEST_ENABLE   1  /* 启用回环测试 */
#define UART_TEST_COUNT             100   /* 测试次数 */
#define UART_TEST_PACKET_SIZE       32    /* 数据包大小(字节) */


const uint16 Soft_Version = 108;/*2025年5月19日13:42:27*/


int main(void)
 { 
//*****外部晶振12Mhz配置****************//	 
	HSE_SetSysClk(RCC_PLLMul_9);//本文件外部晶振为8Mhz RCC_PLLMul_6  改变成9 ，那什么f103.h,  文件中晶振Hz也需要修改
	SysTick_Init();
//设置NVIC中断分组2:2位抢占优先级，2位响应优先级
	NVIC_Configuration(); 	 
//*********蜂鸣器端口，火焰检测端口配置****//
  	LED_GPIO_Config();
	IO_Interrupt_Config();

//*********继电器端口配置******************//
	RELAYS_GPIO_Config();

// ***************ADC 初始化****************//
	ADCx_Init();  
	ADS1220Init();
    ADS1220Config();
//*****************SPI初始化**************************//
	SPI_Config_Init();
	JTAG_Diable();
//***************串口1初始化为9600 for    RS485A1B1****//
	uart_init(9600);//优先级2:0

#ifdef USE_NEW_UART_DRIVER
//***************串口2 使用新DMA驱动 for 10.1寸外置的屏*****//
	uartDisplayInit(9600);  //DMA + IDLE中断模式
//***************串口3 使用新DMA驱动 for 设备内部通信准备******//
	uartSlaveInit(9600);    //DMA + IDLE中断模式

#if UART_LOOPBACK_TEST_ENABLE
	/* 回环测试初始化 - 测试USART2 (需要PA2-PA3短接) */
	u1_printf("\n====== UART DMA Loopback Test ======\n");
	u1_printf("Testing USART2, Count=%d, Size=%d\n", UART_TEST_COUNT, UART_TEST_PACKET_SIZE);
	u1_printf("Please short PA2(TX) <-> PA3(RX)\n");
	
	/* 先用阻塞方式测试USART2是否能发送 */
	{
		uint8_t testData[] = {0xAA, 0x55, 0x01, 0x02, 0x03};
		u1_printf("Blocking send test...\n");
		uartSendBlocking(&uartDisplayHandle, testData, 5);
		u1_printf("Blocking send done.\n");
		SysTick_Delay_ms(100);
		
		/* 检查是否收到回环数据 */
		if (uartIsRxReady(&uartDisplayHandle))
		{
			u1_printf("Loopback OK! Received data.\n");
		}
		else
		{
			u1_printf("No loopback data! Check PA2-PA3 connection.\n");
		}
		uartClearRxFlag(&uartDisplayHandle);
	}
	
	u1_printf("Starting DMA loopback test...\n");
	uartTestInit(&uartDisplayHandle);
	uartTestStartLoopback(&uartDisplayHandle, UART_TEST_COUNT, UART_TEST_PACKET_SIZE);
#endif

#else
//***************串口2     A2 B2初始化为19200       for 10.1寸外置的屏*****//
	uart2_init(9600);//优先级2:1
//***************串口3初始化为9600       for 设备内部通信准备******//
	uart3_init(9600); //优先级2:2	 

#if UART_LOOPBACK_TEST_ENABLE
	/* 旧驱动回环测试 */
	u1_printf("\n====== OLD Driver Loopback Test ======\n");
	{
		uint8_t testData[] = {0xBB, 0x66, 0x01, 0x02, 0x03};
		u1_printf("Old driver blocking send...\n");
		Usart_SendStr_length(USART2, testData, 5);
		u1_printf("Old driver send done.\n");
	}
#endif
#endif

//***************串口4初始化为 联控或者本地通信 ****//
	uart4_init(9600); //优先级2:2	 

	
//***配置1ms定时中断,包括全串口接收周期时间配置***//
	TIM4_Int_Init(1000,71);//1ms定时中断
//***配置0.1ms定时中断,用于风机速度检测***//
	//TIM5_Int_Init(99,71); //默认关闭，需主动开启和关闭
//***************配置蜂鸣器 输出2.7Khz************//
	TIM2_Int_Init(1000,71);  //优先级0:3
//***************配置PWM定时器，2khz可调************//
	TIM3_PWM_Init(); 
//***************开机上电蜂鸣器滴一下************//
	BEEP_TIME(300); 
			
//**************对程序用到的结构体进行初始化处理*//
	clear_struct_memory(); 
	
//**************CPU ID 加密****************************// 

	Write_CPU_ID_Encrypt();
	
	
	sys_flag.Address_4 = PortPin_Scan(SLAVE_ADDRESS_PORT,SLAVE_PIN_1);
	sys_flag.Address_3 = PortPin_Scan(SLAVE_ADDRESS_PORT,SLAVE_PIN_2);
	sys_flag.Address_2 = PortPin_Scan(GPIOA,SLAVE_PIN_3);
	sys_flag.Address_1 = PortPin_Scan(GPIOA,SLAVE_PIN_4);//该引脚未使用

//  改为由软件控制	
	sys_flag.Address_Number = sys_flag.Address_1 * 1 + sys_flag.Address_2 * 2  + sys_flag.Address_3 * 4 + sys_flag.Address_4 * 8;
	
	u1_printf("\n*地址参数：= %d\n",sys_flag.Address_Number);
	u1_printf("\n*软件版本：= %d\n",Soft_Version);

//**************系统参数默认配置，前吹扫，停炉温度等***//	
	sys_control_config_function(); //

	//增加开机自检屏幕，检测到屏幕，则从分机取地址
	
	sys_flag.PowerOn_Index = 0;
	sys_flag.Check_Finsh = OK; 	
	if(sys_flag.Address_Number)
		{
			//解决小屏损坏后，单机无法使用的问题
			sys_flag.Check_Finsh = 0; //当有拨码地址时，则以拨码地址为准
			Sys_Admin.ModBus_Address = sys_flag.Address_Number;  //强制赋值
			LCD4013X.DLCD.Address = Sys_Admin.ModBus_Address;
		}
	
	while(sys_flag.Check_Finsh)
		   {
		   		IWDG_Feed();
				if(Power_ON_Begin_Check_Function())
					sys_flag.Check_Finsh = FALSE;
		   }
	
	
	SysTick_Delay_ms(100);
	
	
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++调试用，待删除
//写内存，设标志，提示成功激活，再次进入，不再提示激活信息	
	read_serial_data();	//15ms
	read_serial_data();	//15ms
	read_serial_data();	//15ms
	read_serial_data();	//15ms
	read_serial_data();	//15ms

	Flash_Read_Protect();
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	Close_All_Realys_Function();

	//
// 单步调试时，需要关闭看门狗，用JLINK时，可以不关闭 	 
	 IWDG_Config(IWDG_Prescaler_64 ,900);//若超时没有喂狗则复位


//**************针对LCD自带RTC和主板RTC时间同步********//	
	
	while(1)
	{	
#if UART_LOOPBACK_TEST_ENABLE && defined(USE_NEW_UART_DRIVER)
		/* 回环测试处理 */
		IWDG_Feed();
		if (uartTestProcess())
		{
			/* 测试完成，打印结果 */
			uartTestPrintResult();
			u1_printf("\nTest completed! System halted.\n");
			while(1) { IWDG_Feed(); }  /* 停止，查看结果 */
		}
		continue;  /* 测试模式下跳过正常业务 */
#endif

		Sys_Admin.Fan_Speed_Check = 0;
		
//***********副系统SPI 炉内温度读取***********************//
		SpiReadData();
//***********喂狗程序***********//
		IWDG_Feed();
//***********炉体温度和烟气温度，两个压力值的处理***********//
		ADC_Process();
//***********读取并转串口的数据**********根据设备类型进行切换*************//
		read_serial_data();	//15ms
//1s时间到，****************************************************//
		One_Sec_Check();
//***********串口1 A1B1远程控制 传感器485通信解析,都要根据设备类型来选择***********//		
		ModBus_Communication();
//**********继电器需要过零控制，检测不到中断，需要强制处理******************************************//
		Relays_NoInterrupt_ON_OFF();
		//***********机器地址和设备类型的判定***********************//
#ifdef USE_NEW_UART_DRIVER
		{
			static uint32_t lastDmaCnt = 0;
			uint32_t currDmaCnt = DMA_GetCurrDataCounter(uartDisplayHandle.config.dmaRxChannel);
			
			/* 调试: 检查USART2是否收到数据 */
			if (uartIsRxReady(&uartDisplayHandle))
			{
				u1_printf("U2 RX: len=%d\n", uartDisplayHandle.buffer.rxDataLen);
			}
			
			/* 调试: 打印DMA计数器 */
			if (currDmaCnt != lastDmaCnt)
			{
				u1_printf("DMA Cnt: %d (Received %d)\n", currDmaCnt, UART_RX_BUFFER_SIZE - currDmaCnt);
				lastDmaCnt = currDmaCnt;
			}
			else
			{
				/* 周期性打印以便确认DMA状态 (每1s打印一次) */
				static uint32_t printTick = 0;
				if (sysTickCount - printTick > 1000)
				{
					u1_printf("DMA Idle: %d\n", currDmaCnt);
					printTick = sysTickCount;
				}
			}
			
			/* 调试: 检查RXNE标志 (如果DMA没工作，RXNE会置位) */
			if (USART_GetFlagStatus(USART2, USART_FLAG_RXNE) != RESET)
			{
				uint16_t data = USART_ReceiveData(USART2);
				u1_printf("RXNE set! Data: %02X (DMA failed to read)\n", data);
			}
		}
#endif

		switch (sys_flag.Address_Number)
			{
				case 0: //主控设备
						//根据设备类型进行切换
						
						//***********串口2 A2B210.1 LCD下发命令解析****************//
							Union_ModBus2_Communication();
						
						switch (Sys_Admin.Device_Style)
							{
								case 0:  //常规单体1吨D级蒸汽发生器
								case 1:  //相变单模块蒸汽发生器
								case 2:
								case 3:
								//***********串口3 多机联控和本地变频补水通信，485通信解析***********//	
										Modbus3_UnionTx_Communication();
										ModBus_Uart3_LocalRX_Communication();
								//*******还需要有联控的功能数据********************************8
								
								//*******处理串口4接收的数据*****************************88
										if(sys_flag.LCD10_Connect)
											{
												//当有主屏连接时，再沟通从机的通信
												Union_Modbus4_UnionTx_Communication();
											}
										
										Union_ModBus_Uart4_Local_Communication();  //
								//***********前后吹扫，点火功率边界值检查***********//
										Union_Check_Config_Data_Function();
								//***********各机组联动控制程序***********//
										D50L_SoloPressure_Union_MuxJiZu_Control_Function();
								//**********报警输出继电器8**************//
										Alarm_Out_Function();

										break;
								
								default:
									Sys_Admin.Device_Style = 0;

										break;
							}

						break;
				case 1:
				case 2:
				case 3:
				case 4:  //从属设备
				case 5:
				case 6:
					//***********屏幕相关变量 检查***********//
						LCD4013_Data_Check_Function();
					//***********串口2 A2B2    LCD下发命令解析****************//
						ModBus2LCD4013_Lcd7013_Communication();
					//*******处理串口3      变频进水阀************************
						Modbus3_UnionTx_Communication();
						ModBus_Uart3_LocalRX_Communication();
					//*******处理串口4接收的数据*****************************88
						ModBus_Uart4_Local_Communication();  //
					
					//*************锅炉主控程序+++++++设备补水功能******************//	
						//XiangBian_Steam_AddFunction();
						System_All_Control();
					//***********风机风速判断***********//
						Fan_Speed_Check_Function();
						
					
					
						break;
				default:
				
						break;
			}
		



 
  }
}



	
	


