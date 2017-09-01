#include <avr/io.h>
#include <avr/interrupt.h>
#include "io.c"

volatile unsigned char TimerFlag = 0;

unsigned long _avr_timer_M = 1;
unsigned long _avr_timer_cntcurr = 0;
volatile unsigned char timer_on = 0;
volatile unsigned char timer_done = 0;
volatile unsigned char timer_min_on = 0;
volatile unsigned char hr_cnt = 0;
volatile unsigned char hr_cnt_10 = 0;
volatile unsigned char min_cnt = 0;
volatile unsigned char min_cnt_10 = 0;
volatile unsigned char clk_sec = 0;
volatile unsigned char clk_sec_10 = 0;
volatile unsigned char timer_on_alm = 0;
volatile unsigned char timer_done_alm = 0;
volatile unsigned char timer_min_on_alm = 0;
volatile unsigned char hr_cnt_alm = 0;
volatile unsigned char hr_cnt_alm_10 = 0;
volatile unsigned char min_cnt_alm = 0;
volatile unsigned char min_cnt_alm_10 = 0;

void TimerOn() {
	TCCR1B = 0x0B;
	OCR1A = 125;
	TIMSK1 = 0x02;
	TCNT1 = 0;

	_avr_timer_cntcurr = _avr_timer_M;
	SREG |= 0x80;
}

void TimerOff() {
	TCCR1B = 0x00;
}

void TimerISR() {
	TimerFlag = 1;
}

ISR(TIMER1_COMPA_vect) {
	_avr_timer_cntcurr--;
	if(_avr_timer_cntcurr == 0) {
		TimerISR();
		_avr_timer_cntcurr = _avr_timer_M;
	}
}

void TimerSet(unsigned long M) {
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}


void set_PWM(double frequency) {
		static double current_frequency;
		if (frequency != current_frequency) {
			if (!frequency) { TCCR3B &= 0x08; }
			else { TCCR3B |= 0x03; }
			if (frequency < 0.954) { OCR3A = 0xFFFF; }
			else if (frequency > 31250) { OCR3A = 0x0000; }
			else { OCR3A = (short)(8000000 / (128 * frequency)) - 1; }
			TCNT3 = 0;
			current_frequency = frequency;
		}
			}


	void PWM_on() {
		TCCR3A = (1 << COM3A0);
		TCCR3B = (1 << WGM32) | (1 << CS31) | (1 << CS30);
		set_PWM(0);
	}

	void PWM_off() {
		TCCR3A = 0x00;
		TCCR3B = 0x00;
	}


enum States_SetTime  {INIT_ST, Set_ST} states_ST ;
unsigned char value = 0;
void Setting_Time ()
{
	unsigned buttons_ST = ~PINA & 0x01;
	//transitions
	switch (states_ST)
	{
		case INIT_ST :
		if (buttons_ST)
		{
			states_ST = Set_ST;
		}

		else
		{
			states_ST = INIT_ST;
		}
		break;

		case Set_ST :
		{
			if ( (timer_done) && (buttons_ST) )
			{
				states_ST =  INIT_ST;
			}
			else
			{
				states_ST= Set_ST;
			}


			break;
		}
		default :
		states_ST = INIT_ST;
		break;
	}

	switch (states_ST) {
		case INIT_ST:
		{
			LCD_DisplayString(1, "00:00:00");
		}
		break;

		case Set_ST :
		{
			//LCD_DisplayString(1, "Setting_Time" );
			timer_on = 1;
		}
		break;
	}

}

enum Timer_HR_States {INIT_HR, START_HR, HR_INC, HR_WRITE,
HR_WAIT, HR_INC_10, HR_ENTER} states_HR;

