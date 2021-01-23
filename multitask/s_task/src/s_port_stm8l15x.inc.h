
static void TIM2_Config(void){
    /* Enable TIM2 clock */
    CLK_PeripheralClockConfig(CLK_Peripheral_TIM2, ENABLE);
    
    /* Time Base configuration */
#define TIM2_PERIOD    0xFFFF
    TIM2_TimeBaseInit(TIM2_Prescaler_1, TIM2_CounterMode_Up, TIM2_PERIOD);
      
    TIM2_UpdateRequestConfig(TIM2_UpdateSource_Global);
    
    /* Clear TIM2 update flag */
    TIM2_ClearFlag(TIM2_FLAG_Update);
    TIM2_ITConfig(TIM2_IT_Update, ENABLE); 

    /* TIM2 counter enable */
    TIM2_Cmd(ENABLE);
}

static void CLK_Config(void){
    /* Select HSI as system clock source */
    CLK->SWCR |= CLK_SWCR_SWEN;

    /* CLK->ICKCR |= (CLK_ICKCR_HSION | CLK_ICKCR_LSION); */
    CLK->ICKCR |= (CLK_ICKCR_HSION);

    /* while (CLK_GetFlagStatus(CLK_FLAG_HSIRDY) == RESET); */
    while((CLK->ICKCR & CLK_ICKCR_HSIRDY) == 0);

    /* CLK_SYSCLKSourceConfig(CLK_SYSCLKSource_HSI); */
    CLK->SWR = CLK_SYSCLKSource_HSI;

    /* system clock prescaler: 16*/
    /* CLK->CKDIVR = CLK_SYSCLKDiv_128; */
    /* CLK->CKDIVR = CLK_SYSCLKDiv_8; */
    CLK->CKDIVR = CLK_SYSCLKDiv_1;

    while (CLK->SCSR != CLK_SYSCLKSource_HSI){
    }

    /* CLK->ICKCR &= (uint8_t)(~CLK_ICKCR_LSION); */
    
    CLK->SWCR &= (uint8_t)(~CLK_SWCR_SWEN);
}

volatile uint32_t g_tim2_count = 0;
INTERRUPT_HANDLER(TIM2_UPD_OVF_TRG_BRK_USART2_TX_IRQHandler,19)
{
    /* In order to detect unexpected events during development,
       it is recommended to set a breakpoint on the following instruction.
    */
    ++g_tim2_count;

    /* Clear the IT pending Bit */
      TIM2->SR1 = (uint8_t)(~(uint8_t)TIM2_IT_Update);
}

#ifdef USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *   where the assert_param error has occurred.
  * @param file: pointer to the source file name
  * @param line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/* 3. Implement the initilization function for clock. Leave it blank if not required. */
void my_clock_init(){
    CLK_Config();
    TIM2_Config();
    enableInterrupts();
}

/* 4. Implement the function of getting current clock ticks. */
my_clock_t my_clock() {
    return g_tim2_count;
}

/* 5. Implement the idle delay function. */
void my_on_idle(uint32_t max_idle_ms) {
    
}


extern void swapcontext(ucontext_t *oucp, const ucontext_t *ucp);

static void create_context(ucontext_t *oucp, void *stack, size_t stack_size) {
    /* stm8 stack structure, 1. fill sp, 2, decrease sp
        6 bytes:   (high address) CC,Y,X,A   
      16 bytes:                 b0 ~ b15  (low address)
    */

    int top_sp = (int)((char *)stack + stack_size);
    *(int *)(top_sp - 2) = (int)&s_task_context_entry;
    *(char *)(top_sp - 3) = 0x20;    /* default CC */
    oucp->sp = top_sp - 2 - 22 - 1;
}

