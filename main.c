#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "stm32f10x.h"
#include "stm32f10x_exti.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_rcc.h"
#include "core_cm3.h"
#include "stm32f10x_adc.h"
#include "lcd.h"
#include "touch.h"
#include "misc.h"

/* function prototype */
void sendDataUART1(uint16_t data);
void RCC_Configure(void);
void GPIO_Configure(void);
void USART1_Init(void);
void USART2_Init(void);
void NVIC_Configure(void);
void EXTI_Configure(void);
void sendPhone(char* buf);
void piezo();
void delay_2();
void lightOff();
void lightOn();
void sendDataUSART2(uint16_t data);
void delay();

//모드: 집중모드, 독서모드, 휴식모드 + 졸음방지기능 on/off

float value_touch = 0;
int mode_index = 0;
char* mode[3] = {"relax", "read", "concentrate"};
uint16_t x, y;
uint8_t study_timer_hour= 0; //count up timer: 공부시간 측정
uint8_t study_timer_min= 0;
uint8_t study_timer_sec= 0;
uint16_t study_timer = 0;
bool studytimer_on = false; //공부시간 측정 타이머 활성화 flag

bool empty_check  = false;
uint16_t empty_timer = 0;
int tmp_empty = 0;

uint16_t sleeptimer_on= 0;
uint16_t sleep_timer = 0; //count down timer: 졸음방지(5분마다),

uint16_t alarm_timer = 0; //count dowm timer : 알람
bool alarmtimer_on = false;

bool red_on = false;
bool green_on = false;
bool blue_on = false;

bool power_on = false;

bool show1_flag = false;        // 초기 화면
bool show2_flag = false;        // 공부 화면
bool show3_flag = false;        // 휴식 화면


uint16_t value=5;
volatile uint32_t ADC_Value[3]; // index 0 : 적외선, 1 : 조도, 2 : 터치
int color[12] = {WHITE, CYAN, BLUE, RED, MAGENTA, LGRAY, GREEN, YELLOW, BROWN, BRRED, GRAY};

//---------------------------------------------------------------------------------------------------


void RCC_Configure(void)
{
  //port
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC,ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD,ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE,ENABLE);

  //ADC1
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1,ENABLE);
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1,ENABLE);

  //bluetooth
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1,ENABLE);
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2,ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);

  //timer
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3,ENABLE);
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4,ENABLE);
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5,ENABLE);
}


void GPIO_Configure(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  //input
  //적외선 센서 PA0 - channel 0
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  //조도 센서 PA1 - channel 1
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; //output-triger
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  //터치 센서 PB1 - channel 9
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  //버튼 1,3
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_13;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_Init(GPIOC, &GPIO_InitStructure);

  //버튼 2
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 ;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_Init(GPIOB, &GPIO_InitStructure);


  //output 밝기 조절 안되는 핀,모드
  //led
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7; //green
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOD, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8; //red
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOD, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin =GPIO_Pin_9; //blue
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOD, &GPIO_InitStructure);


  //UART1
  //TX
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  //RX
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  //UART2
  //TX
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  //RX
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
}

void ADC_Configure(void) {

  ADC_InitTypeDef ADC_12;

  ADC_12.ADC_ContinuousConvMode = ENABLE;
  ADC_12.ADC_DataAlign = ADC_DataAlign_Right;
  ADC_12.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
  ADC_12.ADC_Mode = ADC_Mode_Independent;
  ADC_12.ADC_NbrOfChannel = 3;
  ADC_12.ADC_ScanConvMode = ENABLE;

  ADC_Init(ADC1, &ADC_12);
  ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_239Cycles5); //적외선
  ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 2, ADC_SampleTime_239Cycles5); //조도
  ADC_RegularChannelConfig(ADC1, ADC_Channel_9, 3, ADC_SampleTime_239Cycles5); //터치센서

  //ADC_ITConfig(ADC1,ADC_IT_EOC,ENABLE);
  ADC_DMACmd(ADC1, ENABLE);
  ADC_Cmd(ADC1, ENABLE);
  ADC_ResetCalibration(ADC1);
  while(ADC_GetResetCalibrationStatus(ADC1));
  ADC_StartCalibration(ADC1);
  while(ADC_GetCalibrationStatus(ADC1));
  ADC_SoftwareStartConvCmd(ADC1, ENABLE);

}

