#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ESP32epdx.h>
#include "secrets.h"
#include "IMAGE.h"
#include "EPD.h"

const char *serverUrl = "https://epaper.nito008.com/api/image";

const size_t IMG_SIZE = 48000;
unsigned char *imageBuffer;

String currentETag = "";
unsigned long lastCheckTime = 0;
const unsigned long CHECK_INTERVAL = 10000; // MillSeconds

const char *headerKeys[] = {"ETag"};

void updateDisplay()
{
	Serial.println(">>> Updating Display with new image data... <<<");

	EPD_Init();
	EPD_WhiteScreen_ALL(imageBuffer, gImage_NULL);
	EPD_DeepSleep();
	delay(1000);
}

void checkAndUpdateImage()
{
	if (WiFi.status() != WL_CONNECTED)
		return;

	WiFiClientSecure client;
	client.setInsecure();
	HTTPClient http;

	if (http.begin(client, serverUrl))
	{
		http.collectHeaders(headerKeys, 1);

		if (currentETag.length() > 0)
		{
			http.addHeader("If-None-Match", currentETag);
		}

		int httpCode = http.GET();

		if (httpCode == HTTP_CODE_OK)
		{
			int len = http.getSize();
			if (len == IMG_SIZE)
			{
				if (imageBuffer == nullptr)
					imageBuffer = (uint8_t *)malloc(IMG_SIZE);

				if (imageBuffer)
				{
					WiFiClient *stream = http.getStreamPtr();
					stream->readBytes(imageBuffer, IMG_SIZE);

					if (http.hasHeader("ETag"))
					{
						currentETag = http.header("ETag");
						Serial.printf("New ETag: %s\n", currentETag.c_str());
					}

					updateDisplay();
				}
			}
		}
		else if (httpCode == 304)
		{
			Serial.println("Image not modified. Skipping update.");
		}
		else
		{
			Serial.printf("HTTP Error: %d\n", httpCode);
		}

		http.end();
	}
}

void setup()
{
	Serial.begin(115200);
	Serial.print("Serial OK");

	pinMode(13, INPUT);	 // BUSY
	pinMode(12, OUTPUT); // RES
	pinMode(14, OUTPUT); // DC
	pinMode(27, OUTPUT); // CS

	SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE0));
	SPI.begin();

	imageBuffer = (uint8_t *)malloc(IMG_SIZE);

	WiFi.begin(WIFI_SSID, WIFI_PASS);
	while (WiFi.status() != WL_CONNECTED)
	{
		delay(500);
		Serial.print(".");
	}
	Serial.println("\nWiFi Connected");

	checkAndUpdateImage();
}

void loop()
{
	if (millis() - lastCheckTime > CHECK_INTERVAL)
	{
		lastCheckTime = millis();
		checkAndUpdateImage();
	}
}