void Timer_HR ()
{

	unsigned char button_hr = ~PINA & 0X04;
	unsigned char button_min = ~PINA & 0X08;
	unsigned char button_enter = ~PINA & 0x10;

	switch (states_HR)
	{
		case INIT_HR :
		if (timer_on)
		{
			states_HR = START_HR;
		}
		else
		{
			states_HR = INIT_HR ;
		}
		break;

		case START_HR :
		if (button_hr == 0x04)
		{
			states_HR = HR_INC;
		}
		else
		{
			states_HR = START_HR;
		}
		break;

		case HR_INC :
		if ( (hr_cnt >= 4 ) && (hr_cnt_10 >=2))
		{
			states_HR = START_HR;
		}
		else if (hr_cnt < 10)
		{
			states_HR = HR_WRITE;
		}
		else if (hr_cnt >= 10)
		{
			states_HR = HR_INC_10;
		}

		break;

		case HR_WRITE :
		if (!button_hr)
		{
			states_HR = HR_WAIT;
		}
		else if  (button_hr)
		{
			states_HR = HR_INC;
		}

		break;

		case HR_WAIT :
		if (button_enter)
		{
			states_HR = HR_ENTER;
		}
		else if ( (button_hr) && (hr_cnt < 10) )
		{
			states_HR = HR_INC;
		}
		else if (!button_enter)
		{
			states_HR = HR_WAIT;
		}
		else if ( (button_hr) && (hr_cnt >= 10))
		{
			states_HR = HR_INC_10;
		}
		break;

		case HR_INC_10 :

		if (button_hr)
		{
			states_HR = HR_INC;
		}
		else if (!button_hr)
		{
			states_HR = HR_WAIT;
		}
		break;

		case HR_ENTER :

		states_HR = HR_ENTER;


		break;

		default :
		states_HR = INIT_HR;
		break;

	}

	switch (states_HR) {

		case START_HR:
		//LCD_ClearScreen():

		LCD_DisplayString(1, "00:00:00");

		hr_cnt = 0;
		hr_cnt_10 = 0;
		break;

		case HR_INC :
		hr_cnt = hr_cnt + 1;
		break;

		case HR_WRITE :
		LCD_Cursor(2);
		LCD_WriteData(hr_cnt + '0') ;
		break;

		case HR_INC_10 :
		hr_cnt = 0;
		LCD_Cursor(2);
		LCD_WriteData(0 + '0') ;

		hr_cnt_10 = hr_cnt_10 + 1;
		LCD_Cursor(1);
		LCD_WriteData(hr_cnt_10 + '0') ;
		break;

		case HR_ENTER :
		timer_min_on = 1;
		break;

	}
}

enum Timer_MIN_States {INIT_MIN, START_MIN, MIN_INC, MIN_WRITE,
MIN_WAIT, MIN_INC_10,  MIN_ENTER} states_MIN;