// DMA : 12주차
void DMA_Configure() {

  DMA_InitTypeDef DMA_Instructure;

  DMA_Instructure.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR;
  DMA_Instructure.DMA_MemoryBaseAddr = (uint32_t)&ADC_Value;
  DMA_Instructure.DMA_DIR = DMA_DIR_PeripheralSRC;
  DMA_Instructure.DMA_BufferSize = 3;
  DMA_Instructure.DMA_M2M = DMA_M2M_Disable;
  DMA_Instructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;
  DMA_Instructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_Instructure.DMA_Mode = DMA_Mode_Circular;
  DMA_Instructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;
  DMA_Instructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_Instructure.DMA_Priority = DMA_Priority_High;
  DMA_Init(DMA1_Channel1, &DMA_Instructure);
  DMA_Cmd(DMA1_Channel1,ENABLE);
}
// 버튼1 : 타이머 입력되면서 led 색깔 주황색으로 바뀜 > 집중모드
// 버튼2 : 일시정지 상태일 때 종료, 공부시간 출력
// 버튼3 : 일시정지 > 휴식모드
// EXTI : 7주차
void EXTI_Configure (void) {

  EXTI_InitTypeDef EXTI_InitStructure;

  // 버튼2
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource10);
  EXTI_InitStructure.EXTI_Line = EXTI_Line10;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);


  // 버튼1
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource4);
  EXTI_InitStructure.EXTI_Line = EXTI_Line4;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);

  // 버튼3
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource13);
  EXTI_InitStructure.EXTI_Line = EXTI_Line13;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);

  //버튼4
  //GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource0);
  //EXTI_InitStructure.EXTI_Line = EXTI_Line0;
  //EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  //EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
  //EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  //EXTI_Init(&EXTI_InitStructure);

}

void NVIC_Configure (void) {

  NVIC_InitTypeDef NVIC_InitStructure;

  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);

  // UART1
  NVIC_EnableIRQ(USART1_IRQn);
  NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
  //NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = ;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0000;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  // UART2
  NVIC_EnableIRQ(USART2_IRQn);
  NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
  //NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = ;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0000;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  NVIC_EnableIRQ(TIM2_IRQn);
  NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
  //NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = ;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0000;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);


  // 버튼 2, 1, 3, 4
  NVIC_EnableIRQ(EXTI15_10_IRQn);
  NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;
  //NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = ;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0000;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  NVIC_EnableIRQ(EXTI4_IRQn);
  NVIC_InitStructure.NVIC_IRQChannel = EXTI4_IRQn;
  //NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = ;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0000;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);


  //NVIC_EnableIRQ(EXTI0_IRQn);
  //NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
  //NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = ;
  //NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0000;
  //NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  //NVIC_Init(&NVIC_InitStructure);



}

void USART1_Init(void)
{
    USART_InitTypeDef USART1_InitStructure;

	// Enable the USART1 peripheral
	USART_Cmd(USART1, ENABLE);

	// TODO: Initialize the USART using the structure 'USART_InitTypeDef' and the function 'USART_Init'
	USART1_InitStructure.USART_BaudRate=9600;
    USART1_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART1_InitStructure.USART_StopBits = USART_StopBits_1;
    USART1_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART1_InitStructure.USART_Parity = USART_Parity_No;
    USART1_InitStructure.USART_Mode= USART_Mode_Rx|USART_Mode_Tx;
    USART_Init(USART1, &USART1_InitStructure);

	// TODO: Enable the USART1 RX interrupts using the function 'USART_ITConfig' and the argument value 'Receive Data register not empty interrupt'
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
}


void USART2_Init(void)
{
    USART_InitTypeDef USART2_InitStructure;

	// Enable the USART2 peripheral
	USART_Cmd(USART2, ENABLE);

	// TODO: Initialize the USART using the structure 'USART_InitTypeDef' and the function 'USART_Init'
	USART2_InitStructure.USART_BaudRate=9600;
    USART2_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART2_InitStructure.USART_StopBits = USART_StopBits_1;
    USART2_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART2_InitStructure.USART_Parity = USART_Parity_No;
    USART2_InitStructure.USART_Mode= USART_Mode_Rx|USART_Mode_Tx;
    USART_Init(USART2, &USART2_InitStructure);

	// TODO: Enable the USART2 RX interrupts using the function 'USART_ITConfig' and the argument value 'Receive Data register not empty interrupt'
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
}


