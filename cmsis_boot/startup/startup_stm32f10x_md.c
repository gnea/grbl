/**
 ******************************************************************************
 * @file      startup_stm32f10x_md.c
 * @author    Coocox
 * @version   V1.0
 * @date      12/23/2009
 * @brief     STM32F10x Medium Density Devices Startup code.
 *            This module performs:
 *                - Set the initial SP
 *                - Set the vector table entries with the exceptions ISR address
 *                - Initialize data and bss
 *                - Setup the microcontroller system.
 *                - Call the application's entry point.
 *            After Reset the Cortex-M3 processor is in Thread mode,
 *            priority is Privileged, and the Stack is set to Main.
 *******************************************************************************
 */
 

/*----------Stack Configuration-----------------------------------------------*/  
#define STACK_SIZE       0x00000100      /*!< The Stack size suggest using even number     */
__attribute__ ((section(".co_stack")))
unsigned long pulStack[STACK_SIZE];      


/*----------Macro definition--------------------------------------------------*/  
#define WEAK __attribute__ ((weak))           


/*----------Declaration of the default fault handlers-------------------------*/  
/* System exception vector handler */
__attribute__ ((used))
void WEAK  Reset_Handler(void);
void WEAK  NMI_Handler(void);
void WEAK  HardFault_Handler(void);
void WEAK  MemManage_Handler(void);
void WEAK  BusFault_Handler(void);
void WEAK  UsageFault_Handler(void);
void WEAK  SVC_Handler(void);
void WEAK  DebugMon_Handler(void);
void WEAK  PendSV_Handler(void);
void WEAK  SysTick_Handler(void);
void WEAK  WWDG_IRQHandler(void);
void WEAK  PVD_IRQHandler(void);
void WEAK  TAMPER_IRQHandler(void);
void WEAK  RTC_IRQHandler(void);
void WEAK  FLASH_IRQHandler(void);
void WEAK  RCC_IRQHandler(void);
void WEAK  EXTI0_IRQHandler(void);
void WEAK  EXTI1_IRQHandler(void);
void WEAK  EXTI2_IRQHandler(void);
void WEAK  EXTI3_IRQHandler(void);
void WEAK  EXTI4_IRQHandler(void);
void WEAK  DMA1_Channel1_IRQHandler(void);
void WEAK  DMA1_Channel2_IRQHandler(void);
void WEAK  DMA1_Channel3_IRQHandler(void);
void WEAK  DMA1_Channel4_IRQHandler(void);
void WEAK  DMA1_Channel5_IRQHandler(void);
void WEAK  DMA1_Channel6_IRQHandler(void);
void WEAK  DMA1_Channel7_IRQHandler(void);
void WEAK  ADC1_2_IRQHandler(void);
void WEAK  USB_HP_CAN1_TX_IRQHandler(void);
void WEAK  USB_LP_CAN1_RX0_IRQHandler(void);
void WEAK  CAN1_RX1_IRQHandler(void);
void WEAK  CAN1_SCE_IRQHandler(void);
void WEAK  EXTI9_5_IRQHandler(void);
void WEAK  TIM1_BRK_IRQHandler(void);
void WEAK  TIM1_UP_IRQHandler(void);
void WEAK  TIM1_TRG_COM_IRQHandler(void);
void WEAK  TIM1_CC_IRQHandler(void);
void WEAK  TIM2_IRQHandler(void);
void WEAK  TIM3_IRQHandler(void);
void WEAK  TIM4_IRQHandler(void);
void WEAK  I2C1_EV_IRQHandler(void);
void WEAK  I2C1_ER_IRQHandler(void);
void WEAK  I2C2_EV_IRQHandler(void);
void WEAK  I2C2_ER_IRQHandler(void);
void WEAK  SPI1_IRQHandler(void);
void WEAK  SPI2_IRQHandler(void);
void WEAK  USART1_IRQHandler(void);
void WEAK  USART2_IRQHandler(void);
void WEAK  USART3_IRQHandler(void);
void WEAK  EXTI15_10_IRQHandler(void);
void WEAK  RTCAlarm_IRQHandler(void);
void WEAK  USBWakeUp_IRQHandler(void); 


