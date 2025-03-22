#include <RF24.h>
#include <RF24Network.h>
#include <SPI.h>

RF24 radio(9, 10);              // CE, CSN pins
RF24Network network(radio);

const uint16_t master_node = 00;
const uint16_t slave_nodes[] = {01, 02, 03, 04, 05};
int nodeIndex = -1; // It is '0', '1', '2', '3', or '4'
int prev_nodeIndex = -1;

////////For serial communication
char receivedData[32];  // Array to store the received data1*
////////////////////////////////////////////////
unsigned long timeout = 1000; // 1-second timeout for sending and receiving
unsigned long lastTime = 0;
bool waitingForResponse = false;


//node1:1-216
//node2:217-256
//node3:257-304
//node4:305-406
int array_item_Numrange[4][2] = {
  {1, 216},
  {217, 256},
  {257, 304},
  {305, 406}
};

void setup() {
  Serial.begin(9600);
  SPI.begin();
  radio.begin();
  network.begin(90, master_node); // Channel 90, master address
  radio.setDataRate(RF24_2MBPS);
}

void loop() {
  network.update();

  int itemNum;

  if (Serial.available() > 0) {
    String incomingString = Serial.readStringUntil('\n');//node_index*data

    // Get node and data (ex: Row*Item_num*Quantity*) format here from PC via incomingString
    String result = trimAndValidate(incomingString);

    if (result != "") {
      char charArray[32]; 
      incomingString = result.substring(result.indexOf(',') + 1);
      // Convert the String to a char array
      incomingString.toCharArray(receivedData, 32);  // Copy the String to the char array
      Serial.println(result);

      if (strlen(receivedData) > 2) {
        // Choose node to send to
        nodeIndex = result.substring(0, result.indexOf(',')).toInt() - 1;  

        // Check if the string is complete
        if (nodeIndex == 0) {
          // Extract Item_num from the inputString
          String itemNumStr = extractItemNum(receivedData);
          
          // Convert itemNumStr to an integer
          itemNum = itemNumStr.toInt();

          

          if(itemNum >= array_item_Numrange[0][0] && itemNum <= array_item_Numrange[0][1]){
            nodeIndex = 0;
            modifyItemNum(receivedData, array_item_Numrange[0][0] - 1);
            }
          else if(itemNum >= array_item_Numrange[1][0] && itemNum <= array_item_Numrange[1][1]){
            nodeIndex = 1;
            modifyItemNum(receivedData, array_item_Numrange[1][0] - 1);
            }
          else if(itemNum >= array_item_Numrange[2][0] && itemNum <= array_item_Numrange[2][1]){
            nodeIndex = 2;
            modifyItemNum(receivedData,  array_item_Numrange[2][0] - 1);
            }
          else if(itemNum >= array_item_Numrange[3][0] && itemNum <= array_item_Numrange[3][1]){
            nodeIndex = 3;
            modifyItemNum(receivedData,  array_item_Numrange[3][0] - 1);
            }
          else{
            Serial.println("Error:Item number does not exit!!");
            return;
            }

/*           // Print the Item_num to the serial monitor
          Serial.print("Item_num: ");
          Serial.println(itemNum); */
        }

        else {
            if(nodeIndex != 4){
            Serial.println("Error:Data can't be send, wrong node selected!!");
            return;}
        }

        Serial.println(receivedData);

        if (nodeIndex >= 0 && nodeIndex < 5) {
          uint16_t chosen_node = slave_nodes[nodeIndex];

          /* if (prev_nodeIndex != nodeIndex && prev_nodeIndex >= 0) {
            // Send "cancelled" to the previous node
            if (!sendMessageWithTimeout(slave_nodes[prev_nodeIndex], "cancelled")) {
              Serial.println("Failed to send 'cancelled' message. Timeout occurred.");
            }
          }  */

          // Send data to the selected node
          if (sendMessageWithTimeout(chosen_node, receivedData)) {
            lastTime = millis();
            waitingForResponse = true;
            //prev_nodeIndex = nodeIndex;  // Update previous node only after successful send
          } else {
            Serial.println("Failed to send message. Timeout occurred.");
          }
          nodeIndex = -1;
        }
      }
    }
  }

  if (waitingForResponse) {
    if (millis() - lastTime > timeout) {
      Serial.println("Response timed out!");
      waitingForResponse = false;
    } else {
      checkForResponse();
    }
  }
}

/*---------------------------------------FuNCTIONS-----------------------------------------------*/

void modifyItemNum(char data[], int increment) {
  // Find the position of the first and second '*'
  char* firstStar = strchr(data, '*');
  char* secondStar = strchr(firstStar + 1, '*');
  
  if (firstStar != nullptr && secondStar != nullptr) {
    // Extract the Item_num as a substring
    int itemNumLength = firstStar - data;
    char itemNumStr[8]; // Assuming Item_num won't exceed 7 digits
    strncpy(itemNumStr, data, itemNumLength);
    itemNumStr[itemNumLength] = '\0'; // Null-terminate the string
    
    // Convert Item_num to an integer, modify it, and convert it back to string
    int itemNum = atoi(itemNumStr);
    itemNum = itemNum - increment;
    snprintf(itemNumStr, sizeof(itemNumStr), "%d", itemNum);
    
    // Create the modified string
    snprintf(data, 32, "%s*%s", itemNumStr, firstStar + 1);
  }
}

String extractItemNum(String data) {
  int firstStarIndex = data.indexOf('*');
  int secondStarIndex = data.indexOf('*', firstStarIndex + 1);
  
  if (firstStarIndex >= 0 && secondStarIndex > firstStarIndex) {
    return data.substring(0, firstStarIndex);
  } else {
    return "";
  }
}

String trimAndValidate(String input) {
  int firstAsteriskIndex = input.indexOf('*');
  if (firstAsteriskIndex == -1) {
    return "";  // Return empty string if no asterisk is found
  }

  String firstPart = input.substring(0, firstAsteriskIndex);

  // Check if the first part is a valid integer and within the range 1-5
  if (firstPart.length() == 1 && isDigit(firstPart[0])) {
    int firstValue = firstPart.toInt();
    if (firstValue >= 1 && firstValue <= 5) {
      String remainingPart = input.substring(firstAsteriskIndex + 1);
      return String(firstValue) + "," + remainingPart;  // Return the two parts separated by a comma
    }
  }

  return "";  // Return empty string if invalid
}

bool sendMessageWithTimeout(uint16_t node, const char* message) {
  unsigned long sendStartTime = millis();
  RF24NetworkHeader header(node);

  while (!network.write(header, message, strlen(message) + 1)) {
    if (millis() - sendStartTime > timeout) {
      return false; // Sending timeout occurred
    }
  }

  Serial.print("Message sent to node ");
  Serial.println(node, OCT);
  return true; // Message sent successfully
}

void checkForResponse() {
  if (network.available()) {
    RF24NetworkHeader header;
    char response[32];
    network.read(header, &response, sizeof(response));

    Serial.print("Received response from node ");
    Serial.print(header.from_node, OCT);
    Serial.print(": ");
    Serial.println(response);
    waitingForResponse = false;
  }
}
