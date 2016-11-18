<h2>Speech Recognition with ESP8266 IoT Device</h2>

The Esp8266 with the SparkFun Thing is a very small, cheap and tiny arduino capable microprocessor that can be programmed using the Arduino eco-system. With this set-up you will be able to record speech and to use the GoogleSpeech API to retrieve text results in very good quailty

<h3>What does it do?</h3>
You speek in the microphone, the ADC MCP3201 will convert the analoge signal and send it via SPI to the EPS8266. The processor will create a ulaw 8 bit mono file, encode it and sent it to Google Cloud Speech API via Wifi. The API will return the spoken text. It can sample quite long texts, as the data is "online" sent to Goolge and not stored in the ESP8266 memory.

<h3>What do you need?</h3>
* ESP8266 Thing from Sparkfun (with Wifi)
* Microphone, e.g. SparkFun Electret Microphone 
* ADC MCP3201

<h3>What will I do next?</h3>
I will use Google Cloud Functions to parse the text returned from Speech API before returning it to the ESP8266 - so we have a very smart speech enabled contolling device for less than $20.