//타이머 init
void TIM2_Init(void) {

  TIM_TimeBaseInitTypeDef TIM2_InitStructure;

  //count-up timer: 공부시간 측정
  TIM2_InitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
  TIM2_InitStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM2_InitStructure.TIM_Period = 10000;
  TIM2_InitStructure.TIM_Prescaler = 7200;

  TIM_TimeBaseInit(TIM2, &TIM2_InitStructure);
  TIM_ARRPreloadConfig(TIM2, ENABLE);
  TIM_Cmd(TIM2, ENABLE);
  TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
}




void TIM2_IRQHandler(void) { // 타이머 intterrupt handler (1s):공부시간측정, 알람, 졸음방지, 자리비움

  if(TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET){
    if(studytimer_on == true) {
      study_timer++;
    }

    //. 자리비움 감지
    if(empty_check){
      empty_timer++;
    }
    if(empty_timer >= 5){
        LCD_Clear(WHITE);
        blue_on= true;
        green_on = false;
        red_on =false;
        show2_flag = false;
        show1_flag = false;
        show3_flag = true;
        studytimer_on = false;
        empty_timer=0;
    }


    TIM_ClearITPendingBit(TIM2,TIM_IT_Update);
  }
}

void USART1_IRQHandler() {
    uint16_t word;
    if(USART_GetITStatus(USART1,USART_IT_RXNE)!=RESET){
        // the most recent received data by the USART1 peripheral
        word = USART_ReceiveData(USART1);

        // TODO implement
        USART_SendData(USART2, word);

        // clear 'Read data register not empty' flag
    	USART_ClearITPendingBit(USART1,USART_IT_RXNE);
    }
}


void USART2_IRQHandler() {
    uint16_t word;
    if(USART_GetITStatus(USART2,USART_IT_RXNE)!=RESET){
        // the most recent received data by the USART2 peripheral
        word = USART_ReceiveData(USART2);

        // TODO implement
        USART_SendData(USART1, word);

        // clear 'Read data register not empty' flag
    	USART_ClearITPendingBit(USART2,USART_IT_RXNE);
    }
}


//버튼1 인터럽트 핸들러
void EXTI4_IRQHandler(void) {
  if(EXTI_GetITStatus(EXTI_Line4) != RESET) {
    if((GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_4) == Bit_RESET)) {
         LCD_Clear(WHITE);
        green_on = true;
        red_on = false;
        blue_on =false;
        show2_flag = false;
        show3_flag = false;
        show1_flag = true;
        studytimer_on =true;
    }
    EXTI_ClearITPendingBit(EXTI_Line4);
  }
}


//버튼2
void EXTI15_10_IRQHandler(void) {
    uint16_t tmp = study_timer;

  study_timer_hour=tmp/3600;
  tmp=tmp%3600;
  study_timer_min=tmp/60;
  tmp=tmp%60;
  study_timer_sec=tmp;

  char str[30];

  sprintf(str, "study time : %uh %um %us \r\n", study_timer_hour, study_timer_min, study_timer_sec);

  if(EXTI_GetITStatus(EXTI_Line10) != RESET) {
      if((GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_10) == Bit_RESET)) {
        if(blue_on){
             LCD_Clear(WHITE);
          red_on = true;
          green_on = false;
          blue_on =false;
          show2_flag = true;
          show3_flag = false;
          show1_flag = false;
          for(int i=0; i<30; ++i){
            sendDataUART1(str[i]);
          }
          sendPhone(str);
          study_timer = 0;
        }
      }
      EXTI_ClearITPendingBit(EXTI_Line10);
    }

  if(EXTI_GetITStatus(EXTI_Line13) != RESET) {
    if((GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_13) == Bit_RESET)) {
         LCD_Clear(WHITE);
        blue_on= true;
        green_on = false;
        red_on =false;
        show2_flag = false;
        show1_flag = false;
        show3_flag = true;
        studytimer_on = false;
    }
    EXTI_ClearITPendingBit(EXTI_Line13);
  }
}

