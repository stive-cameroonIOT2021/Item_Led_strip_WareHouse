#include "Animations.h"
#include <RF24.h>
#include <RF24Network.h>
#include <SPI.h>

RF24 radio(9, 10);              // CE, CSN pins
RF24Network network(radio);

const uint16_t this_node = 03;
unsigned long lastMessageTime = 0;
unsigned long idleTaskInterval = 5000; // 5 seconds

// Setup brightness for LEDs 
const int brightness = 50;

// LEDs variables 
#define NUM_OF_LEDS 16
#define NUM_LIT 2
#define NUM_STRIPS 6

// Variable for Number of ITEMS per ROW 
#define NUM_OF_ITEM 8
// Define the pins of each row 
const int pins[NUM_STRIPS] = {8, 7, 6, 5, 4, 3};

// Create an array of NeoPixel objects 
Adafruit_NeoPixel strips[NUM_STRIPS] = {
  Adafruit_NeoPixel(NUM_OF_LEDS, pins[0], NEO_GRB + NEO_KHZ800),
  Adafruit_NeoPixel(NUM_OF_LEDS, pins[1], NEO_GRB + NEO_KHZ800),
  Adafruit_NeoPixel(NUM_OF_LEDS, pins[2], NEO_GRB + NEO_KHZ800),
  Adafruit_NeoPixel(NUM_OF_LEDS, pins[3], NEO_GRB + NEO_KHZ800),
  Adafruit_NeoPixel(NUM_OF_LEDS, pins[4], NEO_GRB + NEO_KHZ800),
  Adafruit_NeoPixel(NUM_OF_LEDS, pins[5], NEO_GRB + NEO_KHZ800)
};

///////////////////////////////////////////////////////////////////////////////////////
// Create Animations objects for each strip
// Create an array of pointers to NeoPixel objects
Adafruit_NeoPixel* stripPointers[NUM_STRIPS];
// Create Animations objects for each strip
Animations animation(stripPointers, NUM_OF_LEDS, NUM_STRIPS, 2, 50, true);

// Rest of the division NUM of LEDS / NUM OF ITEMS 
int r;

// Read parts of string from master
String part1, part2, prev_part1[NUM_STRIPS] = {" "};

// Blinking control
bool blinking = false, prev_blinking = true;
unsigned long previousMillis = 0;
const long blinkDelay = 300; // Global variable for blink delay
unsigned long blinkStartTime = 0;
const long Blink_Period = 10000; // Blinking duration
int currentRow, startLED, endLED;
uint32_t currentColor;

// Boundary values for LED color selection
const int boundary1 = 1;
const int boundary2 = 4;

////Time between animation
unsigned long t_between_anim = 10 * 1000, t_cnt = 0;
//Number of animations
#define animNum 6
//Animation counter
int cntAnim = 0;

void setup() {
  Serial.begin(9600);
  SPI.begin();
  radio.begin();
  network.begin(90, this_node); // Channel 90, node address
  radio.setDataRate(RF24_2MBPS);

  Serial.print("Slave ");
  Serial.print(this_node);
  Serial.println(" setup complete.");

  for (int i = 0; i < NUM_STRIPS; i++) {
    strips[i].begin();
    strips[i].setBrightness(brightness);
    strips[i].show();
  }

  refresh_All_Led();

for (size_t i = 0; i < NUM_STRIPS; i++) {
    stripPointers[i] = &strips[i];
}


  randomSeed(analogRead(A0));
  r = NUM_OF_LEDS % NUM_OF_ITEM;

  lastMessageTime = millis();
}