/*----------Symbols defined in linker script----------------------------------*/  
extern unsigned long _sidata;    /*!< Start address for the initialization 
                                      values of the .data section.            */
extern unsigned long _sdata;     /*!< Start address for the .data section     */    
extern unsigned long _edata;     /*!< End address for the .data section       */    
extern unsigned long _sbss;      /*!< Start address for the .bss section      */
extern unsigned long _ebss;      /*!< End address for the .bss section        */      
extern void _eram;               /*!< End address for ram                     */


/*----------Function prototypes-----------------------------------------------*/  
extern int main(void);           /*!< The entry point for the application.    */
extern void SystemInit(void);    /*!< Setup the microcontroller system(CMSIS) */
void Default_Reset_Handler(void);   /*!< Default reset handler                */
static void Default_Handler(void);  /*!< Default exception handler            */


/**
  *@brief The minimal vector table for a Cortex M3.  Note that the proper constructs
  *       must be placed on this to ensure that it ends up at physical address
  *       0x00000000.  
  */
__attribute__ ((used,section(".isr_vector")))
void (* const g_pfnVectors[])(void) =
{       
  /*----------Core Exceptions-------------------------------------------------*/
  (void *)&pulStack[STACK_SIZE],     /*!< The initial stack pointer         */
  Reset_Handler,                /*!< Reset Handler                            */
  NMI_Handler,                  /*!< NMI Handler                              */
  HardFault_Handler,            /*!< Hard Fault Handler                       */
  MemManage_Handler,            /*!< MPU Fault Handler                        */
  BusFault_Handler,             /*!< Bus Fault Handler                        */
  UsageFault_Handler,           /*!< Usage Fault Handler                      */
  0,0,0,0,                      /*!< Reserved                                 */
  SVC_Handler,                  /*!< SVCall Handler                           */
  DebugMon_Handler,             /*!< Debug Monitor Handler                    */
  0,                            /*!< Reserved                                 */
  PendSV_Handler,               /*!< PendSV Handler                           */
  SysTick_Handler,              /*!< SysTick Handler                          */
  
  /*----------External Exceptions---------------------------------------------*/
  WWDG_IRQHandler,              /*!<  0: Window Watchdog                      */
  PVD_IRQHandler,               /*!<  1: PVD through EXTI Line detect         */
  TAMPER_IRQHandler,            /*!<  2: Tamper                               */
  RTC_IRQHandler,               /*!<  3: RTC                                  */
  FLASH_IRQHandler,             /*!<  4: Flash                                */
  RCC_IRQHandler,               /*!<  5: RCC                                  */
  EXTI0_IRQHandler,             /*!<  6: EXTI Line 0                          */
  EXTI1_IRQHandler,             /*!<  7: EXTI Line 1                          */
  EXTI2_IRQHandler,             /*!<  8: EXTI Line 2                          */
  EXTI3_IRQHandler,             /*!<  9: EXTI Line 3                          */
  EXTI4_IRQHandler,             /*!< 10: EXTI Line 4                          */
  DMA1_Channel1_IRQHandler,     /*!< 11: DMA1 Channel 1                       */
  DMA1_Channel2_IRQHandler,     /*!< 12: DMA1 Channel 2                       */
  DMA1_Channel3_IRQHandler,     /*!< 13: DMA1 Channel 3                       */
  DMA1_Channel4_IRQHandler,     /*!< 14: DMA1 Channel 4                       */
  DMA1_Channel5_IRQHandler,     /*!< 15: DMA1 Channel 5                       */
  DMA1_Channel6_IRQHandler,     /*!< 16: DMA1 Channel 6                       */
  DMA1_Channel7_IRQHandler,     /*!< 17: DMA1 Channel 7                       */
  ADC1_2_IRQHandler,            /*!< 18: ADC1 & ADC2                          */
  USB_HP_CAN1_TX_IRQHandler,    /*!< 19: USB High Priority or CAN1 TX         */
  USB_LP_CAN1_RX0_IRQHandler,   /*!< 20: USB Low  Priority or CAN1 RX0        */
  CAN1_RX1_IRQHandler,          /*!< 21: CAN1 RX1                             */
  CAN1_SCE_IRQHandler,          /*!< 22: CAN1 SCE                             */
  EXTI9_5_IRQHandler,           /*!< 23: EXTI Line 9..5                       */
  TIM1_BRK_IRQHandler,          /*!< 24: TIM1 Break                           */
  TIM1_UP_IRQHandler,           /*!< 25: TIM1 Update                          */
  TIM1_TRG_COM_IRQHandler,      /*!< 26: TIM1 Trigger and Commutation         */
  TIM1_CC_IRQHandler,           /*!< 27: TIM1 Capture Compare                 */
  TIM2_IRQHandler,              /*!< 28: TIM2                                 */
  TIM3_IRQHandler,              /*!< 29: TIM3                                 */
  TIM4_IRQHandler,              /*!< 30: TIM4                                 */
  I2C1_EV_IRQHandler,           /*!< 31: I2C1 Event                           */
  I2C1_ER_IRQHandler,           /*!< 32: I2C1 Error                           */
  I2C2_EV_IRQHandler,           /*!< 33: I2C2 Event                           */
  I2C2_ER_IRQHandler,           /*!< 34: I2C2 Error                           */
  SPI1_IRQHandler,              /*!< 35: SPI1                                 */
  SPI2_IRQHandler,              /*!< 36: SPI2                                 */
  USART1_IRQHandler,            /*!< 37: USART1                               */
  USART2_IRQHandler,            /*!< 38: USART2                               */
  USART3_IRQHandler,            /*!< 39: USART3                               */
  EXTI15_10_IRQHandler,         /*!< 40: EXTI Line 15..10                     */
  RTCAlarm_IRQHandler,          /*!< 41: RTC Alarm through EXTI Line          */
  USBWakeUp_IRQHandler,         /*!< 42: USB Wakeup from suspend              */  
  0,0,0,0,0,0,0,                /*!< Reserved                                 */ 
  (void *)0xF108F85F            /*!< Boot in RAM mode                         */
};        


