int CarboSensorPin = 35;
int ldrMax = 4000;

int enabled = 0;
int pool_delay = 200;

void setup() {
    Serial.begin(115200);    
    Serial.printf("DBG SmartCarbo Initialized.\n");
}

void loop() {
    String serialCommand;

    while (Serial.available() > 0) {
        char serialChar = Serial.read();
        serialCommand += serialChar; 

        if (serialChar == '\n') {
            processCommand(serialCommand);
            serialCommand = "";
        }
    }

    delay(pool_delay);
}


void processCommand(String command) {
    command.trim();
    command.toUpperCase();

    // Serial.println("DBG Received command: " + command);

    if (command == "GET_MQ7") {
      Serial.printf("GAS VALUE: %d\n", mq7GetValue());
    }
    
    else if (command.startsWith("ENABLED ")) {
        int enableValue = command.substring(8).toInt();

        if (enableValue == 0) {
            enabled = enableValue;
            Serial.printf("RES ENABLED 0\n");
        }
        else if(enableValue == 1){
            enabled = enableValue;
            Serial.printf("RES ENABLED 1\n");
        }
        else {
            Serial.printf("RES ENABLED -1\n");
        }
    }

    else if (command.startsWith("POLL_DELAY ")) {
        int poll_value = command.substring(11).toInt();

        if (poll_value >= 100) {
            pool_delay = poll_value;
            Serial.printf("RES POLL_DELAY %d\n", poll_value);
        }
        else {
            Serial.printf("RES POLL_DELAY -1\n");
        }
    }
    
    else
      Serial.println("ERR Unknown command.");
      
}

int mq7GetValue() {
    int mq7Analog = analogRead(CarboSensorPin);

    return mq7Analog;
}
