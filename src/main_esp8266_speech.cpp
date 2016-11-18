#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ulaw.h>
#include <SPI.h>
#include <ExBase64.h>
#include <NoFlashSpi.h>

// #define SEND_DUMMY true

#define ESP8266_LED 5


// Wifi & network Variables
const char WiFiSSID[] = "Alice-WLANXP";
const char WiFiPSK[] = "fabneu7167";
IPAddress server(192, 168, 1, 100);  // numeric IP for Google (no DNS)
char speech_server[] = "https://speech.googleapis.com";    // name address for Google (using DNS)

// Buffer to send voice data
#define INTERVAL_US 125 // interrupt in msceconds 8000HZ
#define BUFFER_SIZE   2100
#define SEND_BUFFER_SIZE 2800 //4*(n/3)
#define BUFFER_LOOPS 20

char buffer_a[BUFFER_SIZE + 1];
char buffer_b[BUFFER_SIZE + 1];
char buffer_encode[SEND_BUFFER_SIZE + 1];
char *send_buffer;
char *write_buffer;
char *tmp_buffer;
int wifi_loop_count = 0;

bool b_buffer_send_ready = false;

uint8 loop_count = 0;
int buf_count = 0;
uint8_t ledStatus;

unsigned char linear2ulaw(int sample);

void ConnectToSpeechAPI();
void ReadAndCloseSpeechAPI();
void SetUpADC_SPI();
void SetupWifi();
void SerialKeyWait();
void StartADCTimer();

void InitADCTimer();



#ifdef SEND_DUMMY
WiFiClient client;
#else
WiFiClientSecure client;
#endif


// Interrupt *******************************************************************
void  ICACHE_RAM_ATTR  my_callback(void) {
    uint16 result = my_transfer16(0x00);
    result = result >> 1;
    result = result & 0b0000111111111111;
//    int16_t newDataSigned = (int16_t) (result + 32768); // 2 power 15
//    Serial.print(result);Serial.print("-");Serial.println(newDataSigned);
    unsigned char ulaw = linear2ulaw(result);
    write_buffer[buf_count] = ulaw;
    buf_count++;

    if (buf_count == BUFFER_SIZE) {
        tmp_buffer = write_buffer;
        write_buffer = send_buffer;
        send_buffer = tmp_buffer;
        buf_count = 0;
        b_buffer_send_ready = true;
    }

}

// Setup *******************************************************************

void setup() {
    pinMode(ESP8266_LED, OUTPUT);
    send_buffer = &buffer_a[0];
    write_buffer = &buffer_b[0];
    Serial.setDebugOutput(true);
    SerialKeyWait();

    SetUpADC_SPI();
    SetupWifi();

    InitADCTimer();

}


// the loop function runs over and over again forever

void loop() {

    loop_count = 0;buf_count = 0;
    wifi_loop_count++;
    ConnectToSpeechAPI();

    Serial.print("Recording...");

    StartADCTimer();

    while (loop_count < BUFFER_LOOPS) {
        yield();
        digitalWrite(ESP8266_LED, ledStatus); // Write LED high/low
        ledStatus = (ledStatus == HIGH) ? LOW : HIGH;

        if (b_buffer_send_ready) {
            b_buffer_send_ready = false;

            int len = base64_encode(buffer_encode, send_buffer, BUFFER_SIZE);
            buffer_encode[len] = 0; //terminate with 0

            // Write to WIFI
//            timer1_disable();
            int res = client.print(buffer_encode);
//            StartADCTimer();
//            Serial.print(loop_count);Serial.print("-"); Serial.println(res);
            loop_count++; //move me up
        }

    }


    timer1_disable();
    Serial.println("done");

    digitalWrite(ESP8266_LED, HIGH); // Write LED high/low

    yield();
    ReadAndCloseSpeechAPI();

    uint8_t ledStatus;
    while (false) {
        digitalWrite(ESP8266_LED, ledStatus); // Write LED high/low
        ledStatus = (ledStatus == HIGH) ? LOW : HIGH;
        delay(500);
    }


}



void ConnectToSpeechAPI() {
#ifdef SEND_DUMMY
    // Connect to Data Server
    Serial.print("Connect to server ");Serial.print(server);Serial.print(":");
    if (client.connect(server, 8080)) {
        Serial.println("ok");
        digitalWrite(ESP8266_LED, HIGH);
    } else {
        Serial.println("ERROR - not connected");
    };
}