void loop() {
  network.update();

  while (network.available()) {
    RF24NetworkHeader header;
    char message[32];
    network.read(header, &message, sizeof(message));

    Serial.print("Received message: ");
    Serial.println(message);
    lastMessageTime = millis(); // Reset the last message time

    sendResponse(header.from_node, "Message received");

    // Basic validation: Check if the received string contains exactly two '*' characters
    int firstAsteriskIndex = String(message).indexOf('*');
    int secondAsteriskIndex = String(message).indexOf('*', firstAsteriskIndex + 1);
  
    if (firstAsteriskIndex == -1 || secondAsteriskIndex == -1) {
      Serial.println("Received data is not in the expected format, ignoring.");
      blinking = false;
      return; // Skip further processing if the format is incorrect
    }

    // Extract parts
    part1 = String(message).substring(0, firstAsteriskIndex);//Extract Item number
    part2 = String(message).substring(firstAsteriskIndex + 1, secondAsteriskIndex);//Extract item quantity
    

/*     // Check if part1 is not empty before converting to an integer
    if (part1.length() == 0 || part1.toInt() <= 0 ) {
      Serial.println("Error: part1 value. Ignoring command.");
      return;
    }    */

    // Check if part2 is not empty before converting to an integer
    if (part2.length() == 0 || part2.toInt() < 0 ) {
      Serial.println("Error: part2 value. Ignoring command.");
      return;
    }  
    
    int index_row = ((part1.toInt() - 1) / NUM_OF_ITEM) ;// For raw number ((part1.toInt() - 1) / NUM_OF_ITEM) + 1

    if(index_row < 0 || index_row > NUM_STRIPS - 1) index_row = -1;

    if (index_row != -1) {

      int position_in_row = ((part1.toInt() - 1) % NUM_OF_ITEM) + 1;
      //Serial.println(position_in_row);

      if (position_in_row  <= 0 || position_in_row  > NUM_OF_ITEM) {
        if (position_in_row  <= 0) Serial.println(F("Error: item_num must be greater than 0."));
        if (position_in_row  > NUM_OF_ITEM) {
          Serial.print(F("Error: item_num must be less than "));
          Serial.print(NUM_OF_ITEM);
          Serial.println(F("."));
          return;
        }
      } else {
        int start, end;
        start = (position_in_row  - 1) * int(NUM_OF_LEDS / NUM_OF_ITEM) + min(position_in_row  - 1, r) + 1;

        if (position_in_row  > r) {
          end = start + int(NUM_OF_LEDS / NUM_OF_ITEM) - 1;
        } else {
          end = start + int(NUM_OF_LEDS / NUM_OF_ITEM);
        }

        // If new data is received and it differs from the previous one, reset blinking
        if (blinking && (index_row != currentRow || part1 != prev_part1[index_row])) {
          resetBlinking();
        }

        // Determine color based on part2.toInt() value
        uint32_t color;
        if (part2.toInt() < boundary1) {
          color = strips[index_row].Color(255, 0, 0); // Red
        } else if (part2.toInt() >= boundary1 && part2.toInt() < boundary2) {
          color = strips[index_row].Color(255, 0, 255); // Yellow
        } else {
          color = strips[index_row].Color(0, 0, 255); // Green
        }

        // Start the new blink process
        currentRow = index_row;
        startLED = start;
        endLED = end;
        currentColor = color;
        blinking = true;
        blinkStartTime = millis();  // Start timing the blink duration

        prev_part1[index_row] = part1;
      }
    }

  else{
      Serial.println("Error: part1 value. Ignoring command.");
      return;
  }
  }

  if (blinking) {
    if(prev_blinking != blinking)refresh_All_Led();
    blinkLeds();
  }
  else{
    if(prev_blinking != blinking)refresh_All_Led();
    PlayAnimation(t_between_anim);
  }

  prev_blinking = blinking;
/*   if (millis() - lastMessageTime > idleTaskInterval) {
    performIdleTask();
    lastMessageTime = millis(); // Reset the timer after performing the task
  } */
}


/*--------------------------------------------FUNCTIONS---------------------------------------------------------*/

void PlayAnimation(unsigned long Time) {
    //animation.BouncingBallEffect();
    if(millis() - t_cnt > Time){
      t_cnt = millis();
      cntAnim++;
      refresh_All_Led();
      if(cntAnim >= animNum)cntAnim = 0;
    }

    switch (cntAnim) {
          case 0: animation.colorWipe2(1000);
          break;

          case 1: animation.CometEffect(random(2), 10, 10);
          break;

          case 2: animation.colorWipe(1000);
          break;

          case 3: animation.BouncingBallEffect();
          break;

          case 4: animation.DrawMarqueeDrawMarqueeMirrored(2000);
          break;

          case 5: animation.drawTwinkle(2000);
          break;

          default:
          break;
    }
    

}


void resetBlinking() {
  blinking = false;
  refresh_All_Led();  // Turn off all LEDs
}

void blinkLeds() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= blinkDelay) {
    previousMillis = currentMillis;

    // Toggle LEDs
    for (int i = startLED - 1; i < endLED; i++) {
      uint32_t currentPixelColor = strips[currentRow].getPixelColor(i);
      if (currentPixelColor == 0) {
        strips[currentRow].setPixelColor(i, currentColor);
      } else {
        strips[currentRow].setPixelColor(i, 0);  // Turn off
      }
    }
    strips[currentRow].show();
  }

  // Stop blinking after the specified period
  if (currentMillis - blinkStartTime >= Blink_Period) {
    resetBlinking();
  }
}

void refresh_All_Led() {
  for (int j = 0; j < NUM_STRIPS; j++) {
    for (int i = 0; i < NUM_OF_LEDS; i++) {
      strips[j].setPixelColor(i, 0, 0, 0);
    }
    strips[j].show();
  }
}

void sendResponse(uint16_t to_node, const char* response) {
  RF24NetworkHeader header(to_node);
  network.write(header, response, strlen(response) + 1);
  Serial.print("Response sent to node ");
  Serial.println(to_node, OCT);
}