void Timer_MIN ()
{
	//static unsigned char min_cnt = 0;
	//static unsigned char min_cnt_10 = 0;
	//unsigned char button_min = ~PINA & 0X04;
	unsigned char button_min = ~PINA & 0X08;
	unsigned char button_enter = ~PINA & 0x10;

	switch (states_MIN)
	{
		case INIT_MIN :
		if (timer_min_on)
		{
			states_MIN = START_MIN;
		}
		else
		{
			states_MIN = INIT_MIN ;
		}
		break;

		case START_MIN :
		if (button_min )
		{
			states_MIN = MIN_INC;
		}
		else
		{
			states_MIN = START_MIN;
		}
		break;

		case MIN_INC :
		if ( (min_cnt >= 9 ) && (min_cnt_10 >=5))
		{
			states_MIN = START_MIN;
		}
		else if (min_cnt < 10)
		{
			states_MIN = MIN_WRITE;
		}
		else if (min_cnt >= 10)
		{
			states_MIN = MIN_INC_10;
		}

		break;

		case MIN_WRITE :
		if (!button_min)
		{
			states_MIN = MIN_WAIT;
		}
		else if  (button_min)
		{
			states_MIN = MIN_INC;
		}

		break;

		case MIN_WAIT :
		if (button_enter)
		{
			states_MIN = MIN_ENTER;
		}
		else if ( (button_min) && (min_cnt < 10) )
		{
			states_MIN = MIN_INC;
		}
		else if (!button_enter)
		{
			states_MIN = MIN_WAIT;
		}
		else if ( (button_min) && (min_cnt >= 10))
		{
			states_MIN = MIN_INC_10;
		}
		break;

		case MIN_INC_10 :

		if (button_min)
		{
			states_MIN = MIN_INC;
		}
		else if (!button_min)
		{
			states_MIN = MIN_WAIT;
		}
		break;

		case MIN_ENTER :
		states_MIN = MIN_ENTER;
		break;

		default :
		states_MIN = INIT_MIN;
		break;

	}

	switch (states_MIN) {

		case START_MIN:
		min_cnt = 0;
		min_cnt_10 = 0;
		break;

		case MIN_INC :
		min_cnt = min_cnt + 1;
		break;

		case MIN_WRITE :
		LCD_Cursor(5);
		LCD_WriteData(min_cnt + '0') ;
		break;

		case MIN_INC_10 :
		min_cnt = 0;
		LCD_Cursor(5);
		LCD_WriteData(0 + '0') ;

		min_cnt_10 = min_cnt_10 + 1;
		LCD_Cursor(4);
		LCD_WriteData(min_cnt_10 + '0') ;
		break;

		case MIN_ENTER :


		timer_done = 1;
		break;

	}
}
enum Clock_States { INIT_CLK,  CLK_TICK,
CLK_CNT , WRITE_SEC, WRITE_SEC_10, CHANGE_MIN, CHANGE_MIN_10, CHANGE_HR, CHANGE_HR_10} clk_states;