#else
    Serial.print("Connect to GoogleSpeech: ");Serial.print(speech_server);
    Serial.print(":");

    if (client.connect(speech_server, 443)) {

        String content = ("{\n"
                "  \"config\": {\n"
                "    \"encoding\":\"MULAW\",\n"
                "    \"sampleRate\":8000,\n"
                "    \"languageCode\":\"de-de\"\n"
                "  },\n"
                "  \"audio\": {\n"
                "    \"content\": \"");

        u_int clength = content.length() + (SEND_BUFFER_SIZE * BUFFER_LOOPS) + 3;

        client.println("POST /v1beta1/speech:syncrecognize?key=your key here HTTP/1.1");
        client.println("Host: speech.googleapis.com");
        client.println("Content-Type: application/json");
        client.println("Connection: close");
        client.print("Content-Length: ");
        client.println(clength);
        client.println();
        client.print(content);
        digitalWrite(ESP8266_LED, HIGH);
        Serial.println("ok");

    } else {
        Serial.println("Error -  no connection to Google");
    }

#endif

}

void ReadAndCloseSpeechAPI() {
#ifndef SEND_DUMMY
    client.print("\"}}");
    client.println();

    Serial.println("WAIT for SpeechApiAnswer:");

    while (true) {
        // from the server, read them and print them:
        while (client.available()) {
            char c = client.read();
            Serial.write(c);
            yield();
        }

        // if the server's disconnected, stop the client:
        if (!client.connected()) {
            client.stop();
            break;
        }

    }
    digitalWrite(ESP8266_LED, LOW); // Write LED high/low
    Serial.print("DONE with loops:");Serial.println(wifi_loop_count);

#endif
}

void SetUpADC_SPI() {
    // Setup SPI to read micro
    // Install a decoupling capacitor (.1uF ceramic disc) across pins 2 & 3 on the MCP3201 to eliminate noise on the input.

    Serial.print("Init SPI:");
    SPI.begin();                        //Initialize the bus
    SPI.setHwCs(true); //D15 default CS
    SPI.setDataMode(SPI_MODE3);             //Set SPI data mode to 1 SPI_MODE3 0x11 - CPOL: 1  CPHA: 1
    SPI.setBitOrder(MSBFIRST);     //Expect the most significant bit first
    SPI.setFrequency(2000000);       //Set clock divider 2 MHz SPI bus speed)
    Serial.println("ok");
}

void SetupWifi() {
    Serial.println("Init Wifi:");
    byte ledStatus = LOW;

    WiFi.persistent(false); // Bugfix connectivity to RP
    WiFi.mode(WIFI_OFF);   // this is a temporary line, to be removed after SDK update to 1.5.4
    WiFi.mode(WIFI_STA);
    WiFi.begin(WiFiSSID, WiFiPSK);

    while (true) {
        wl_status_t wifi_status = WiFi.status();
        if (wifi_status == WL_CONNECTED) break;
        digitalWrite(ESP8266_LED, ledStatus); // Write LED high/low
        ledStatus = (ledStatus == HIGH) ? LOW : HIGH;
        Serial.print(".");
        delay(100);
    }
    Serial.println("ok");
}

void SerialKeyWait() {// Wait for Key
    Serial.begin(115200);
    Serial.println("Hit a key to start...");
    Serial.flush();

    while (true) {
        int inbyte = Serial.available();
        delay(500);
        if (inbyte > 0) break;
    }
}

void StartADCTimer() {
//timer dividers
// #define TIM_DIV1 	0 //80MHz (80 ticks/us - 104857.588 us max)
// #define TIM_DIV16	1 //5MHz (5 ticks/us - 1677721.4 us max)
// #define TIM_DIV265	3 //312.5Khz (1 tick = 3.2us - 26843542.4 us max)

    timer1_isr_init();
    timer1_enable(TIM_DIV16, TIM_EDGE, TIM_LOOP);
//    timer1_write(ESP.getCycleCount() + INTERVAL_US * 80); // 160 when running at 160mhz
//    timer1_write(INTERVAL_US * 80); // 160 when running at 160mhz
    timer1_write(INTERVAL_US * 5);
}


void InitADCTimer() {
    Serial.print("Init Timer:");
    timer1_disable();
    timer1_attachInterrupt(my_callback);
    Serial.println("ok");
}