/**
  * @brief  This is the code that gets called when the processor first
  *         starts execution following a reset event. Only the absolutely
  *         necessary set is performed, after which the application
  *         supplied main() routine is called. 
  * @param  None
  * @retval None
  */
void Default_Reset_Handler(void)
{
  /* Initialize data and bss */
  unsigned long *pulSrc, *pulDest;

  /* Copy the data segment initializers from flash to SRAM */
  pulSrc = &_sidata;

  for(pulDest = &_sdata; pulDest < &_edata; )
  {
    *(pulDest++) = *(pulSrc++);
  }
  
  /* Zero fill the bss segment.  This is done with inline assembly since this
     will clear the value of pulDest if it is not kept in a register. */
  __asm("  ldr     r0, =_sbss\n"
        "  ldr     r1, =_ebss\n"
        "  mov     r2, #0\n"
        "  .thumb_func\n"
        "zero_loop:\n"
        "    cmp     r0, r1\n"
        "    it      lt\n"
        "    strlt   r2, [r0], #4\n"
        "    blt     zero_loop");
  
  /* Setup the microcontroller system. */
  SystemInit();
    
  /* Call the application's entry point.*/
  main();
}

/**
  *@brief Provide weak aliases for each Exception handler to the Default_Handler. 
  *       As they are weak aliases, any function with the same name will override 
  *       this definition.
  */
