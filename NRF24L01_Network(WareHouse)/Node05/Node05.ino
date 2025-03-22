#include <Adafruit_NeoPixel.h>
#include <RF24.h>
#include <RF24Network.h>
#include <SPI.h>
#include "Animations.h"

RF24 radio(49, 48);              // CE, CSN pins
RF24Network network(radio);

const uint16_t this_node = 05;
unsigned long lastMessageTime = 0;
unsigned long idleTaskInterval = 5000; // 5 seconds

// Setup brightness for LEDs 
const int brightness = 30;

// LEDs variables 
#define NUM_STRIPS 7

// Number of LEDs in each strip
const int numLeds[NUM_STRIPS] = {115, 115, 115, 115, 114, 115, 71};

// Number of items in each row
const int numItems[NUM_STRIPS] = {66, 66, 66, 66, 66, 68, 40};

// Define the pins of each row 
const int pins[NUM_STRIPS] = {8, 7, 6, 5, 4, 3, 2};

// Create an array of NeoPixel objects 
Adafruit_NeoPixel strips[NUM_STRIPS] = {
  Adafruit_NeoPixel(numLeds[0], pins[0], NEO_GRB + NEO_KHZ800),
  Adafruit_NeoPixel(numLeds[1], pins[1], NEO_GRB + NEO_KHZ800),
  Adafruit_NeoPixel(numLeds[2], pins[2], NEO_GRB + NEO_KHZ800),
  Adafruit_NeoPixel(numLeds[3], pins[3], NEO_GRB + NEO_KHZ800),
  Adafruit_NeoPixel(numLeds[4], pins[4], NEO_GRB + NEO_KHZ800),
  Adafruit_NeoPixel(numLeds[5], pins[5], NEO_GRB + NEO_KHZ800),
  Adafruit_NeoPixel(numLeds[6], pins[6], NEO_GRB + NEO_KHZ800)
};

///////////////////////////////////////////////////////////////////////////////////////
// Create Animations objects for each strip
// Create an array of pointers to NeoPixel objects
Adafruit_NeoPixel* stripPointers[NUM_STRIPS];
// Create Animations objects for each strip
Animations animation(stripPointers, numLeds[0], NUM_STRIPS, 2, 50, true);


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
// Array to store the remainder for each row
// Rest of the division NUM of LEDS / NUM OF ITEMS 
int r[NUM_STRIPS];

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
    r[i] = numLeds[i] % numItems[i];  // Calculate r for each row
  }

  refresh_All_Led();