void Clock_Fct ()
{
	static unsigned char tick = 0;
	static unsigned char clk_sec = 0;
	static unsigned char clk_sec_10 = 0;
	//static unsigned char min_clk = 0;

	switch (clk_states)
	{
		case INIT_CLK :
		if (timer_done)
		{
			clk_states =  CLK_TICK;
		}
		else if (!timer_done)
		{
			clk_states = INIT_CLK;
		}
		break;

		case CLK_TICK :
		if (tick >= 4)
		{
			clk_states = CLK_CNT;
		}
		else if (tick < 4)
		{
			clk_states = CLK_TICK;
		}
		break;

		case CLK_CNT :
		if ((hr_cnt >= 9) && (min_cnt_10 >= 5) && (min_cnt >= 9) && (clk_sec >= 10 ) && (clk_sec_10 >= 5)) {
			/* code */
			clk_states = CHANGE_HR_10;
		}
		else if ((min_cnt_10 >= 5) && (min_cnt >= 9) && (clk_sec >= 10 ) && (clk_sec_10 >= 5))
		{
			clk_states = CHANGE_HR;
		}
		else if ((min_cnt <9) && (clk_sec >= 10 ) && (clk_sec_10 >= 5))
		{
			clk_states = CHANGE_MIN;
		}
		else if ((min_cnt >= 9) &&(clk_sec >= 10 ) && (clk_sec_10 >= 5))
		{
			clk_states = CHANGE_MIN_10;
		}


		else if (clk_sec >= 10)
		{
			clk_states = WRITE_SEC_10;
		}
		else if (clk_sec < 10)
		{	clk_states = WRITE_SEC;}
		break;

		case WRITE_SEC :
		clk_states =CLK_TICK;
		break;

		case WRITE_SEC_10 :
		clk_states = CLK_TICK ;
		break;

		case CHANGE_MIN :
		clk_states = CLK_TICK ;
		break;

		case CHANGE_MIN_10 :
		clk_states = CLK_TICK ;
		break;
		case CHANGE_HR :
		clk_states = CLK_TICK ;
		break;
		case CHANGE_HR_10 :
		clk_states = CLK_TICK ;
		break;
	}

	switch (clk_states)
	{
		case INIT_CLK :
		//hr_cnt_10 = hr_cnt_10 + 1;
		LCD_Cursor(1);
		LCD_WriteData(hr_cnt_10 + '0') ;
		//hr_cnt = 0;
		LCD_Cursor(2);
		LCD_WriteData(hr_cnt + '0') ;
		//min_cnt_10 = 0;
		LCD_Cursor(4);
		LCD_WriteData(min_cnt_10 + '0') ;
		//min_cnt = 0;
		LCD_Cursor(5);
		LCD_WriteData(min_cnt + '0') ;
		clk_sec_10 = 0;
		LCD_Cursor(7);
		LCD_WriteData(clk_sec_10 + '0') ;
		clk_sec = 0;
		LCD_Cursor(8);
		LCD_WriteData(clk_sec + '0') ;
		tick = 0;

		break;

		case CLK_TICK :
		tick = tick +1;
		//LCD_WriteData(1 + '0') ;
		break;

		case CLK_CNT :
		tick = 0;
		clk_sec = clk_sec +1;
		//LCD_WriteData(2 + '0') ;
		break;

		case WRITE_SEC :
		LCD_Cursor(8);
		LCD_WriteData(clk_sec + '0') ;
		break;

		case WRITE_SEC_10 :
		clk_sec_10 = clk_sec_10 + 1;
		LCD_Cursor(7);
		LCD_WriteData(clk_sec_10 + '0') ;
		clk_sec = 0;
		LCD_Cursor(8);
		LCD_WriteData(clk_sec + '0') ;
		break;

		case CHANGE_MIN :
		min_cnt = min_cnt + 1;
		LCD_Cursor(5);
		LCD_WriteData(min_cnt + '0') ;
		clk_sec_10 = 0;
		LCD_Cursor(7);
		LCD_WriteData(clk_sec_10 + '0') ;
		clk_sec = 0;
		LCD_Cursor(8);
		LCD_WriteData(clk_sec + '0') ;
		break;

		case CHANGE_MIN_10 :
		min_cnt_10 = min_cnt_10 + 1;
		LCD_Cursor(4);
		LCD_WriteData(min_cnt_10 + '0') ;
		min_cnt = 0;
		LCD_Cursor(5);
		LCD_WriteData(min_cnt + '0') ;
		clk_sec_10 = 0;
		LCD_Cursor(7);
		LCD_WriteData(clk_sec_10 + '0') ;
		clk_sec = 0;
		LCD_Cursor(8);
		LCD_WriteData(clk_sec + '0') ;
		break;

		case CHANGE_HR :
		hr_cnt = hr_cnt + 1;
		LCD_Cursor(2);
		LCD_WriteData(hr_cnt + '0') ;
		min_cnt_10 = 0;
		LCD_Cursor(4);
		LCD_WriteData(min_cnt_10 + '0') ;
		min_cnt = 0;
		LCD_Cursor(5);
		LCD_WriteData(min_cnt + '0') ;
		clk_sec_10 = 0;
		LCD_Cursor(7);
		LCD_WriteData(clk_sec_10 + '0') ;
		clk_sec = 0;
		LCD_Cursor(8);
		LCD_WriteData(clk_sec + '0') ;
		break;
		case CHANGE_HR_10 :
		hr_cnt_10 = hr_cnt_10 + 1;
		LCD_Cursor(1);
		LCD_WriteData(hr_cnt_10 + '0') ;
		hr_cnt = 0;
		LCD_Cursor(2);
		LCD_WriteData(hr_cnt + '0') ;
		min_cnt_10 = 0;
		LCD_Cursor(4);
		LCD_WriteData(min_cnt_10 + '0') ;
		min_cnt = 0;
		LCD_Cursor(5);
		LCD_WriteData(min_cnt + '0') ;
		clk_sec_10 = 0;
		LCD_Cursor(7);
		LCD_WriteData(clk_sec_10 + '0') ;
		clk_sec = 0;
		LCD_Cursor(8);
		LCD_WriteData(clk_sec + '0') ;
		break;
	}
}

