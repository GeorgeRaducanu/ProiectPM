//#include <DHT11.h>
#include <SD.h>
#include <SPI.h>
#include <LiquidCrystal_I2C.h>
#include <DistanceSensor.h>
#include "IRremote.h"

int distance_treshold = 15;

// const int echoPin = 8;
// const int trigPin = 9;
DistanceSensor sensor(9, 8);
#define buzzer 4
#define SD_CS 10
// pt sd portul 4
File myFile; // pentru fisiere
LiquidCrystal_I2C lcd(0x27, 16, 2);

int MY_TIME_CMP = 5;
#define NUM_MEASURES 5

//const int receiver = 6; // Signal Pin of IR receiver to Arduino Digital Pin 11

/*-----( Declare objects )-----*/ // pentru telecomanda
IRrecv irrecv(6);     // create instance of 'irrecv'
decode_results results;      // create instance of 'decode_results'

int command = -1;
bool first_alarm = true;

#define ledPIN 2

void setupTimer1() {
  noInterrupts();
  // Clear registers
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;

  // 1 Hz (16000000/((15624+1)*1024))
  OCR1A = 15624;
  // CTC
  TCCR1B |= (1 << WGM12);
  // Prescaler 1024
  TCCR1B |= (1 << CS12) | (1 << CS10);
  // Output Compare Match A Interrupt Enable
  TIMSK1 |= (1 << OCIE1A);
  interrupts();
}

void translateIR()
{

  switch(results.value)

  {
  case 0xFFA25D: Serial.println("CH-"); break;
  case 0xFFE21D: Serial.println("CH+"); break;
  case 0xFF629D: Serial.println("CH"); break;
  case 0xFF22DD: Serial.println("FAST BACK");    break;
  case 0xFF02FD: Serial.println("FAST FORWARD");    break;
  case 0xFFC23D: Serial.println("PAUSE");   break;
  case 0xFFE01F: Serial.println("VOL-");    break;
  case 0xFFA857: Serial.println("VOL+");    break;
  case 0xFF906F: Serial.println("EQ");    break;
  case 0xFF9867: Serial.println("100+");    break;
  case 0xFFB04F: Serial.println("200+");    break;
  case 0xFF6897: command = 0;    break;
  case 0xFF30CF: command = 1;    break;
  case 0xFF18E7: command = 2;    break;
  case 0xFF7A85: command = 3;    break;
  case 0xFF10EF: command = 4;    break;
  case 0xFF38C7: command = 5;    break;
  case 0xFF5AA5: command = 6;    break;
  case 0xFF42BD: command = 7;    break;
  case 0xFF4AB5: Serial.println("8");    break;
  case 0xFF52AD: Serial.println("9");    break;
  case 0xFFFFFFFF: Serial.println(" REPEAT");break;  

  default: 
    Serial.println(" other button   ");

  }// End Case

  // debouncer like
  delay(500); // Do not get immediate repeat


}

// Pt ecuatia Steinhart-hart
#define RT0 10000   // Ω
#define B 3977      //  K
#define VCC 5    //Supply  voltage
#define R 10000  //R=10KΩ


float my_temp_C() {
    float RT, VR, ln, TX,  T0, VRT;
    T0 = 25 + 273.15;
    VRT = analogRead(A0);
    VRT = (5.00 / 1023.00) * VRT;
    VR = VCC - VRT;
    RT = VRT / (VR / R);
    ln = log(RT / RT0);
    TX = (1 / ( (ln / B) + (1 / T0)));
    TX = TX - 273.15;
    return TX;
}

int my_clamp(int val, int min, int max) {
      return val < min ? min : (val > max ? max : val);
}

volatile unsigned int my_time = 0;

ISR(TIMER1_COMPA_vect) {
  my_time++;
}

void setup() {


    Serial.begin(9600);
      while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
    }
    
    lcd.begin();

    // Turn on the blacklight and print a message.
    //lcd.backlight();
    lcd.print("Hallo, world!");

    setupTimer1();

    irrecv.enableIRIn(); // Start the receiver
    pinMode(buzzer, OUTPUT);//initialize the buzzer pin as an output

    Serial.print("Initializing SD card...");

    if (!SD.begin(SD_CS)) {
      Serial.println("initialization failed!");
      while (1);
    }
    Serial.println("initialization done.");

    if (SD.exists("example.txt")) {
      Serial.println("example.txt exists.");
    } else {
      Serial.println("example.txt doesn't exist.");
    }

    // open a new file and immediately close it:
    Serial.println("Creating example.txt...");
    
    //myFile = SD.open("example.txt", FILE_WRITE);
    
    //myFile.close();

    // myFile.println("Hello World om");
    // myFile.println("Hai sa iasa la PM");

    // // Check to see if the file exists:
    // if (SD.exists("example.txt")) {
    //   Serial.println("example.txt exists.");
    // } else {
    //   Serial.println("example.txt doesn't exist.");
    // }
    //myFile.close();

  // delete the file:
  // Serial.println("Removing example.txt...");
  // SD.remove("example.txt");

  // if (SD.exists("example.txt")) {
  //   Serial.println("example.txt exists.");
  // } else {
  //   Serial.println("example.txt doesn't exist.");
  // }
    
}

