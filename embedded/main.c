#include "main.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define UBBRVAL 103 //9600 baud, pagina 243 van de datasheet

//1638 led/key
const uint8_t data = 0;
const uint8_t clock = 1;
const uint8_t strobe = 2;
#define HIGH 0x1
#define LOW  0x0



uint8_t receivebuffersize = 50; // Ought to be enough for anybody
uint8_t bufferlocation = 0;
uint8_t autonomemode = 0; // Staat niet in autonome modus
unsigned char receivebuffer[sizeof(char) * 50];

int8_t ondergrenstemperatuur = 0;
int8_t bovengrenstemperatuur = 0;

uint8_t bovengrenslichtintensiteit = 0;
uint8_t ondergrenslichtintensiteit = 0;

uint8_t ondergrensuitrol = 0;
uint8_t bovengrensuitrol = 0;

uint8_t schermuitrol = 0;
uint8_t gewensteschermuitrol = 0;

// EEPROM
uint16_t reboot_counter_ee EEMEM = 0;
uint16_t unique_marker EEMEM = 0; // Als deze niet op een magische waarde is gezet is het een nieuw apparaat


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
// Ontvangt een bericht over UART tot er een \r ontvangen is en stop dit in de receivebuffer
// Geeft 0 terug als het sccesvol was, 1 als het bericht groter is dan de buffer
uint8_t uartReceive()
{
  for (uint8_t i = 0;i < receivebuffersize;i++) {
    // wait for an empty receive buffer
    // UDRE is set when the receive buffer is set
    loop_until_bit_is_set(UCSR0A, RXC0);
    receivebuffer[i] = UDR0;
    if (receivebuffer[i] == '\r') {
      // Einde van de regel, overschrijf \r met een nullbyte
      receivebuffer[i] = '\0';
      return 0;
    }
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
  // Is uart input buffer empty?
  return  UCSR0A & ( 1 << RXC0 );
}

void init_welcome() {
  uint16_t reboots;
  uint16_t magic_marker = eeprom_read_word(&unique_marker);
  if (magic_marker != 31337) {
    // Appraat is nieuw, stel magic marker in en zet reboots op 0
    magic_marker = 31337;
    reboots = 0;
    eeprom_write_word(&unique_marker, magic_marker);
  } else {
    // Apparaat is niet nieuw, increment de reboots timer
    reboots = eeprom_read_word(&reboot_counter_ee);
    reboots++;
  }
  unsigned char reboots_str[50];
  snprintf(reboots_str, 50, "%s%u", "Moi eem\r\nBoot nummer: ", reboots);
  uartSend(reboots_str);
  eeprom_write_word(&reboot_counter_ee, reboots);
}

// Leest het argument uit de receivebuffer en geeft deze terug
int8_t commandArgumentParser() {
  unsigned char argument[4]; // Kan maximaal uit 4 tekens bestaan
  uint8_t argumentpos = 0;
  int8_t argumentint;
  // Kopieer de chars na het = teken over naar argument
  for (uint8_t i = 0;i < receivebuffersize;i++) {
    if (receivebuffer[i] == '=') {
      while (receivebuffer[i] != '\0' && argumentpos < 3) {
        i++;
        argument[argumentpos] = receivebuffer[i];
        argumentpos++;
      }
      argument[argumentpos] = '\0';
      break; // Stop het kopieren
    }
  }
  // Converteer argument naar int
  argumentint = atoi(argument);
  return argumentint;
}

// Leest het argument uit de receivebuffer en geeft deze terug
int8_t commandArgumentParser() {
  unsigned char argument[4]; // Kan maximaal uit 4 tekens bestaan
  uint8_t argumentpos = 0;
  int8_t argumentint;
  // Kopieer de chars na het = teken over naar argument
  for (uint8_t i = 0;i < receivebuffersize;i++) {
    if (receivebuffer[i] == '=') {
      while (receivebuffer[i] != '\0' && argumentpos < 4) {
        i++;
        argument[argumentpos] = receivebuffer[i];
        argumentpos++;
      }
      argument[argumentpos] = '\0';
      break; // Stop het kopieren
    }
  }
  // Converteer argument naar int
  argumentint = atoi(argument);
  return argumentint;
}

// Handel commando's af en stuur een reactie
void handleCommand() {
  unsigned char output[50];
  // Commando's die alleen een status update geven
  if (strcmp(receivebuffer, "!connectie-check") == 0) {
    uartSend("@temperatuur"); // Besturingsunit is een temperatuurmeter
  } else if (strcmp(receivebuffer, "!autonoom") == 0) {
    if (autonomemode == 0) {
      uartSend("@nee");
    } else {
      uartSend("@ja");
    }
  } else if (strcmp(receivebuffer, "!bovengrenstemperatuur") == 0) {
    sprintf(output,"@bovengrenstemperatuur=%d",bovengrenstemperatuur);
    uartSend(output);
  } else if (strcmp(receivebuffer, "!ondergrenstemperatuur") == 0) {
    sprintf(output,"@ondergrenstemperatuur=%d",ondergrenstemperatuur);
    uartSend(output);
  } else if (strcmp(receivebuffer, "!schermuitrol") == 0) {
    sprintf(output,"@schermuitrol=%d",schermuitrol);
    uartSend(output);
  } else if (strcmp(receivebuffer, "!ondergrensuitrol") == 0) {
    sprintf(output,"@ondergrensuitrol=%d",ondergrensuitrol);
    uartSend(output);
  } else if (strcmp(receivebuffer, "!bovengrensuitrol") == 0) {
    sprintf(output,"@bovengrensuitrol=%d",bovengrensuitrol);
    uartSend(output);
  } else if (strcmp(receivebuffer, "!ondergrenslichtintensiteit") == 0) {
    sprintf(output,"@ondergrenslichtintensiteit=%d",ondergrenslichtintensiteit);
    uartSend(output);
  } else if (strcmp(receivebuffer, "!bovengrenslichtintensiteit") == 0) {
    sprintf(output,"@bovengrenslichtintensiteit=%d",bovengrenslichtintensiteit);
    uartSend(output);
  // Commando's die de staat aanpassen
  } else if (strcmp(receivebuffer, "!autonoom=0") == 0) {
    autonomemode = 0;
    uartSend("@succes");
  } else if (strcmp(receivebuffer, "!autonoom=1") == 0) {
    autonomemode = 1;
    uartSend("@succes");
  } else if (strncmp(receivebuffer, "!bovengrenstemperatuur=", 23) == 0) {
    bovengrenstemperatuur = commandArgumentParser();
    uartSend("@succes");
  } else if (strncmp(receivebuffer, "!ondergrenstemperatuur=", 23) == 0) {
    ondergrenstemperatuur = commandArgumentParser();
    uartSend("@succes");
  } else if (strncmp(receivebuffer, "!schermuitrol=", 14) == 0) {
    gewensteschermuitrol = commandArgumentParser();
    uartSend("@succes");
  // Fout commando
  } else {
    uartSend("@ongeldig");
  }
}

// Niet blokkerende ontvanger
void messagehandler() {
  if(uartStatus() != 0) { // Er staat iets in de buffer als het != 0 is
    if(uartReceive() == 0) { // Ontvangst is succesvol
      handleCommand();
    } else { // Bericht is te groot voor de buffer, geef error terug
      uartSend("@fout");
    }
  }
}

// Vraag temperatuur op
// Geeft 20.6 graden terug als 206
int16_t get_temperature() {
  // Selecteer Analoge input 0
  ADMUX = (1<<REFS0)|(1<<ADLAR)|(0<<MUX3)|(0<<MUX2)|(0<<MUX1)|(0<<MUX0);
  ADCSRA |= _BV(ADSC);
  loop_until_bit_is_clear(ADCSRA, ADSC);

  uint16_t analogv = 0;
  analogv = ((ADCH)*(5000/1024));
  int16_t tempinc = 0;
  tempinc = (((analogv)-500)/10);
  return tempinc;
}

// Geef de lichtsterkte terug als 8 bit unsigned integer.
uint8_t get_light() {
  // Selecteer Analoge input 1
  ADMUX = (1<<REFS0)|(1<<ADLAR)|(0<<MUX3)|(0<<MUX2)|(0<<MUX1)|(1<<MUX0);
  ADCSRA |= _BV(ADSC);
  loop_until_bit_is_clear(ADCSRA, ADSC);
  return ADCH ;
}

void send_status_temperature() {
  unsigned char temperature_str[50];
  int16_t temperature_int = get_temperature();
  snprintf(temperature_str, 50, "%s%d", "#temp=", temperature_int);
  uartSend(temperature_str);
}

//T1638 gerelateerde muek

void write1638(uint8_t pin, uint8_t val)
{
    if (val == LOW) {
        PORTB &= ~(_BV(pin)); // clear bit
    } else {
        PORTB |= _BV(pin); // set bit
    }
}

void shiftOut1638 (uint8_t val)
{
    uint8_t i;
    for (i = 0; i < 8; i++)  {
        write1638(clock, LOW);   // bit valid on rising edge
        write1638(data, val & 1 ? HIGH : LOW); // lsb first
        val = val >> 1;
        write1638(clock, HIGH);
    }
}

void sendCommand1638(uint8_t value) {
    write1638(strobe, LOW);
    shiftOut1638(value);
    write1638(strobe, HIGH);
}

void setup1638()
{
  DDRB=0xff; // set port B as output
  sendCommand1638(0x89);  // activate and set brightness to medium
}

void reset1638()
{
  // clear memory - all 16 addresses
  sendCommand1638(0x40); // set auto increment mode
  write1638(strobe, LOW);
  shiftOut1638(0xc0);   // set starting address to 0
  for(uint8_t i = 0; i < 16; i++)
  {
    shiftOut1638(0x00);
  }
  write1638(strobe, HIGH);
}

// Zet de aangegeven hoeveelheid ledjes aan op het bordje om aan te geven hoe ver uitgeschoven het zonnescherm is.
void turnonled1638(uint8_t leds) {
  uint8_t position = 0;
  do {
    sendCommand1638(0x44);
    write1638(strobe, LOW);
    shiftOut1638(0xC0 + (position << 1) + 1);
    shiftOut1638(0x01);
   write1638(strobe, HIGH);
    position++;
  } while (position < leds);
  do {
    sendCommand1638(0x44);
    write1638(strobe, LOW);
    shiftOut1638(0xC0 + (position << 1) + 1);
    shiftOut1638(0x00);
    write1638(strobe, HIGH);
    position++;
  } while (position < 8);

}

// Kijkt naar de huidige en gewenste schermuitrol en past deze 1 stap aan
// De bedoeling is om deze functie iedere seconde aan te roepen vanuit de scheduler
void passchermuitrolaan() {
  if (gewensteschermuitrol > schermuitrol) {
    schermuitrol = schermuitrol + 12;
  } else if (gewensteschermuitrol < schermuitrol) {
    schermuitrol = schermuitrol - 12;
  }
  turnonled1638(schermuitrol / 12);
}

// Neemt beslissing over het aanpasen van de schermuitrol in autonome modus.
void autonoomaanpassenschermuitrol() {
  if (autonomemode == 1) {
    int16_t temperatuur = get_temperature();
    if (temperatuur > ondergrenstemperatuur && temperatuur < bovengrenstemperatuur) {
      gewensteschermuitrol = 100;
    } else {
      gewensteschermuitrol = 0;
    }
  }
}

uint8_t main() {
  uart_init();
  SCH_Init_T1(); // Init de interrupts
  // Init de ADC
  ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);
  // Init de 1638 LED/KEY
  setup1638();
  reset1638();
  // Welkomspraatje en eventueel afhandelen eerste boot
  init_welcome();
  // Insert tasks here
  uint8_t messagehandler_id = SCH_Add_Task(messagehandler,1000,1); // Vuur de messagehandler iedere 1ms af
  uint8_t temperaturehandler_id = SCH_Add_Task(send_status_temperature,1000,2000); //Stuur de temperatuur iedere 40 sec
  SCH_Add_Task(passchermuitrolaan,1000,2000); // Pas de schermuitrol leds iedere seconde aan
  SCH_Add_Task(autonoomaanpassenschermuitrol,10000,2000); // Vraag om de 10 seconde de temperatuur op en pas de schermuitrol aan
  SCH_Start(); // Zet de scheduler aan
  while (1) {
    SCH_Dispatch_Tasks(); // Werklus
  }

  return 0;
}
