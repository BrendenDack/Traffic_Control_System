#include <stdint.h>
#include "SysTick.h"
#include "PLL.h"
#include "tm4c123gh6pm.h"

void GPIOPortB_Init() {
    SYSCTL_RCGC2_R |= 0b10;
    SysTick_Wait10ms(2);
    GPIO_PORTB_AMSEL_R &= ~(0xFF);
    GPIO_PORTB_PCTL_R &= ~(0xFF);
    GPIO_PORTB_DIR_R |= 0xFF;
    GPIO_PORTB_AFSEL_R &= ~(0xFF);
    GPIO_PORTB_DEN_R |= 0xFF;
    //GPIO_PORTB_DATA_R |= 0b10;
}

void GPIOPortE_Init() {
    SYSCTL_RCGC2_R |= 0b10000;
    SysTick_Wait10ms(2);
    GPIO_PORTE_AMSEL_R &= ~(0xFF);
    GPIO_PORTE_PCTL_R &= ~(0xFF);
    GPIO_PORTE_DIR_R &= ~(0xFF);
    GPIO_PORTE_AFSEL_R &= ~(0xFF);
    GPIO_PORTE_DEN_R &= 0xFF;
}

struct State {
    uint32_t out;
    uint32_t Time; // 10 ms units
    const struct State *Next[4];
};
typedef const struct State STyp;
#define goS &FSM[0]
#define waitS &FSM[1]
#define goW &FSM[2]
#define waitW &FSM[3]
STyp FSM[4] = {
               {0b100001,500,{goS,waitS,goS,waitS}},
               {0b100010, 100,{goW,goW,goW,goW}},
               {0b001100,500,{goW,goW,waitW,waitW}},
               {0b010100, 100,{goS,goS,goS,goS}}
};



uint32_t ReadSwitches() {
    uint32_t data = GPIO_PORTE_DATA_R &= 0b11;
    return data;
}

void R_CLK() { //PB3 - Store Shift Register Values
    GPIO_PORTB_DATA_R |= 0b1000;
    SysTick_Wait10ms(2);
    GPIO_PORTB_DATA_R &= ~(0b1000);
}

void SR_CLK() { //PB1 - Shift Register
    GPIO_PORTB_DATA_R |= 0b10;
    SysTick_Wait10ms(2);
    GPIO_PORTB_DATA_R &= ~(0b10);
}

void SetLights(uint32_t Light) { //PB0 - Shift in the LED values
    //GPIO_PORTB_DATA_R &= ~(0xFF);
    //GPIO_PORTB_DATA_R |= (Light);
    uint32_t i;
    for ( i = 0; i < 8; i++) { //run 8 times for 8 bit register
        if (i == 6 || i == 7) { //for last 2 bits, shift in 0
            GPIO_PORTB_DATA_R &= ~(0b1);
            SR_CLK();
        }
        else {
        GPIO_PORTB_DATA_R &= ~(0b1); //clear bit 0
        uint32_t temp = (Light >> i)&0b1;
        GPIO_PORTB_DATA_R |= temp; //put either 0 or 1 in bit 0
        SysTick_Wait10ms(2);
        SR_CLK(); //shift register
        }
    }

    //store output in shift register's store register
    R_CLK();
}

void main(void){

  PLL_Init(Bus80MHz);  // set system clock to 80 MHz
  SysTick_Init(); // Initialize SysTick



  STyp *Pt; // state pointer
  uint32_t Input;

  // initialize ports and timer
  GPIOPortB_Init();
  GPIOPortE_Init();

  //PB2 - Clear Shift Register
  GPIO_PORTB_DATA_R |= 0b100;
  SysTick_Wait10ms(2);
  GPIO_PORTB_DATA_R &= ~(0b100);

  Pt = goS; // start state
  SetLights(Pt->out); // set lights

  while(1) {


      SysTick_Wait10ms(Pt->Time);
      Input = ReadSwitches(); // read sensors

      //waitW
      if (Input == 0x10 || Input == 0x11) {
          Pt = Pt->Next[3];
          SetLights(Pt->out);
          SysTick_Wait10ms(Pt->Time);
          //goS
          Pt = Pt->Next[0];
          SetLights(Pt->out);
          SysTick_Wait10ms(Pt->Time);

      }

      //waitS
      if (Input == 0x01 || Input == 0x11) {
          Pt = Pt->Next[1];
          SetLights(Pt->out);
          SysTick_Wait10ms(Pt->Time);
          //goW
          Pt = Pt->Next[2];
          SetLights(Pt->out);
          SysTick_Wait10ms(Pt->Time);

      }
  }
}
