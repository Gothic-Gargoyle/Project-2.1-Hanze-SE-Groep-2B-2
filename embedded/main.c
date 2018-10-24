#include "main.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define UBBRVAL 103 //9600 baud, pagina 243 van de datasheet

uint8_t receivebuffersize = 50; // Ought to be enough for anybody
unsigned char receivebuffer[sizeof(char) * 50];

// The array of tasks
sTask SCH_tasks_G[SCH_MAX_TASKS];


/*------------------------------------------------------------------*-

  SCH_Dispatch_Tasks()

  This is the 'dispatcher' function.  When a task (function)
  is due to run, SCH_Dispatch_Tasks() will run it.
  This function must be called (repeatedly) from the main loop.

-*------------------------------------------------------------------*/

void SCH_Dispatch_Tasks(void)
{
   unsigned char Index;

   // Dispatches (runs) the next task (if one is ready)
   for(Index = 0; Index < SCH_MAX_TASKS; Index++)
   {
      if((SCH_tasks_G[Index].RunMe > 0) && (SCH_tasks_G[Index].pTask != 0))
      {
         (*SCH_tasks_G[Index].pTask)();  // Run the task
         SCH_tasks_G[Index].RunMe -= 1;   // Reset / reduce RunMe flag

         // Periodic tasks will automatically run again
         // - if this is a 'one shot' task, remove it from the array
         if(SCH_tasks_G[Index].Period == 0)
         {
            SCH_Delete_Task(Index);
         }
      }
   }
}

/*------------------------------------------------------------------*-

  SCH_Add_Task()

  Causes a task (function) to be executed at regular intervals 
  or after a user-defined delay

  pFunction - The name of the function which is to be scheduled.
              NOTE: All scheduled functions must be 'void, void' -
              that is, they must take no parameters, and have 
              a void return type. 
                   
  DELAY     - The interval (TICKS) before the task is first executed

  PERIOD    - If 'PERIOD' is 0, the function is only called once,
              at the time determined by 'DELAY'.  If PERIOD is non-zero,
              then the function is called repeatedly at an interval
              determined by the value of PERIOD (see below for examples
              which should help clarify this).


  RETURN VALUE:  

  Returns the position in the task array at which the task has been 
  added.  If the return value is SCH_MAX_TASKS then the task could 
  not be added to the array (there was insufficient space).  If the
  return value is < SCH_MAX_TASKS, then the task was added 
  successfully.  

  Note: this return value may be required, if a task is
  to be subsequently deleted - see SCH_Delete_Task().

  EXAMPLES:

  Task_ID = SCH_Add_Task(Do_X,1000,0);
  Causes the function Do_X() to be executed once after 1000 sch ticks.            

  Task_ID = SCH_Add_Task(Do_X,0,1000);
  Causes the function Do_X() to be executed regularly, every 1000 sch ticks.            

  Task_ID = SCH_Add_Task(Do_X,300,1000);
  Causes the function Do_X() to be executed regularly, every 1000 ticks.
  Task will be first executed at T = 300 ticks, then 1300, 2300, etc.            
 
-*------------------------------------------------------------------*/

unsigned char SCH_Add_Task(void (*pFunction)(), const unsigned int DELAY, const unsigned int PERIOD)
{
   unsigned char Index = 0;

   // First find a gap in the array (if there is one)
   while((SCH_tasks_G[Index].pTask != 0) && (Index < SCH_MAX_TASKS))
   {
      Index++;
   }

   // Have we reached the end of the list?   
   if(Index == SCH_MAX_TASKS)
   {
      // Task list is full, return an error code
      return SCH_MAX_TASKS;  
   }

   // If we're here, there is a space in the task array
   SCH_tasks_G[Index].pTask = pFunction;
   SCH_tasks_G[Index].Delay =DELAY;
   SCH_tasks_G[Index].Period = PERIOD;
   SCH_tasks_G[Index].RunMe = 0;

   // return position of task (to allow later deletion)
   return Index;
}

/*------------------------------------------------------------------*-

  SCH_Delete_Task()

  Removes a task from the scheduler.  Note that this does
  *not* delete the associated function from memory: 
  it simply means that it is no longer called by the scheduler. 
 
  TASK_INDEX - The task index.  Provided by SCH_Add_Task(). 

  RETURN VALUE:  RETURN_ERROR or RETURN_NORMAL

-*------------------------------------------------------------------*/

unsigned char SCH_Delete_Task(const unsigned char TASK_INDEX)
{
   // Return_code can be used for error reporting, NOT USED HERE THOUGH!
   unsigned char Return_code = 0;

   SCH_tasks_G[TASK_INDEX].pTask = 0;
   SCH_tasks_G[TASK_INDEX].Delay = 0;
   SCH_tasks_G[TASK_INDEX].Period = 0;
   SCH_tasks_G[TASK_INDEX].RunMe = 0;

   return Return_code;
}

/*------------------------------------------------------------------*-

  SCH_Init_T1()

  Scheduler initialisation function.  Prepares scheduler
  data structures and sets up timer interrupts at required rate.
  You must call this function before using the scheduler.  

-*------------------------------------------------------------------*/

