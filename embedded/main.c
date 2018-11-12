#include "main.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <util/delay.h>

#define UBBRVAL 103 //9600 baud, pagina 243 van de datasheet

// PINOUT aansluiting apparaten op Arduino
// Temperatuur = PC0 (A0)
// Lichtsenson = PC1 (A1)
// TM1638
// Data        = PB0 (8)
// Clock       = PB1 (9)
// Strobe      = PB2 (10)
// Ultrasonic sensor HC-05
// Trig        = PB3 (11)
// Echo        = PD2 (2)

// TM1638 led/key
const uint8_t data = 0;
const uint8_t clock = 1;
const uint8_t strobe = 2;
#define HIGH 0x1
#define LOW  0x0
                   /*0*/  /*1*/   /*2*/  /*3*/  /*4*/  /*5*/  /*6*/  /*7*/   /*8*/  /*9*/  /*off*/
const uint8_t digits[] = { 0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f, 0x0 };
uint8_t statusindicator = 0;



// Ultrasonic sensor HC-05
volatile uint8_t distancewanted = 0;
volatile uint8_t resultready = 0;
volatile uint8_t inprogress = 0;
volatile uint16_t echo = 0;

uint8_t receivebuffersize = 50; // Ought to be enough for anybody
uint8_t bufferlocation = 0;
uint8_t autonomemode = 0; // Staat niet in autonome modus
unsigned char receivebuffer[sizeof(unsigned char) * 50];

int16_t ondergrenstemperatuur = 0;
int16_t bovengrenstemperatuur = 0;

uint8_t bovengrenslichtintensiteit = 0;
uint8_t ondergrenslichtintensiteit = 0;

uint8_t ondergrensuitrol = 100;
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
   SCH_tasks_G[Index].Delay = DELAY;
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
   TIMSK1 |= 1 << OCIE1A;                     // Timer 1 Output Compare A Match Interrupt Enable
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

// Voor ultrasone sensor
ISR(INT0_vect) {
  if(distancewanted == 1 && inprogress == 0 && (PIND & (1<<PD2))) {
    TCCR1B |= (1<<CS11); // Enable Timer
    TCNT1 = 0; // Reset Timer
    inprogress = 1;
    distancewanted = 0;
  } else if(inprogress == 1 && ((PIND & (1<<PD2)) == 0)) {
    TCCR1B &= ~(1<<CS11); // Disable Timer
    echo = TCNT1; // Count echo
    inprogress = 0;
    resultready = 1;
  }
}

