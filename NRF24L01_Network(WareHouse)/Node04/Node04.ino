#include <Adafruit_NeoPixel.h>
#include <RF24.h>
#include <RF24Network.h>
#include <SPI.h>

RF24 radio(9, 10);              // CE, CSN pins
RF24Network network(radio);

const uint16_t this_node = 04;
unsigned long lastMessageTime = 0;
unsigned long idleTaskInterval = 5000; // 5 seconds

// Setup brightness for LEDs 
const int brightness = 200;

// LEDs variables 
#define NUM_OF_LEDS 33
#define NUM_LIT 2
#define NUM_STRIPS 6

// Variable for Number of ITEMS per ROW 
#define NUM_OF_ITEM 17
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
    colorWipe(100*NUM_OF_LEDS);
  }

  prev_blinking = blinking;
/*   if (millis() - lastMessageTime > idleTaskInterval) {
    performIdleTask();
    lastMessageTime = millis(); // Reset the timer after performing the task
  } */
}


/*--------------------------------------------FUNCTIONS---------------------------------------------------------*/

void colorWipe(unsigned long wipeTime) {
  static unsigned long wipeStartTime = 0;
  static int left = 0;
  static int right = NUM_OF_LEDS - 1;
  static bool wipingForward = true;
  static bool wipingOff = false;
  static bool animationComplete = false;
  static uint32_t color = strips[0].Color(0, 255, 0);

  unsigned long currentMillis = millis();

  // Initialize wipe start time if this is the first call
  if (wipeStartTime == 0) {
    wipeStartTime = currentMillis;
  }

  // Determine the interval between updates
  unsigned long totalSteps = NUM_OF_LEDS; // Total steps from end to end
  unsigned long wipeInterval = wipeTime / totalSteps;

  // Perform wiping effect if enough time has passed
  if (currentMillis - wipeStartTime >= wipeInterval) {
    if (wipingForward) {
      // Set color at current positions
      for (int j = 0; j < NUM_STRIPS; j++) {
        strips[j].setPixelColor(left, color);
        strips[j].setPixelColor(right, color);
      }
    } else if (wipingOff) {
      // Clear color at current positions
      for (int j = 0; j < NUM_STRIPS; j++) {
        strips[j].setPixelColor(left, 0);
        strips[j].setPixelColor(right, 0);
      }
    }

    // Update the LEDs
    for (int j = 0; j < NUM_STRIPS; j++) {
      strips[j].show();
    }

    // Move indices towards the center
    if (wipingForward) {
      left++;
      right--;
      // Reverse direction if collision is detected
      if (left > right) {
        wipingForward = false; // Switch direction to wiping off
        wipingOff = true;
        left = 0;
        right = NUM_OF_LEDS - 1;
        wipeStartTime = currentMillis; // Restart timer for wipe off
      }
    } else if (wipingOff) {
      left++;
      right--;
      // End animation if reaching the original start position
      if (left > right) {
        animationComplete = true; // Mark animation as complete
      }
    }

    // Reset start time for the next frame
    wipeStartTime = currentMillis;
  }

  // Reset state if animation is complete
  if (animationComplete) {
    // Turn off all LEDs and reset state
    for (int j = 0; j < NUM_STRIPS; j++) {
      for (int i = 0; i < NUM_OF_LEDS; i++) {
        strips[j].setPixelColor(i, 0);
      }
      strips[j].show();
      color = getRandomColor();
    }

    left = 0;
    right = NUM_OF_LEDS - 1;
    wipingForward = true;
    wipingOff = false;
    animationComplete = false;
  }
}


void colorWipe2(uint32_t color, unsigned long wipeTime) {
  static unsigned long wipeStartTime = 0;
  static int left = 0;
  static int right = NUM_OF_LEDS - 1;
  static bool wipingForward = true;
  static bool wipeOff = false;
  static bool animationComplete = false;
  
  unsigned long currentMillis = millis();
  
  // Initialize wipe start time if this is the first call
  if (wipeStartTime == 0) {
    wipeStartTime = currentMillis;
  }
  
  // Determine the interval between updates
  unsigned long elapsedTime = currentMillis - wipeStartTime;
  unsigned long totalSteps = NUM_OF_LEDS; // Total steps from end to end
  unsigned long wipeInterval = wipeTime / totalSteps;
  
  // Perform wiping effect if enough time has passed
  if (elapsedTime >= wipeInterval) {
    // Clear LEDs at previous positions
    if (wipingForward || wipeOff) {
      for (int j = 0; j < NUM_STRIPS; j++) {
        strips[j].setPixelColor(left, 0);
        strips[j].setPixelColor(right, 0);
      }
    }

    if (wipingForward) {
      // Set color at current positions
      for (int j = 0; j < NUM_STRIPS; j++) {
        strips[j].setPixelColor(left, color);
        strips[j].setPixelColor(right, color);
      }
      
      left++;
      right--;
      
      // Reverse direction if collision is detected
      if (left > right) {
        // Stop forward wipe and start wipe off
        wipingForward = false;
        wipeOff = true;
        left--;
        right++;
      }
    } else if (wipeOff) {
      // Continue wiping effect in forward direction (to turn off LEDs)
      for (int j = 0; j < NUM_STRIPS; j++) {
        strips[j].setPixelColor(left, 0);
        strips[j].setPixelColor(right, 0);
      }

      left++;
      right--;

      // End wipe off animation if reaching the end
      if (left >= NUM_OF_LEDS) {
        animationComplete = true; // Mark animation as complete
      }
    }
    
    // Update the LEDs
    for (int j = 0; j < NUM_STRIPS; j++) {
      strips[j].show();
    }
    
    // Reset start time for the next frame
    wipeStartTime = currentMillis;
  }
  
  // Reset state if animation is complete
  if (animationComplete) {
    // Turn off all LEDs
    for (int j = 0; j < NUM_STRIPS; j++) {
      for (int i = 0; i < NUM_OF_LEDS; i++) {
        strips[j].setPixelColor(i, 0);
      }
      strips[j].show();
    }
    
    // Reset state
    left = 0;
    right = NUM_OF_LEDS - 1;
    wipingForward = true;
    wipeOff = false;
    animationComplete = false;
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

uint32_t getRandomColor() {
  // Generate random values for red, green, and blue
  byte r = random(0, 256);
  byte g = random(0, 256);
  byte b = random(0, 256);
  return strips[0].Color(r, b, g);
}