void loop() {

    if (irrecv.decode(&results)) // have we received an IR signal?
    {
      digitalWrite(ledPIN, HIGH);

      translateIR(); 
      if (command == 0) {
        Serial.println("Comanda 0");
        distance_treshold -= 1;
        distance_treshold = my_clamp(distance_treshold, 10, 30);
      }
      else if (command == 1) {
        distance_treshold += 1;
        distance_treshold = my_clamp(distance_treshold, 10, 30);
      }
      else if (command == 2) {
        first_alarm = true;
      }
      else if (command == 3) {
        first_alarm = false;
      }
      
      else if (command == 4) {
        // afisez pe lcd min si max
        //Serial.println("la 4");
        myFile = SD.open("example.txt");
        myFile.seek(myFile.size() - (sizeof(int) + 2 * sizeof(char)) * (NUM_MEASURES + 1));
        float minn = 100.0;
        float maxx = -100.0;
        for (int i = 0; i < NUM_MEASURES; i++) {
          float val = 0.0;
          int val_int = 0;
          val_int = myFile.parseInt();
          //val_int = myFile.read();
          int val_int2 = myFile.read();
          val_int2 = myFile.read();
          val = 1.0 * (float)val_int / 100.0;
          if (val >= 10.0 && val <= 50.0) {
            if (val > maxx) {
              maxx = val;
            }
            if (val < minn) {
              minn = val;
            }
          }
        }
        myFile.close();
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("T min: ");
        lcd.print(minn);
        lcd.setCursor(0, 1);
        lcd.print("T max: ");
        lcd.print(maxx);
      }

      else if (command == 5) {
        // afisez pe lcd avg si dispersia
        // Serial.println("la 5");
        myFile = SD.open("example.txt");
        myFile.seek(myFile.size() - (sizeof(int) + 2 * sizeof(char)) * (NUM_MEASURES + 1));
        float summ = 0.0;
        float summSq = 0.0;
        int cont = 0;
        for (int i = 0; i < NUM_MEASURES; i++) {
          float val = 0.0;
          int val_int = 0;
          val_int = myFile.parseInt();
          int val_int2 = myFile.read();
          val_int2 = myFile.read();
          val = 1.0 * (float)val_int / 100.0;
          if (val >= 10.0 && val <= 50.0) {
            summ += val;
            summSq += val * val;
            cont++;
          }
        }
        myFile.close();
        // avg = summ
        summ = summ / cont;
        summSq = (summSq / cont) - summ * summ;
    
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("T med: ");
        lcd.print(summ);
        lcd.setCursor(0, 1);
        lcd.print("Dev st: ");
        lcd.print(summSq);
      }
      else if (command == 6) {
        MY_TIME_CMP += 1;
        MY_TIME_CMP = my_clamp(MY_TIME_CMP, 5, 20);
      }

      else if (command == 7) {
        MY_TIME_CMP -= 1;
        MY_TIME_CMP = my_clamp(MY_TIME_CMP, 5, 20);
      }

      digitalWrite(ledPIN, LOW);
      command = -1;
      // da voie primirea urmatoarei valori
      irrecv.resume();
    }

    if (my_time >= MY_TIME_CMP) {
        my_time = 0;
        // fac masuratoarea si o scriu in fisier
        float curr_temp = my_temp_C();
        int int_curr_temp = (int)(curr_temp * 100);
        // Serial.println(curr_temp);
        // Serial.println(int_curr_temp);
        myFile = SD.open("example.txt", FILE_WRITE);
        myFile.println(int_curr_temp);
        //myFile.print("n");
        myFile.close();

        //Serial.println("Time is comming!");
    }

    if (sensor.getCM() <= distance_treshold) {
      if (first_alarm) {
        int i;
        for(i=0;i<80;i++)
        {
          digitalWrite(buzzer,HIGH);
          delay(1);//wait for 1ms
          digitalWrite(buzzer,LOW);
          delay(1);//wait for 1ms
        }
      } else {
        int i;
        for(i=0;i<100;i++)
          {
            digitalWrite(buzzer,HIGH);
            delay(2);//wait for 2ms
            digitalWrite(buzzer,LOW);
            delay(2);//wait for 2ms
          }
      }
    }

}