void sendDataUART1(uint16_t data){
  while((USART1->SR & USART_SR_TC) == 0);
  USART_SendData(USART1, data);
}

void sendDataUSART2(uint16_t data) {
  USART_SendData(USART2, data);
}

void lightOn() { // 전원 ON
  GPIO_ResetBits(GPIOD,GPIO_Pin_7); //g
  GPIO_ResetBits(GPIOD,GPIO_Pin_8); //r
  GPIO_ResetBits(GPIOD,GPIO_Pin_9); //b
  red_on = true;
  green_on = true;
  blue_on = true;
}

void lightOff() { // 전원 OFF
  GPIO_SetBits(GPIOD,GPIO_Pin_7);
  GPIO_SetBits(GPIOD,GPIO_Pin_8);
  GPIO_SetBits(GPIOD,GPIO_Pin_9);
  red_on = false;
  green_on = false;
  blue_on = false;
}

void sendPhone(char* buf) {
  char *info = buf;
  while(*info) {
    while(USART_GetFlagStatus(USART2,USART_FLAG_TXE) == RESET) ;
    USART_SendData(USART2,*info++);
  }
}


void delay_2() {
  for(int i=0;i<1000000;i++) ;
}
    //공부모드 시작
    // 타이머 작동

void show_timer(){
  uint16_t tmp = study_timer;

  study_timer_hour=tmp/3600;
  tmp=tmp%3600;
  study_timer_min=tmp/60;
  tmp=tmp%60;
  study_timer_sec=tmp;

  char str[30];

  sprintf(str, "%uh %um %us", study_timer_hour, study_timer_min, study_timer_sec);


  LCD_ShowString(80, 120, str, BLACK, WHITE);

}

void show1(){

  GPIO_ResetBits(GPIOD,GPIO_Pin_7);
  GPIO_SetBits(GPIOD,GPIO_Pin_8);
  GPIO_SetBits(GPIOD,GPIO_Pin_9);

  int a = GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_0);
  if(tmp_empty == 0){
      if(a == 1){
        empty_check = false;
        tmp_empty = a;
      }
      if(a==0){
        empty_check = true;
      }
    }
    if(tmp_empty == 1){

      if(a == 0 ){
        empty_check = false;
        tmp_empty = a;
      }
      if(a==1){
        empty_check = true;
      }
    }
  LCD_ShowNum(80, 60, empty_timer, 4, BLACK, WHITE);
  show_timer();
}

void show2(){
  GPIO_ResetBits(GPIOD,GPIO_Pin_8);
  GPIO_SetBits(GPIOD,GPIO_Pin_7);
  GPIO_SetBits(GPIOD,GPIO_Pin_9);
    show_timer();
}
void show3(){
  empty_check=false;
  GPIO_SetBits(GPIOD,GPIO_Pin_7);
  GPIO_SetBits(GPIOD,GPIO_Pin_8);
  GPIO_ResetBits(GPIOD,GPIO_Pin_9);

  LCD_ShowNum(80, 60, empty_timer, 4, BLACK, WHITE);

  show_timer();
}

int main(void)
{

  int32_t distance;
  SystemInit();

  RCC_Configure();

  GPIO_Configure();

  ADC_Configure();

  DMA_Configure();

  NVIC_Configure();

  EXTI_Configure();

  TIM2_Init();

  USART1_Init();

  USART2_Init();

  LCD_Init();

  Touch_Configuration();
  Touch_Adjust();
  LCD_Clear(WHITE);

  lightOff();

  while(1){
    if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 1) {
      break;
    }
  }

  // 터치센서 터치 하면 전원 on
  while(1){
      if(ADC_Value[2] > 2000) {
          LCD_ShowNum(100,70, 1,4,BLACK, WHITE);

          lightOn();
          delay_2();
          break;
      }
  }
  LCD_Clear(WHITE);
  lightOff();
  empty_timer =0;

  while(1) {
    if(!empty_check) {
      empty_timer =0;
    }
    if(show1_flag) {
      show1();
    } else if(show3_flag) {
      show3();
    } else if(show2_flag) {
      show2();
    }
  }

  return 0;
}