enum Set_Alarm_States {INIT_ALM, START_ALM} state_Alm;
void Set_Alarm ()
{
	unsigned char button_alm = ~PINA & 0x02;
	unsigned char semicolon = 10;
	switch (state_Alm)
	{
		case INIT_ALM :
		if (button_alm)
		{
			state_Alm = START_ALM;
		}

		else{
			state_Alm = INIT_ALM;
		}
		break;

		case START_ALM :
		state_Alm = START_ALM ;

		default :
		state_Alm = INIT_ALM;
	}

	switch (state_Alm)
	{
		case INIT_ALM :
		timer_on_alm = 0;
		break;

		case START_ALM :
		timer_on_alm = 1;
		LCD_DisplayString(17, " Alarm=00:00");
		LCD_Cursor(1);
		LCD_WriteData(hr_cnt_10 + '0') ;
		LCD_Cursor(2);
		LCD_WriteData(hr_cnt + '0') ;
		LCD_Cursor(3);
		LCD_WriteData(semicolon + '0') ;
		LCD_Cursor(4);
		LCD_WriteData(min_cnt_10 + '0') ;
		LCD_Cursor(5);
		LCD_WriteData(min_cnt + '0') ;
		LCD_Cursor(6);
		LCD_WriteData(semicolon + '0') ;
		LCD_Cursor(7);
		LCD_WriteData(clk_sec_10 + '0') ;
		LCD_Cursor(8);
		LCD_WriteData(clk_sec + '0') ;
		break;
	}
}

enum Timer_HR_ALM_States {INIT_HR_ALM, START_HR_ALM, HR_ALM_INC, HR_ALM_WRITE,
HR_ALM_WAIT, HR_ALM_INC_10, HR_ALM_ENTER} states_HR_ALM;

void Timer_HR_ALM ()
{

	unsigned char button_hr_alm = ~PINA & 0X04;
	unsigned char button_min_alm = ~PINA & 0X08;
	unsigned char button_enter_alm = ~PINA & 0x10;

	switch (states_HR_ALM)
	{
		case INIT_HR_ALM :
		if (timer_on_alm)
		{
			states_HR_ALM = START_HR_ALM;
		}
		else
		{
			states_HR_ALM = INIT_HR_ALM ;
		}
		break;

		case START_HR_ALM :
		if (button_hr_alm )
		{
			states_HR_ALM = HR_ALM_INC;
		}
		else
		{
			states_HR_ALM = START_HR_ALM;
		}
		break;

		case HR_ALM_INC :
		if ( (hr_cnt_alm >= 4 ) && (hr_cnt_alm_10 >=2))
		{
			states_HR_ALM = START_HR_ALM;
		}
		else if (hr_cnt_alm < 10)
		{
			states_HR_ALM = HR_ALM_WRITE;
		}
		else if (hr_cnt_alm >= 10)
		{
			states_HR_ALM = HR_ALM_INC_10;
		}

		break;

		case HR_ALM_WRITE :
		if (!button_hr_alm)
		{
			states_HR_ALM = HR_ALM_WAIT;
		}
		else if  (button_hr_alm)
		{
			states_HR_ALM = HR_ALM_INC;
		}

		break;

		case HR_ALM_WAIT :
		if (button_enter_alm)
		{
			states_HR_ALM = HR_ALM_ENTER;
		}
		else if ( (button_hr_alm) && (hr_cnt_alm < 10) )
		{
			states_HR_ALM = HR_ALM_INC;
		}
		else if (!button_enter_alm)
		{
			states_HR_ALM = HR_ALM_WAIT;
		}
		else if ( (button_hr_alm) && (hr_cnt_alm >= 10))
		{
			states_HR_ALM = HR_ALM_INC_10;
		}
		break;

		case HR_ALM_INC_10 :

		if (button_hr_alm)
		{
			states_HR_ALM = HR_ALM_INC;
		}
		else if (!button_hr_alm)
		{
			states_HR_ALM = HR_ALM_WAIT;
		}
		break;

		case HR_ALM_ENTER :

		states_HR_ALM = HR_ALM_ENTER;


		break;

		default :
		states_HR_ALM = INIT_HR_ALM;
		break;

	}

	switch (states_HR_ALM) {

		case START_HR_ALM:
		//LCD_ClearScreen():
		hr_cnt_alm = 0;
		hr_cnt_alm_10 = 0;
		break;

		case HR_ALM_INC :
		hr_cnt_alm = hr_cnt_alm + 1;
		break;

		case HR_ALM_WRITE :
		LCD_Cursor(25);
		LCD_WriteData(hr_cnt_alm + '0') ;
		break;

		case HR_ALM_INC_10 :
		hr_cnt_alm = 0;
		LCD_Cursor(25);
		LCD_WriteData(0 + '0') ;

		hr_cnt_alm_10 = hr_cnt_alm_10 + 1;
		LCD_Cursor(24);
		LCD_WriteData(hr_cnt_alm_10 + '0') ;
		break;

		case HR_ALM_ENTER :
		timer_min_on_alm = 1;
		break;

	}
}


