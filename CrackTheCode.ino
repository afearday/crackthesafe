#include <LiquidCrystal_I2C.h>



//Encoder interrupt routine adapted from Simon Merrett's example code

#include <SPI.h>                          //Import libraries to control the OLED display
#include <Wire.h>
#include <Servo.h>                        //Import library to control the servo

Servo lockServo;                          //Create a servo object for the lock servo

LiquidCrystal_I2C lcd(0x27, 16, 2);

static int pinA = 2;                      //Hardware interrupt digital pin 2
static int pinB = 3;                      //Hardware interrupt digital pin 3
volatile byte aFlag = 0;                  //Rising edge on pinA to signal that the encoder has arrived at a detent
volatile byte bFlag = 0;                  //Rising edge on pinB to signal that the encoder has arrived at a detent (opposite direction to when aFlag is set)
volatile byte encoderPos = 0;             //Current value of encoder position, digit being input form 0 to 9
volatile byte prevEncoderPos = 0;         //To track whether the encoder has been turned and the display needs to update
volatile byte reading = 0;                //Stores direct value from interrupt pin

const byte buttonPin = 4;                 //Pin number for encoder push button
byte oldButtonState = HIGH;               //First button state is open because of pull-up resistor
const unsigned long debounceTime = 10;    //Debounce delay time
unsigned long buttonPressTime;            //Time button has been pressed for debounce

byte correctNumLEDs[4] = {9,12,7,11};      //Pin numbers for correct number LEDs (Indicate a correct digit)
byte correctPlaceLEDs[4] = {6,10,8,13};    //Pin numbers for correct place LEDs (Indicate a correct digit in the correct place)

byte code[4] = {0,0,0,0};                  //Create an array to store the code digits
byte codeGuess[4] = {0,0,0,0};             //Create an array to store the guessed code digits
byte guessingDigit = 0;                    //Tracks the current digit being guessed
byte numGuesses = 0;                       //Tracks how many guesses it takes to crack the code
boolean correctGuess = true;               //Variable to check whether the code has been guessed correctly, true initially to generate a new code on startup