#pragma weak Reset_Handler = Default_Reset_Handler  
#pragma weak NMI_Handler = Default_Handler
#pragma weak HardFault_Handler = Default_Handler
#pragma weak MemManage_Handler = Default_Handler
#pragma weak BusFault_Handler = Default_Handler
#pragma weak UsageFault_Handler = Default_Handler
#pragma weak SVC_Handler = Default_Handler
#pragma weak DebugMon_Handler = Default_Handler
#pragma weak PendSV_Handler = Default_Handler
#pragma weak SysTick_Handler = Default_Handler
#pragma weak WWDG_IRQHandler = Default_Handler
#pragma weak PVD_IRQHandler = Default_Handler
#pragma weak TAMPER_IRQHandler = Default_Handler
#pragma weak RTC_IRQHandler = Default_Handler
#pragma weak FLASH_IRQHandler = Default_Handler
#pragma weak RCC_IRQHandler = Default_Handler
#pragma weak EXTI0_IRQHandler = Default_Handler
#pragma weak EXTI1_IRQHandler = Default_Handler
#pragma weak EXTI2_IRQHandler = Default_Handler
#pragma weak EXTI3_IRQHandler = Default_Handler
#pragma weak EXTI4_IRQHandler = Default_Handler
#pragma weak DMA1_Channel1_IRQHandler = Default_Handler
#pragma weak DMA1_Channel2_IRQHandler = Default_Handler
#pragma weak DMA1_Channel3_IRQHandler = Default_Handler
#pragma weak DMA1_Channel4_IRQHandler = Default_Handler
#pragma weak DMA1_Channel5_IRQHandler = Default_Handler
#pragma weak DMA1_Channel6_IRQHandler = Default_Handler
#pragma weak DMA1_Channel7_IRQHandler = Default_Handler
#pragma weak ADC1_2_IRQHandler = Default_Handler
#pragma weak USB_HP_CAN1_TX_IRQHandler = Default_Handler
#pragma weak USB_LP_CAN1_RX0_IRQHandler = Default_Handler
#pragma weak CAN1_RX1_IRQHandler = Default_Handler
#pragma weak CAN1_SCE_IRQHandler = Default_Handler
#pragma weak EXTI9_5_IRQHandler = Default_Handler
#pragma weak TIM1_BRK_IRQHandler = Default_Handler
#pragma weak TIM1_UP_IRQHandler = Default_Handler
#pragma weak TIM1_TRG_COM_IRQHandler = Default_Handler
#pragma weak TIM1_CC_IRQHandler = Default_Handler
#pragma weak TIM2_IRQHandler = Default_Handler
#pragma weak TIM3_IRQHandler = Default_Handler
#pragma weak TIM4_IRQHandler = Default_Handler
#pragma weak I2C1_EV_IRQHandler = Default_Handler
#pragma weak I2C1_ER_IRQHandler = Default_Handler
#pragma weak I2C2_EV_IRQHandler = Default_Handler
#pragma weak I2C2_ER_IRQHandler = Default_Handler
#pragma weak SPI1_IRQHandler = Default_Handler
#pragma weak SPI2_IRQHandler = Default_Handler
#pragma weak USART1_IRQHandler = Default_Handler
#pragma weak USART2_IRQHandler = Default_Handler
#pragma weak USART3_IRQHandler = Default_Handler
#pragma weak EXTI15_10_IRQHandler = Default_Handler
#pragma weak RTCAlarm_IRQHandler = Default_Handler
#pragma weak USBWakeUp_IRQHandler = Default_Handler


/**
  * @brief  This is the code that gets called when the processor receives an 
  *         unexpected interrupt.  This simply enters an infinite loop, 
  *         preserving the system state for examination by a debugger.
  * @param  None
  * @retval None  
  */
static void Default_Handler(void) 
{
  /* Go into an infinite loop. */
  while (1) 
  {
  }
}

/*********************** (C) COPYRIGHT 2009 Coocox ************END OF FILE*****/