for (size_t i = 0; i < NUM_STRIPS; i++) {
    stripPointers[i] = &strips[i];
}
  
  randomSeed(analogRead(A0));
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
    part1 = String(message).substring(0, firstAsteriskIndex); // Extract Item number
    part2 = String(message).substring(firstAsteriskIndex + 1, secondAsteriskIndex); // Extract item quantity

    // Check if part2 is not empty before converting to an integer
    if (part2.length() == 0 || part2.toInt() < 0) {
      Serial.println("Error: part2 value. Ignoring command.");
      return;
    }

    // Determine which row the item belongs to
    // Determine the row (indexRow) based on the item number (part1)
    int indexRow = -1;
    int cumulativeItems = 0;

    for (int i = 0; i < NUM_STRIPS; i++) {
      cumulativeItems += numItems[i];
      if (part1.toInt() <= cumulativeItems) {
        indexRow = i;
        break;
      }
    }

    if (indexRow == -1) {
      Serial.println("Error: part1 value out of range.");
      return;
    }

    if (indexRow != -1) {
int cumulativeItemsBefore = 0;

// Calculate the cumulative number of items before the current row
for (int i = 0; i < indexRow; i++) {
    cumulativeItemsBefore += numItems[i];
}

// Calculate the position in the current row
int positionInRow = part1.toInt() - cumulativeItemsBefore;

      if (positionInRow <= 0 || positionInRow > numItems[indexRow]) {
        Serial.println("Error: part1 value out of range.");
        return;
      } else {
        int start, end;
        start = (positionInRow - 1) * (numLeds[indexRow] / numItems[indexRow]) + min(positionInRow - 1, r[indexRow]) + 1;

        if (positionInRow > r[indexRow]) {
          end = start + int(numLeds[indexRow] / numItems[indexRow]) - 1;
        } else {
          end = start + int(numLeds[indexRow] / numItems[indexRow]);
        }

        // If new data is received and it differs from the previous one, reset blinking
        if (blinking && (indexRow != currentRow || part1 != prev_part1[indexRow])) {
          resetBlinking();
        }

        // Determine color based on part2.toInt() value
        uint32_t color;
        if (part2.toInt() < boundary1) {
          color = strips[indexRow].Color(255, 0, 0); // Red
        } else if (part2.toInt() > boundary1 && part2.toInt() < boundary2) {
          color = strips[indexRow].Color(255, 0, 255); // Yellow
        } else {
          color = strips[indexRow].Color(0, 0, 255); // Green
        }

        // Start the new blink process
        currentRow = indexRow;
        startLED = start;
        endLED = end;
        currentColor = color;
        blinking = true;
        blinkStartTime = millis();  // Start timing the blink duration

        prev_part1[indexRow] = part1;
      }
    } else {
      Serial.println("Error: part1 value. Ignoring command.");
      return;
    }
  }

  if (blinking) {
    if(prev_blinking != blinking)refresh_All_Led();
    blinkLeds();
  } else {
    uint32_t color = strips[0].Color(0, 255, 0);
    if(prev_blinking != blinking)refresh_All_Led();
    PlayAnimation(t_between_anim);
  }

  prev_blinking = blinking;
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
          case 0: animation.colorWipe2(100);
          break;

          case 1: animation.CometEffect(random(8), 10, 10);
          break;

          case 2: animation.colorWipe(100);
          break;

          case 3: animation.BouncingBallEffect();
          break;

          case 4: animation.DrawMarqueeDrawMarqueeMirrored(1000);
          break;

          case 5: animation.drawTwinkle(1000);
          break;

          default:
          break;
    }
    

}
/* void colorWipe(unsigned long wipeTime) {
  static unsigned long wipeStartTime = 0;
  static int left = 0;
  static int right = numLeds[0] - 1;
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
  unsigned long totalSteps = numLeds[0]; // Total steps from end to end
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
        right = numLeds[0] - 1;
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
      for (int i = 0; i < numLeds[0]; i++) {
        strips[j].setPixelColor(i, 0);
      }
      strips[j].show();
      color = getRandomColor();
    }

    left = 0;
    right = numLeds[0] - 1;
    wipingForward = true;
    wipingOff = false;
    animationComplete = false;
  }
}


void colorWipe2(uint32_t color, unsigned long wipeTime) {
  static unsigned long wipeStartTime = 0;
  static int left = 0;
  static int right = numLeds[0] - 1;
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
  unsigned long totalSteps = numLeds[0]; // Total steps from end to end
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
      if (left >= numLeds[0]) {
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
      for (int i = 0; i < numLeds[0]; i++) {
        strips[j].setPixelColor(i, 0);
      }
      strips[j].show();
    }
    
    // Reset state
    left = 0;
    right = numLeds[0] - 1;
    wipingForward = true;
    wipeOff = false;
    animationComplete = false;
  }
} */


void resetBlinking() {
  blinking = false;
  refresh_All_Led();  // Turn off all LEDs
}

void blinkLeds() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= blinkDelay) {
    previousMillis = currentMillis;

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

  if (currentMillis - blinkStartTime >= Blink_Period) {
    resetBlinking();
  }
}

void refresh_All_Led() {
  for (int j = 0; j < NUM_STRIPS; j++) {
    for (int i = 0; i < numLeds[j]; i++) {
      strips[j].setPixelColor(i, 0);
    }
    strips[j].show();
  }
}

void sendResponse(uint16_t toNode, const char* response) {
  RF24NetworkHeader header(toNode);
  network.write(header, response, strlen(response) + 1);
}

uint32_t getRandomColor() {
  // Generate random values for red, green, and blue
  byte r = random(0, 256);
  byte g = random(0, 256);
  byte b = random(0, 256);
  return strips[0].Color(r, b, g);
}