void setup()
{
  Serial.begin(9600);                                 //Starts the Serial monitor for debugging
  if(!lcd.begin{
    Serial.println(F("SSD1306 allocation failed"));   //If connection fails
    for(;;);                                          //Don't proceed, loop forever
  }
  lcd.clearDisplay();                             //Clear display
  lockServo.attach(5);                                //Assign the lock servo to pin 5
  for(int i=0 ; i<=3 ; i++)                           //Define pin modes for the LEDs
  {
    pinMode(correctNumLEDs[i], OUTPUT);
    pinMode(correctPlaceLEDs[i], OUTPUT);
  }
  pinMode(pinA, INPUT_PULLUP);                        //Set pinA as an input, pulled HIGH to the logic voltage
  pinMode(pinB, INPUT_PULLUP);                        //Set pinB as an input, pulled HIGH to the logic voltage
  attachInterrupt(0,PinA,RISING);                     //Set an interrupt on PinA
  attachInterrupt(1,PinB,RISING);                     //Set an interrupt on PinB
  pinMode (buttonPin, INPUT_PULLUP);                  //Set the encoder button as an input, pulled HIGH to the logic voltage
  randomSeed(analogRead(0));                          //Randomly choose a starting point for the random function, otherwise code pattern is predictable
  startupAni();                                       //Display the startup animation
}

void loop() 
{
  if(correctGuess)                                            //Code between games to reset if the guess is correct, initially true to open safe and then generate new code
  {
    lockServo.write(140);                                     //Unlock the safe
    delay(300);
    updateLEDs (0,4);                                         //Flashing LED sequence
    delay(300);
    updateLEDs (4,0);
    delay(300);
    updateLEDs (0,4);
    delay(300);
    updateLEDs (4,0);
    delay(300);
    updateLEDs (4,4);                                         //Turn all LEDs on
    if(numGuesses >= 1)                                       //Check that its not the start of the game
    {
      lcd.clearDisplay();                                 //Clear the display
      lcd.setCursor(35,10);                               //Set the display cursor position
      lcd.print(F("In "));                                //Set the display text
      lcd.print(numGuesses);                              //Set the display text
      lcd.setCursor(35,20);                               //Set the display cursor position
      lcd.print(F("Attempts"));                           //Set the display text
      lcd.display();                                      //Output the display text
      delay(5000);
    }
    lcd.clearDisplay();                                   //Clear the display
    lcd.setCursor(35,10);                                 //Set the display cursor position
    lcd.print(F("Push To"));                              //Set the display text
    lcd.setCursor(35,20);                                 //Set the display cursor position
    lcd.print(F("Lock Safe"));                            //Set the display text
    lcd.display();                                        //Output the display text
    boolean lock = false;                                     //Safe is initially not locked
    boolean pressed = false;                                  //Keeps track of button press
    while(!lock)                                              //While button is not pressed, wait for it to be pressed
    {
      byte buttonState = digitalRead (buttonPin); 
      if (buttonState != oldButtonState)
      {
        if (millis () - buttonPressTime >= debounceTime)      //Debounce button
        {
          buttonPressTime = millis ();                        //Time when button is pressed
          oldButtonState =  buttonState;                      //Remember button state
          if (buttonState == LOW)
          {
            pressed = true;                                   //Records button has been pressed
          }
          else 
          {
            if (pressed == true)                              //Makes sure that button is pressed and then released before continuing in the code
            {
              lockServo.write(45);                            //Lock the safe
              lcd.clearDisplay();                         //Clear the display
              lcd.setCursor(30,10);                       //Set the display cursor position
              lcd.print(F("Locked"));                     //Set the display text
              lcd.display();                              //Output the display text
              lock = true;
            }
          }  
        }
      }
    }
    generateNewCode();                                        //Calls function to generate a new random code
    updateLEDs (0,0);
    correctGuess = false;                                     //The code guess is initially set to incorrect
    numGuesses = 0;                                           //Reset the number of guesses counter
  }
  inputCodeGuess();                                           //Calls function to allow the user to input a guess
  numGuesses++;                                               //Increment the guess counter
  checkCodeGuess();                                           //Calls function to check the input guess
  encoderPos = 0;                                             //Reset the encoder position
  guessingDigit = 0;                                          //Reset the digit being guessed
  codeGuess[0] = 0;                                           //Reset the first digit of the code
  updateDisplayCode();                                        //Update the displayed code
}

void updateDisplayCode()                                      //Function to update the display with the input code
{
  String temp = "";                                           //Temporary variable to concatenate the code string
  if(!correctGuess)                                           //If the guess is not correct then update the display
  {
    for (int i=0 ; i<guessingDigit ; i++)                     //Loops through the four digits to display them
    {
      temp = temp + codeGuess[i];
    }
    temp = temp + encoderPos;
    for (int i=guessingDigit+1 ; i<=3 ; i++)
    {
      temp = temp + "0";
    }
    Serial.println(temp);                                     //Output to Serial monitor for debugging
    lcd.clearDisplay();                                   //Clear the display
    lcd.setCursor(40,10);                                 //Set the display cursor position
    lcd.println(temp);                                    //Set the display text
    lcd.display();                                        //Update the display
  }
}

void generateNewCode()                                        //Function to generate a new random code
{
  Serial.print("Code: ");
  for (int i=0 ; i<= 3 ; i++)                                 //Loops through the four digits and assigns a random number to each
  {
    code[i] = random(0,9);                                    //Generate a random number for each digit
    Serial.print(code[i]);                                    //Display the code on Serial monitor for debugging
  }
  Serial.println();
}

void inputCodeGuess()                                         //Function to allow the user to input a guess
{
  for(int i=0 ; i<=3 ; i++)                                   //User must guess all four digits
  {
    guessingDigit = i;
    boolean confirmed = false;                                //Both used to confirm button push to assign a digit to the guess code
    boolean pressed = false;
    encoderPos = 0;                                           //Encoder starts from 0 for each digit
    while(!confirmed)                                         //While the user has not confirmed the digit input
    {
      byte buttonState = digitalRead (buttonPin); 
      if (buttonState != oldButtonState)
      {
        if (millis () - buttonPressTime >= debounceTime)      //Debounce button
        {
          buttonPressTime = millis ();                        //Time when button was pushed
          oldButtonState =  buttonState;                      //Remember button state for next time
          if (buttonState == LOW)
          {
            codeGuess[i] = encoderPos;                        //If the button is pressed, accept the current digit into the guessed code
            pressed = true;
          }
          else 
          {
            if (pressed == true)                              //Confirm the input once the button is released again
            {
              updateDisplayCode();                            //Update the code being displayed
              confirmed = true;
            }
          }  
        }
      }
      if(encoderPos!=prevEncoderPos)                          //Update the displayed code if the encoder position has changed
      {
        updateDisplayCode();
        prevEncoderPos=encoderPos;
      }
    }
  }
}

void checkCodeGuess()                                         //Function to check the users guess against the generated code
{
  int correctNum = 0;                                         //Variable for the number of correct digits in the wrong place
  int correctPlace = 0;                                       //Variable for the number of correct digits in the correct place
  int usedDigits[4] = {0,0,0,0};                              //Mark off digits which have been already identified in the wrong place, avoids counting repeated digits twice
  for (int i=0 ; i<= 3 ; i++)                                 //Loop through the four digits in the guessed code
  {
    for (int j=0 ; j<=3 ; j++)                                //Loop through the four digits in the generated code
    {
      if (codeGuess[i]==code[j])                              //If a number is found to match
      {
        if(usedDigits[j]!=1)                                  //Check that it hasn't been previously identified
        {
          correctNum++;                                       //Increment the correct digits in the wrong place counter
          usedDigits[j] = 1;                                  //Mark off the digit as been identified
          break;                                              //Stop looking once the digit is found
        }
      }
    }
  }
  for (int i=0 ; i<= 3 ; i++)                                 //Compares the guess digits to the code digits for correct digits in correct place
  {
    if (codeGuess[i]==code[i])                                //If a correct digit in the correct place is found
      correctPlace++;                                         //Increment the correct place counter
  }
  updateLEDs(correctNum, correctPlace);                        //Calls a function to update the LEDs to reflect the guess
  if(correctPlace==4)                                          //If all 4 digits are correct then the code has been cracked
  {
    lcd.clearDisplay();                                    //Clear the display
    lcd.setCursor(20,10);                                  //Set the display cursor position
    lcd.print(F("Cracked"));                               //Set the display text
    lcd.display();                                         //Output the display text
    correctGuess = true;
  }
  else
    correctGuess = false;
}

void updateLEDs (int corNum, int corPla)                        //Function to update the LEDs to reflect the guess
{
  for(int i=0 ; i<=3 ; i++)                                     //First turn all LEDs off
  {
    digitalWrite(correctNumLEDs[i], LOW);
    digitalWrite(correctPlaceLEDs[i], LOW);
  }
  for(int j=0 ; j<=corNum-1 ; j++)                              //Turn on the number of correct digits in wrong place LEDs
  {
    digitalWrite(correctNumLEDs[j], HIGH);
  }
  for(int k=0 ; k<=corPla-1 ; k++)                              //Turn on the number of correct digits in the correct place LEDs
  {
    digitalWrite(correctPlaceLEDs[k], HIGH);
  }
}

void startupAni ()
{
  lcd.begin();
  lcd.setCursor(35,10);                   //Set the display cursor position
  lcd.print(F("Crack"));                //Set the display text
  lcd.display();                          //Output the display text
  delay(500);
  lcd.clearDisplay();                     //Clear the display
  lcd.setCursor(45,10);
  lcd.print(F("The"));
  lcd.display();
  delay(500);
  lcd.clearDisplay();
  lcd.setCursor(40,10);
  lcd.print(F("Code"));
  lcd.display();
  delay(500);
  lcd.clearDisplay();
}

void PinA()                               //Rotary encoder interrupt service routine for one encoder pin
{
  cli();                                  //Stop interrupts happening before we read pin values
  reading = PIND & 0xC;                   //Read all eight pin values then strip away all but pinA and pinB's values
  if(reading == B00001100 && aFlag)       //Check that we have both pins at detent (HIGH) and that we are expecting detent on this pin's rising edge
  {     
    if(encoderPos>0)
      encoderPos --;                      //Decrement the encoder's position count
    else
      encoderPos = 9;                     //Go back to 9 after 0
    bFlag = 0;                            //Reset flags for the next turn
    aFlag = 0;                            //Reset flags for the next turn
  }
  else if (reading == B00000100)          //Signal that we're expecting pinB to signal the transition to detent from free rotation
    bFlag = 1;
  sei();                                  //Restart interrupts
}

void PinB()                               //Rotary encoder interrupt service routine for the other encoder pin
{
  cli();                                  //Stop interrupts happening before we read pin values
  reading = PIND & 0xC;                   //Read all eight pin values then strip away all but pinA and pinB's values
  if (reading == B00001100 && bFlag)      //Check that we have both pins at detent (HIGH) and that we are expecting detent on this pin's rising edge
  {
    if(encoderPos<9)
      encoderPos ++;                      //Increment the encoder's position count
    else
      encoderPos = 0;                     //Go back to 0 after 9
    bFlag = 0;                            //Reset flags for the next turn
    aFlag = 0;                            //Reset flags for the next turn
  }
  else if (reading == B00001000)          //Signal that we're expecting pinA to signal the transition to detent from free rotation
    aFlag = 1;
  sei();                                  //Restart interrupts
}