enum Timer_MIN_ALM_States {INIT_MIN_ALM, START_MIN_ALM, MIN_ALM_INC, MIN_ALM_WRITE,
MIN_ALM_WAIT, MIN_ALM_INC_10,  MIN_ALM_ENTER} states_MIN_ALM;

void Timer_MIN_ALM ()
{
	//static unsigned char min_cnt_alm = 0;
	//static unsigned char min_cnt_alm_10 = 0;
	//unsigned char button_min_alm = ~PINA & 0X04;
	unsigned char button_min_alm = ~PINA & 0X08;
	unsigned char button_enter_alm = ~PINA & 0x10;

	switch (states_MIN_ALM)
	{
		case INIT_MIN_ALM :
		if (timer_min_on_alm)
		{
			states_MIN_ALM = START_MIN_ALM;
		}
		else
		{
			states_MIN_ALM = INIT_MIN_ALM ;
		}
		break;

		case START_MIN_ALM :
		if (button_min_alm )
		{
			states_MIN_ALM = MIN_ALM_INC;
		}
		else
		{
			states_MIN_ALM = START_MIN_ALM;
		}
		break;

		case MIN_ALM_INC :
		if ( (min_cnt_alm >= 9 ) && (min_cnt_alm_10 >=5))
		{
			states_MIN_ALM = START_MIN_ALM;
		}
		else if (min_cnt_alm < 10)
		{
			states_MIN_ALM = MIN_ALM_WRITE;
		}
		else if (min_cnt_alm >= 10)
		{
			states_MIN_ALM = MIN_ALM_INC_10;
		}

		break;

		case MIN_ALM_WRITE :
		if (!button_min_alm)
		{
			states_MIN_ALM = MIN_ALM_WAIT;
		}
		else if  (button_min_alm)
		{
			states_MIN_ALM = MIN_ALM_INC;
		}

		break;

		case MIN_ALM_WAIT :
		if (button_enter_alm)
		{
			states_MIN_ALM = MIN_ALM_ENTER;
		}
		else if ( (button_min_alm) && (min_cnt_alm < 10) )
		{
			states_MIN_ALM = MIN_ALM_INC;
		}
		else if (!button_enter_alm)
		{
			states_MIN_ALM = MIN_ALM_WAIT;
		}
		else if ( (button_min_alm) && (min_cnt_alm >= 10))
		{
			states_MIN_ALM = MIN_ALM_INC_10;
		}
		break;

		case MIN_ALM_INC_10 :

		if (button_min_alm)
		{
			states_MIN_ALM = MIN_ALM_INC;
		}
		else if (!button_min_alm)
		{
			states_MIN_ALM = MIN_ALM_WAIT;
		}
		break;

		case MIN_ALM_ENTER :
		states_MIN_ALM = MIN_ALM_ENTER;
		break;

		default :
		states_MIN_ALM = INIT_MIN_ALM;
		break;

	}

	switch (states_MIN_ALM) {

		case START_MIN_ALM:
		min_cnt_alm = 0;
		min_cnt_alm_10 = 0;
		timer_done_alm = 0;
		break;

		case MIN_ALM_INC :
		min_cnt_alm = min_cnt_alm + 1;
		break;

		case MIN_ALM_WRITE :
		LCD_Cursor(28);
		LCD_WriteData(min_cnt_alm + '0') ;
		break;

		case MIN_ALM_INC_10 :
		min_cnt_alm = 0;
		LCD_Cursor(28);
		LCD_WriteData(0 + '0') ;

		min_cnt_alm_10 = min_cnt_alm_10 + 1;
		LCD_Cursor(27);
		LCD_WriteData(min_cnt_alm_10 + '0') ;
		break;

		case MIN_ALM_ENTER :
		timer_done_alm = 1;
		break;

	}
}
enum Ring_States {INIT_RI, CHECK_RI, TURN_ON_RI, OFF_RI, SNOOZE_RI} ring_state ;