ISR(TIMER1_OVF_vect) {
  TCCR1B &= ~(1<<CS11); // Disable Timer
  resultready = 1;
  inprogress = 0;
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

// Returns the amount of numbers in an int
uint8_t countint(uint16_t i) {
  uint8_t amount = 0;
  while (i != 0) {
    amount++;
    i /= 10;
  }
  if (amount == 0) {
    amount = 1; // Int bestaat minstens uit 1 getal
  }
  return amount;
}

uint8_t * intsplitter(uint16_t getal) {
  uint8_t getalsize = countint(getal);
  uint8_t *getalarray = malloc(sizeof(uint8_t) * getalsize);
  while (getalsize--) {
    getalarray[getalsize] = getal % 10;
    getal /= 10;
  }
  return getalarray;
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
int16_t commandArgumentParser() {
  unsigned char argument[10]; // Kan maximaal uit 9 tekens bestaan
  uint8_t argumentpos = 0;
  int16_t argumentint;
  // Kopieer de chars na het = teken over naar argument
  for (uint8_t i = 0;i < receivebuffersize;i++) {
    if (receivebuffer[i] == '=') {
      while (receivebuffer[i] != '\0' && argumentpos < 9) {
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

// Ultrasonic sensor HC-05 gerelateerde muek
void inithc05() {
  DDRB |= 0b00001000; // set port B as output
  TIMSK1 |= (1 << TOIE1); /* Enable Timer1 overflow interrupts */
  TCNT1 = 0;

  EICRA = (1<<ISC00);
  EIMSK = (1<<INT0);
}

// Geeft de afstand in centimeter terug
uint16_t get_distancehc06() {
  distancewanted = 1;
  PORTB |= (1 << PB3);
  _delay_us(10);
  PORTB &= (~(1 << PB3));
  while (resultready == 0); /* Wait for the result to be ready */
  return echo / 15;
}
// Einde HC-05 ultrasoonsensor muek

// Geef de lichtsterkte terug als 8 bit unsigned integer.
// Tussen de 0 en 100 in, 0 is minimaal, 100 is maximaal
uint8_t get_light() {
  // Selecteer Analoge input 1
  ADMUX = (1<<REFS0)|(1<<ADLAR)|(0<<MUX3)|(0<<MUX2)|(0<<MUX1)|(1<<MUX0);
  ADCSRA |= _BV(ADSC);
  loop_until_bit_is_clear(ADCSRA, ADSC);
  uint8_t percentage;
  percentage = ((uint32_t)ADCH*100)/255;
  return percentage;
}

// Vraag temperatuur op
// Geeft 20.6 graden terug als 206
int16_t get_temperature() {
  // Selecteer Analoge input 0
  ADMUX = (1<<REFS0)|(0<<ADLAR)|(0<<MUX3)|(0<<MUX2)|(0<<MUX1)|(0<<MUX0);
  ADCSRA |= _BV(ADSC);
  loop_until_bit_is_clear(ADCSRA, ADSC);

  uint16_t analogv = 0;
  analogv = ((uint32_t)ADC*5000)/1024;
  int16_t tempinc = 0;
  tempinc = (analogv - 500);
  return tempinc;
}

// Handel commando's af en stuur een reactie
void handleCommand() {
  unsigned char output[50];
  // Commando's die alleen een status update geven
  if (strcmp(receivebuffer, "!connectie-check") == 0) {
    snprintf(output, 50, "%s", "@connectie-check=succes");
  } else if (strcmp(receivebuffer, "!autonoom") == 0) {
    if (autonomemode == 0) {
      snprintf(output, 50, "%s", "@autonoom=0");
    } else {
      snprintf(output, 50, "%s", "@autonoom=1");
    }
  } else if (strcmp(receivebuffer, "!bovengrenstemperatuur") == 0) {
    sprintf(output,"@bovengrenstemperatuur=%d",bovengrenstemperatuur);
  } else if (strcmp(receivebuffer, "!ondergrenstemperatuur") == 0) {
    sprintf(output,"@ondergrenstemperatuur=%d",ondergrenstemperatuur);
  } else if (strcmp(receivebuffer, "!schermuitrol") == 0) {
    sprintf(output,"@schermuitrol=%d",schermuitrol);
  } else if (strcmp(receivebuffer, "!ondergrensuitrol") == 0) {
    sprintf(output,"@ondergrensuitrol=%d",ondergrensuitrol);
  } else if (strcmp(receivebuffer, "!bovengrensuitrol") == 0) {
    sprintf(output,"@bovengrensuitrol=%d",bovengrensuitrol);
  } else if (strcmp(receivebuffer, "!ondergrenslichtintensiteit") == 0) {
    sprintf(output,"@ondergrenslichtintensiteit=%d",ondergrenslichtintensiteit);
  } else if (strcmp(receivebuffer, "!bovengrenslichtintensiteit") == 0) {
    sprintf(output,"@bovengrenslichtintensiteit=%d",bovengrenslichtintensiteit);
  } else if (strcmp(receivebuffer, "!afstand") == 0) {
    uint16_t distance_int = get_distancehc06();
    snprintf(output, 50, "%s%d", "#afstand=", distance_int);
  } else if (strcmp(receivebuffer, "!licht") == 0) {
    uint8_t light_int = get_light();
    snprintf(output, 50, "%s%u", "@licht=", light_int);
  } else if (strcmp(receivebuffer, "!temperatuur") == 0) {
    int16_t temperature_int = get_temperature();
    snprintf(output, 50, "%s%d", "@temperatuur=", temperature_int);
  // Commando's die de staat aanpassen
  } else if (strcmp(receivebuffer, "!autonoom=0") == 0) {
    autonomemode = 0;
    snprintf(output, 50, "%s", "@autonoom=succes");
  } else if (strcmp(receivebuffer, "!autonoom=1") == 0) {
    autonomemode = 1;
    snprintf(output, 50, "%s", "@autonoom=succes");
  } else if (strncmp(receivebuffer, "!bovengrenstemperatuur=", 23) == 0) {
    bovengrenstemperatuur = commandArgumentParser();
    snprintf(output, 50, "%s", "@bovengrenstemperatuur=succes");
  } else if (strncmp(receivebuffer, "!ondergrenstemperatuur=", 23) == 0) {
    ondergrenstemperatuur = commandArgumentParser();
    snprintf(output, 50, "%s", "@ondergrenstemperatuur=succes");
  } else if (strncmp(receivebuffer, "!schermuitrol=", 14) == 0) {
    uint8_t schermuitrolint = commandArgumentParser();
    if (schermuitrolint >= 0 && schermuitrolint <= 100) {
      gewensteschermuitrol = schermuitrolint;
      snprintf(output, 50, "%s", "@schermuitrol=succes");
    } else {
      snprintf(output, 50, "%s", "@ongeldig");
    }
  // Fout commando
  } else {
    snprintf(output, 50, "%s", "@ongeldig");
  }
  uartSend(output);
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

void send_status_temperature() {
  unsigned char temperature_str[50];
  int16_t temperature_int = get_temperature();
  snprintf(temperature_str, 50, "%s%d", "#temp=", temperature_int);
  uartSend(temperature_str);
}

void send_status_light() {
  unsigned char light_str[50];
  uint8_t light_int = get_light();
  snprintf(light_str, 50, "%s%u", "#licht=", light_int);
  uartSend(light_str);
}

// TM1638 gerelateerde muek
void write1638(uint8_t pin, uint8_t val) {
    if (val == LOW) {
        PORTB &= ~(_BV(pin)); // clear bit
    } else {
        PORTB |= _BV(pin); // set bit
    }
}

void shiftOut1638 (uint8_t val) {
    uint8_t i;
    for (i = 0; i < 8; i++)  {
        write1638(clock, LOW); // bit valid on rising edge
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

void setup1638() {
  DDRB |= 0b00000111; // set port B as output
  sendCommand1638(0x89);  // activate and set brightness to medium
}

void reset1638() {
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
  uint8_t value = 0x01; // Led aan
  do {
    if (position == leds) {
      value = 0x0; // Zet de leds uit
    }
    sendCommand1638(0x44);
    write1638(strobe, LOW);
    shiftOut1638(0xC0 + (position << 1) + 1);
    shiftOut1638(value);
    write1638(strobe, HIGH);
    position++;
  } while (position < 8);
}

// Handelt het printen van temperaturen af voor het 1638 scherm
void temperaturehandler1638(uint8_t *display, int16_t stemperatuur) {
  uint16_t temperatuur;
  uint8_t displayposition;
  if (stemperatuur < 0) { // Als de temperatuur negatief is, print een - teken en maak het getal positief
    temperatuur = abs(stemperatuur);
    displayposition = 6 - countint(temperatuur); // Right allign het getal
    display[displayposition] = 0b01000000; // min teken voor negatieve temperaturen
    displayposition++;
  } else {
    temperatuur = (uint16_t)stemperatuur;
    displayposition = 7 - countint(temperatuur); // Right allign het getal
  }
  uint8_t * temparray = intsplitter(temperatuur);
  for (uint8_t loc = 0;loc < countint(temperatuur);loc++) {
    uint8_t number = temparray[loc];
    display[displayposition] = digits[number];
    if (countint(temperatuur) - loc == 2) {
      display[displayposition] |= 0b10000000; // decimaal punt
    }
    displayposition++;
  }
  free(temparray);
  display[displayposition] |= 0b01011000; // Graden celcius
  displayposition++;
}

void textstatus1638() {
  uint8_t position = 0;
  uint8_t display[8] = { 0 }; // Een byte voor elke indicator
  uint8_t displayposition = 0;

  // statusindicator 0 == Temperature indication
  if (statusindicator == 0) {
    int16_t stemperatuur = get_temperature();
    display[displayposition++] = 0b01111000; // Teken iets dat op een T lijkt
    display[displayposition++] = 0b01110011; // P

    temperaturehandler1638(display, stemperatuur);

    statusindicator++;
  }
  // statusindicator 1 == Licht indicatie
  else if (statusindicator == 1) {
    uint8_t light = get_light();
    display[displayposition++] = 0b00111010; // L i
    display[displayposition++] = 0b00111001; // C
    display[displayposition++] = 0b01110110; // H
    display[displayposition++] = 0b01111000; // T
    displayposition = 8 - countint(light); // Right allign het getal
    uint8_t * lichtarray = intsplitter(light);
    for (uint8_t loc = 0;loc < countint(light);loc++) {
      display[displayposition++] = digits[lichtarray[loc]];
    }
    free(lichtarray);

    statusindicator++;
  } else if (statusindicator == 2) {
    uint16_t distance = get_distancehc06();
    display[displayposition++] = 0b01110111; // A
    display[displayposition++] = 0b01110001; // F
    display[displayposition++] = 0b01101101; // S
    display[displayposition++] = 0b01111000; // T
    displayposition = 8 - countint(distance); // Right allign het getal
    uint8_t * afstandarray = intsplitter(distance);
    for (uint8_t loc = 0;loc < countint(distance);loc++) {
      display[displayposition++] = digits[afstandarray[loc]];
    }
    free(afstandarray);

    statusindicator++;
  // Laat de autonome mode instelling zien
  } else if (statusindicator == 3) {
    display[displayposition++] = 0b01110111; // A
    display[displayposition++] = 0b00111110; // U
    display[displayposition++] = 0b01111000; // T
    display[displayposition++] = 0b00111111; // O
    displayposition++;
    if (autonomemode == 0) {
      display[displayposition++] = 0b00111110; // U
      display[displayposition++] = 0b00110000; // I
      display[displayposition++] = 0b01111000; // T
    } else {
      display[displayposition++] = 0b01110111; // A
      display[displayposition++] = 0b01110111; // A
      display[displayposition++] = 0b01010100; // N
    }
  statusindicator++;
  // Laat bovengrens temperatuurinstelling zien
  } else if (statusindicator == 4) {
    display[displayposition++] = 0b01111000; // T
    display[displayposition++] = 0b01110011; // P
    display[displayposition++] = 0b00000001; // Hoog streepje
    temperaturehandler1638(display, bovengrenstemperatuur);
    statusindicator++;
  // Laat ondergrens temperatuurinstelling zien
  } else if (statusindicator == 5) {
    display[displayposition++] = 0b01111000; // T
    display[displayposition++] = 0b01110011; // P
    display[displayposition++] = 0b00001000; // Laag streepje
    temperaturehandler1638(display, ondergrenstemperatuur);
    statusindicator = 0;
  }
  while (position < 8) {
    sendCommand1638(0x44);
    write1638(strobe, LOW);
    shiftOut1638(0xC0 + (position << 1));
    shiftOut1638(display[position]);
    write1638(strobe, HIGH);
    position++;
  }
}

// Einde TM1638 gerelateerde muek

// Kijkt naar de huidige en gewenste schermuitrol en past deze 1 stap aan
void passchermuitrolaan() {
  if (gewensteschermuitrol > schermuitrol && schermuitrol < 100) {
    schermuitrol = schermuitrol + 1;
    turnonled1638(schermuitrol / 12);
  } else if (gewensteschermuitrol < schermuitrol && schermuitrol > 0 ) {
    schermuitrol = schermuitrol - 1;
    turnonled1638(schermuitrol / 12);
  }
}

// Neemt beslissing over het aanpasen van de schermuitrol in autonome modus.
void autonoomaanpassenschermuitrol() {
  if (autonomemode == 1) {
    int16_t temperatuur = get_temperature();
    uint8_t licht = get_light();
    if (temperatuur > ondergrenstemperatuur && temperatuur < bovengrenstemperatuur) {
      gewensteschermuitrol = 100;
    } else if (licht > ondergrenslichtintensiteit && licht < bovengrenslichtintensiteit) {
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
  // Init de Ultrasonic sensor HC-05
  inithc05();
  // Welkomspraatje en eventueel afhandelen eerste boot
  init_welcome();
  // Insert tasks here
  uint8_t messagehandler_id = SCH_Add_Task(messagehandler,1000,1); // Vuur de messagehandler iedere 1ms af
  SCH_Add_Task(send_status_temperature,1000,40000); //Stuur de temperatuur iedere 40 sec
  SCH_Add_Task(send_status_light,1000,60000); //Stuur de lichtsterkte iedere 60 sec
  SCH_Add_Task(passchermuitrolaan,1000,100); // Pas de schermuitrol leds 10 keer per seconde aan
  SCH_Add_Task(autonoomaanpassenschermuitrol,1000,10000); // Vraag om de 10 seconde de temperatuur op en pas de schermuitrol aan
  SCH_Add_Task(textstatus1638,1000,3000); // Update het scherm iedere 3 sec met andere info
  SCH_Start(); // Zet de scheduler aan
  while (1) {
    SCH_Dispatch_Tasks(); // Werklus
  }

  return 0;
}
