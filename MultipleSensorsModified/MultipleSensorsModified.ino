#include <OneWire.h>
#include <DallasTemperature.h>
#include <SPI.h>
#include <Ethernet.h>

// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 2
#define TEMPERATURE_PRECISION 11

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

DeviceAddress intoPoolThermometer = { 
    0x28, 0x88, 0xAF, 0xBD, 0x04, 0x00, 0x00, 0x9B };
DeviceAddress fromPanelThermometer  = { 
    0x28, 0x07, 0xB4, 0xBD, 0x04, 0x00, 0x00, 0x59 };

// Networking setup
byte mac[] = { 
    0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

// Device IP
IPAddress ip(10, 193, 15, 32);

// Local DNS
IPAddress myDns(192, 168, 1, 254);

// initialize the library instance:
EthernetClient client;

char server[] = "data.sparkfun.com";
int serverPort = 80;

unsigned long lastConnectionTime = 0;          // last time you connected to the server, in milliseconds
boolean lastConnected = false;                 // state of the connection last time through the main loop
const unsigned long postingInterval = 30*1000;  // delay between updates, in milliseconds

// Temporary buffer for converting floats
char t1s[10];
char t2s[10];

int ledPin = 13;

void setup(void)
{
    pinMode(ledPin, OUTPUT);

    // start serial port
    Serial.begin(9600);

    // Start up the library
    sensors.begin();

    // locate devices on the bus
    Serial.print("Locating devices...");
    Serial.print("Found ");
    Serial.print(sensors.getDeviceCount(), DEC);
    Serial.println(" devices.");

    // report parasite power requirements
    Serial.print("Parasite power is: "); 
    if (sensors.isParasitePowerMode()) Serial.println("ON");
    else Serial.println("OFF");

    // set the resolution to 9 bit
    sensors.setResolution(intoPoolThermometer, TEMPERATURE_PRECISION);
    sensors.setResolution(fromPanelThermometer, TEMPERATURE_PRECISION);

    // Time for ethernet module to boot
    delay(1000);

    if (Ethernet.begin(mac) == 0) {
        Serial.println("Failed to configure Ethernet using DHCP");
        // no point in carrying on, so do nothing forevermore:
        // try to congifure using IP address instead of DHCP:
        Ethernet.begin(mac, ip);
    }
    // print the Ethernet board/shield's IP address:
    Serial.print("My IP address: ");
    Serial.println(Ethernet.localIP());
}

void loop(void)
{ 
    // if there's incoming data from the net connection.
    // send it out the serial port.  This is for debugging
    // purposes only:
    if (client.available()) {
        char c = client.read();
        Serial.print(c);
    }

    // if there's no net connection, but there was one last time
    // through the loop, then stop the client:
    if (!client.connected() && lastConnected) {
        Serial.println();
        Serial.println("disconnecting.");
        client.stop();
    }

    // if you're not connected, and ten seconds have passed since
    // your last connection, then connect again and send data:
    if(!client.connected() && (millis() - lastConnectionTime > postingInterval)) {
        httpRequest();
    }

    // store the state of the connection for next time through
    // the loop:
    lastConnected = client.connected();
}

// this method makes a HTTP connection to the server:
void httpRequest() {
    // call sensors.requestTemperatures() to issue a global temperature 
    // request to all devices on the bus
    sensors.requestTemperatures();

    float t1 = sensors.getTempC(intoPoolThermometer);
    float t2 = sensors.getTempC(fromPanelThermometer);

    // Convert the floats to strings using the char buffers t1s and t2s
    dtostrf(t1, 6, 2, t1s);
    dtostrf(t2, 6, 2, t2s);

    Serial.println("Temps:");
    Serial.print(t1);
    Serial.print(" -- ");
    Serial.print(t2);
    Serial.println();

    String PostData="temp1=";
    PostData=String(PostData + t1s);

    PostData=PostData+"&temp2=";
    PostData=String(PostData + t2s);

    Serial.println(PostData);

    // if there's a successful connection:
    if (client.connect(server, serverPort)) {
        Serial.println("Connecting...");
        // send the HTTP POST request:
        client.println("POST /input/EJj5MwJoYlC5w53wYnMO HTTP/1.1");
        client.println("Host: data.sparkfun.com");
        client.println("User-Agent: arduino-ethernet");
        client.println("Phant-Private-Key: dqpe5ZqwoBTyvyZvVb4X");
        client.println("Connection: close");

        client.println("Content-Type: application/x-www-form-urlencoded; charset=UTF-8");
        client.print("Content-Length: ");
        client.println(PostData.length());
        client.println();
        client.println(PostData);
        client.println();

        // note the time that the connection was made:
        lastConnectionTime = millis();
    } 
    else {
        // if you couldn't make a connection:
        Serial.println("Connection failed");
        Serial.println("Disconnecting.");
        client.stop();
    }
}
