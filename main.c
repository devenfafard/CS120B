/*
 * dfafa002_FinalProject.c
 *
 * Created: 5/21/2019 2:53:33 PM
 * Author : Deven Fafard
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <timer.h>
#include <scheduler.h>

#define F_CPU 8000000          //For ATmega1284
#define LED_STRIP_PORT PORTB
#define LED_STRIP_DDR DDRB
#define LED_STRIP_PIN 4
#define LED_COUNT 15

enum LetterStates { INIT, S0, S1, S2, S3 } state;

//Struct to hold RBG values
typedef struct rgb_color
{
	unsigned char red;
	unsigned char green;
	unsigned char blue;
} rgb_color;

//Array of RBG colors - essentially a representation of the LED strip
rgb_color colors[LED_COUNT];

unsigned char A0 = 0x00;
unsigned char A1 = 0x00;
unsigned char A2 = 0x00;
unsigned char A3 = 0x00;

// Below function was sourced from: https://github.com/pololu/pololu-led-strip-avr/blob/master/led_strip.c
#pragma region WRITE_FUNCT
void __attribute__((noinline)) led_strip_write(rgb_color * colors, unsigned int count)
{
	// Set the pin to be an output driving low.
	LED_STRIP_PORT &= ~(1<<LED_STRIP_PIN);
	LED_STRIP_DDR |= (1<<LED_STRIP_PIN);

	cli();   // Disable interrupts temporarily because we don't want our pulse timing to be messed up.
	while(count--)
	{
		// Send a color to the LED strip.
		// The assembly below also increments the 'colors' pointer,
		// it will be pointing to the next color at the end of this loop.
		asm volatile(
		"ld __tmp_reg__, %a0+\n"
		"ld __tmp_reg__, %a0\n"
		"rcall send_led_strip_byte%=\n"  // Send red component.
		"ld __tmp_reg__, -%a0\n"
		"rcall send_led_strip_byte%=\n"  // Send green component.
		"ld __tmp_reg__, %a0+\n"
		"ld __tmp_reg__, %a0+\n"
		"ld __tmp_reg__, %a0+\n"
		"rcall send_led_strip_byte%=\n"  // Send blue component.
		"rjmp led_strip_asm_end%=\n"     // Jump past the assembly subroutines.

		// send_led_strip_byte subroutine:  Sends a byte to the LED strip.
		"send_led_strip_byte%=:\n"
		"rcall send_led_strip_bit%=\n"  // Send most-significant bit (bit 7).
		"rcall send_led_strip_bit%=\n"
		"rcall send_led_strip_bit%=\n"
		"rcall send_led_strip_bit%=\n"
		"rcall send_led_strip_bit%=\n"
		"rcall send_led_strip_bit%=\n"
		"rcall send_led_strip_bit%=\n"
		"rcall send_led_strip_bit%=\n"  // Send least-significant bit (bit 0).
		"ret\n"

		// send_led_strip_bit subroutine:  Sends single bit to the LED strip by driving the data line
		// high for some time.  The amount of time the line is high depends on whether the bit is 0 or 1,
		// but this function always takes the same time (2 us).
		"send_led_strip_bit%=:\n"
		#if F_CPU == 8000000
		"rol __tmp_reg__\n"                      // Rotate left through carry.
		#endif
		"sbi %2, %3\n"                           // Drive the line high.

		#if F_CPU != 8000000
		"rol __tmp_reg__\n"                      // Rotate left through carry.
		#endif

		#if F_CPU == 16000000
		"nop\n" "nop\n"
		#elif F_CPU == 20000000
		"nop\n" "nop\n" "nop\n" "nop\n"
		#elif F_CPU != 8000000
		#error "Unsupported F_CPU"
		#endif

		"brcs .+2\n" "cbi %2, %3\n"              // If the bit to send is 0, drive the line low now.

		#if F_CPU == 8000000
		"nop\n" "nop\n"
		#elif F_CPU == 16000000
		"nop\n" "nop\n" "nop\n" "nop\n" "nop\n"
		#elif F_CPU == 20000000
		"nop\n" "nop\n" "nop\n" "nop\n" "nop\n"
		"nop\n" "nop\n"
		#endif

		"brcc .+2\n" "cbi %2, %3\n"              // If the bit to send is 1, drive the line low now.

		"ret\n"
		"led_strip_asm_end%=: "
		: "=b" (colors)
		: "0" (colors),         // %a0 points to the next color to display
		"I" (_SFR_IO_ADDR(LED_STRIP_PORT)),   // %2 is the port register (e.g. PORTC)
		"I" (LED_STRIP_PIN)     // %3 is the pin number (0-8)
		);

		// Uncomment the line below to temporarily enable interrupts between each color.
		//sei(); asm volatile("nop\n"); cli();
	}
	sei();          // Re-enable interrupts now that we are done.
	_delay_us(80);  // Send the reset signal.
}
#pragma endregion WRITE_FUNCT
	
void SMTick1()
{
	//State transitions
	switch(state)
	{
		case(INIT):
			if(A1)
			{
				state = S0;
			}
			if ((A0 && A1) || (A1 && A2) || (A2 && A3) || (A0 && A2) ||
			(A0 && A3) || (A0 && A1 && A2) || (A1 && A2 && A3) ||
			(A0 && A1 && A3) || (A0 && A2 && A3) || (A0 && A1 && A2 && A3))
			{
				state = INIT;
			}
			break;
			
		case (S0):
			if (A3)
			{
				state = S1;
			}
			if ((A0 && A1) || (A1 && A2) || (A2 && A3) || (A0 && A2) ||
			(A0 && A3) || (A0 && A1 && A2) || (A1 && A2 && A3) ||
			(A0 && A1 && A3) || (A0 && A2 && A3) || (A0 && A1 && A2 && A3))
			{
				state = INIT;
			}
			break;
		
		case(S1):
			if(A0)
			{
				state = S2;
			}
			if ((A0 && A1) || (A1 && A2) || (A2 && A3) || (A0 && A2) ||
			(A0 && A3) || (A0 && A1 && A2) || (A1 && A2 && A3) ||
			(A0 && A1 && A3) || (A0 && A2 && A3) || (A0 && A1 && A2 && A3))
			{
				state = INIT;
			}
			break;
		
		case(S2):
			if (A2)
			{
				state = S3;
			}
			if ((A0 && A1) || (A1 && A2) || (A2 && A3) || (A0 && A2) ||
			(A0 && A3) || (A0 && A1 && A2) || (A1 && A2 && A3) ||
			(A0 && A1 && A3) || (A0 && A2 && A3) || (A0 && A1 && A2 && A3))
			{
				state = INIT;
			}
			break;
			
		case(S3):
			if ((A0 && A1) || (A1 && A2) || (A2 && A3) || (A0 && A2) || 
	            (A0 && A3) || (A0 && A1 && A2) || (A1 && A2 && A3) ||
				(A0 && A1 && A3) || (A0 && A2 && A3) || (A0 && A1 && A2 && A3))
			{
				state = INIT;
			}
			break;
			
		default:
			state = INIT;
			break;
	}
	
	//State functions
	switch(state)
	{
		//For each of the LEDs, set the color, then send color info to strip over PORTB
		case(INIT):
			for(unsigned char i = 0; i < LED_COUNT; ++i)
			{
				//White
				colors[i] = (rgb_color){255, 255, 255};
			}
			led_strip_write(colors, LED_COUNT);
			break;
		
		case(S0):
			for(unsigned char i = 0; i < LED_COUNT; ++i)
			{
				//Green
				colors[i] = (rgb_color){0, 255, 21};
			}
			led_strip_write(colors, LED_COUNT);
			break;
			
		case(S1):
			for(unsigned char i = 0; i < LED_COUNT; ++i)
			{
				//Yellow
				colors[i] = (rgb_color){255, 131, 0};
			}
			led_strip_write(colors, LED_COUNT);
			break;
			
		case(S2):
			for(unsigned char i = 0; i < LED_COUNT; ++i)
			{
				//Blue
				colors[i] = (rgb_color){0, 80, 255};
			}
			led_strip_write(colors, LED_COUNT);
			break;
			
		case(S3):
			for(unsigned char i = 0; i < LED_COUNT; ++i)
			{
				//Pink
				colors[i] = (rgb_color){255, 0, 178};
			}
			led_strip_write(colors, LED_COUNT);
			break;
			
		default:
			state = INIT;
			break;
	}
}

int main(void)
{
	DDRA = 0x00; PINA = 0xFF;
	DDRB = 0xFF; PORTB = 0x00;
	
	unsigned long int SMTick1_calc = 20;
	unsigned long int GCD = 10;
	unsigned long int SMTick1_period = SMTick1_calc / GCD;
	
	static task task1;
	task *tasks[] = { &task1 };
	const unsigned short numTasks = sizeof(tasks) / sizeof(task*);
	
	task1.state = 0;
	task1.period = SMTick1_period;
	task1.elapsedTime = SMTick1_period;
	task1.TickFct = &SMTick1;
		
	TimerSet(GCD);
	TimerOn();
	
	unsigned short i;
	
	while(1)
	{
		A0 = ~PINA & 0x01;
		A1 = ~PINA & 0x02;
		A2 = ~PINA & 0x04;
		A3 = ~PINA & 0x08;

		for(i=0; i < numTasks; i++)
		{
			if(tasks[i]->elapsedTime == tasks[i]->period)
			{
				tasks[i]->state = tasks[i]->TickFct(tasks[i]->state);
				tasks[i]->elapsedTime = 0;
			}
			
			tasks[i]->elapsedTime += 1;
		}
		
		while(!TimerFlag);
		TimerFlag = 0;
	}
	
	return 0;
}