void Ring_Fct ()
{
	unsigned char tempB = 0x00;
	static short tick_RI = 0;
	unsigned char button_off = ~PINA & 0x40;
	unsigned char button_snooze = ~PINA & 0x20;
	switch (ring_state)
	{
		case INIT_RI :
		if (timer_done_alm)
		{
			ring_state = CHECK_RI;
		}
		else
		{
			ring_state = INIT_RI;
		}
		break;

		case CHECK_RI :
		if (( hr_cnt_10 == hr_cnt_alm_10 )  && (hr_cnt == hr_cnt_alm) &&
		(min_cnt_10 == min_cnt_alm_10) && (min_cnt == min_cnt_alm))
		{
			ring_state = TURN_ON_RI;
		}
		else{
			ring_state = CHECK_RI;
		}
		break;

		case TURN_ON_RI :
		if (button_off)
		{
			ring_state = OFF_RI;
		}
		else if (button_snooze)
		{
			ring_state = SNOOZE_RI;
		}
		else
		ring_state = TURN_ON_RI;
		break;

		case OFF_RI :
		if (!button_off)
		{
			ring_state = CHECK_RI ;
		}
		else
		{
			ring_state = OFF_RI;
		}
		break;

		case SNOOZE_RI :
		if (tick_RI >= 240)
		{
			ring_state = TURN_ON_RI ;
		}
		else if (tick_RI < 240)
		{
			ring_state= SNOOZE_RI;
		}
		break;

		default :
		ring_state = INIT_RI;
		break;

	}

	switch (ring_state) {
		case INIT_RI :
		tempB = 0x00;
		tick_RI = 0;
		break;

		case TURN_ON_RI:
		tempB = 0x01;
		tick_RI = 0;
		set_PWM (293.66);
		break;

		case OFF_RI :
		tempB = 0x00;
		set_PWM (0);
		break;

		case SNOOZE_RI :
		tick_RI = tick_RI + 1;
		tempB = 0x00;
		set_PWM (0);
		break;
	}

	PORTB = tempB;
}
int main(void) {
	DDRA = 0x00; PORTA = 0xFF;
	DDRB = 0xFF; PORTB = 0x00;
	DDRC = 0xFF; PORTC = 0x00;
	DDRD = 0xFF; PORTD = 0x00;

	states_ST = INIT_ST;
	state_Alm = INIT_ALM;
	LCD_init();

	TimerSet(250);
	TimerOn();

	PWM_on();
	set_PWM (0);

	while(1) {
		Setting_Time();
		Timer_HR ();
		Timer_MIN ();
		Clock_Fct();
		Set_Alarm();
		Timer_HR_ALM();
		Timer_MIN_ALM();
		Ring_Fct();
		while(!TimerFlag);
		TimerFlag = 0;

	}
}