void SCH_Init_T1(void)
{
   unsigned char i;

   for(i = 0; i < SCH_MAX_TASKS; i++)
   {
      SCH_Delete_Task(i);
   }

   // Set up Timer 1
   // Values for 1ms and 10ms ticks are provided for various crystals

   // Hier moet de timer periode worden aangepast ....!
   OCR1A = (uint16_t)15999;                  // 1ms = 16000000 / (1 * 1000) - 1
   TCCR1B = (1 << CS10) | (1 << WGM12);  // prescale op 1, top counter = value OCR1A (CTC mode)
   TIMSK1 = 1 << OCIE1A;                     // Timer 1 Output Compare A Match Interrupt Enable
}

/*------------------------------------------------------------------*-

  SCH_Start()

  Starts the scheduler, by enabling interrupts.

  NOTE: Usually called after all regular tasks are added,
  to keep the tasks synchronised.

  NOTE: ONLY THE SCHEDULER INTERRUPT SHOULD BE ENABLED!!! 
 
-*------------------------------------------------------------------*/

void SCH_Start(void)
{
      sei();
}

/*------------------------------------------------------------------*-

  SCH_Update

  This is the scheduler ISR.  It is called at a rate 
  determined by the timer settings in SCH_Init_T1().

-*------------------------------------------------------------------*/

ISR(TIMER1_COMPA_vect)
{
   unsigned char Index;
   for(Index = 0; Index < SCH_MAX_TASKS; Index++)
   {
      // Check if there is a task at this location
      if(SCH_tasks_G[Index].pTask)
      {
         if(SCH_tasks_G[Index].Delay == 0)
         {
            // The task is due to run, Inc. the 'RunMe' flag
            SCH_tasks_G[Index].RunMe += 1;

            if(SCH_tasks_G[Index].Period)
            {
               // Schedule periodic tasks to run again
               SCH_tasks_G[Index].Delay = SCH_tasks_G[Index].Period;
               SCH_tasks_G[Index].Delay -= 1;
            }
         }
         else
         {
            // Not yet ready to run: just decrement the delay
            SCH_tasks_G[Index].Delay -= 1;
         }
      }
   }
}

void uart_init()
{
  //set the baud rate
  UBRR0H = 0;
  UBRR0L = UBBRVAL;

  //disable U2X mode
  UCSR0A = 0;

  //enable receiver & transmitter
  UCSR0B = _BV(RXEN0) | _BV(TXEN0);

  //set frame format : asynchronous, 8 data bits, 1 stop bit, no parity
  UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);
}

// ------------------------------------------------------------------
// Ontvangt een bericht over de seriele verbinding en stopt het in een buffer
// Geeft 0 terug bij succes, 1 als het bericht groter was dan de buffer
uint8_t uartReceive(unsigned char *buffer, uint8_t buffersize)
{
  unsigned char c;
  for(uint8_t i = 0;i < buffersize;i++) {
    //  wait for an empty receive buffer
    //  UDRE is set when the receive buffer is set
    loop_until_bit_is_set(UCSR0A, RXC0);
    c = UDR0;
    if (c == '\n' || c == '\r') {
      // Einde van de regel, voeg handmatig een nullbyte toe
      buffer[i] = '\0';
      return 0;
    }
    buffer[i] = c;

  }
  return 1;
}

// Stuurt een bericht over de seriele poort heen. String moet getermineerd zijn met een nullcharacter
void uartSend(unsigned char *buffer) {
  for (uint8_t i = 0;i < strlen(buffer) + 1;i++) {
    // wait until usart buffer is empty
    while ( bit_is_clear(UCSR0A, UDRE0) );
    // Stop met uitsturen bij een nullbyte
    if (buffer[i] == '\0') {
      // Gratis cr/linefeed
      UDR0 = '\r';
      while ( bit_is_clear(UCSR0A, UDRE0) );
      UDR0 = '\n';
      return;
    }

    // send the data
    UDR0 = buffer[i];
  }
}

unsigned char uartStatus() {
	//Is uart input buffer empty?

	return  UCSR0A & ( 1 << RXC0 );
}

void print_about_serial() {
  char *message = "Temperatuurmeetsensor v0.1\r\nDoor: Jelle Kaufmann";
  uartSend(message);
}

void print_test_serial() {
  char *message = "Testbericht uit scheduler";
  uartSend(message);
}

// Kijkt of er iets in de UART buffer staat en zal dat bericht afhandelen
void messagehandler() {
  if(uartStatus() != 0) { // Er staat iets in de buffer als het != 0 is
    if (uartReceive(receivebuffer, receivebuffersize) != 0) {
      char *message = "Bericht te groot voor de buffer";
      uartSend(message);
    } else {
      uartSend(receivebuffer);
    }
  }
}

uint8_t main()
{
  uart_init();
  SCH_Init_T1(); // Init de interrupts
  // Insert tasks here
  SCH_Add_Task(print_about_serial,1000,0); // Welkomstpraatje na 1 sec weergeven
  uint8_t messagehandler_id = SCH_Add_Task(messagehandler,1200,100); // Vuur de messagehandler iedere 100ms af
  uint8_t test_id = SCH_Add_Task(print_test_serial,1200,200);

  SCH_Start(); // Zet de scheduler aan
  while (1) {
    SCH_Dispatch_Tasks(); // Werklus
  }

  return 0;
}
