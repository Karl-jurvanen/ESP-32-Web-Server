#include <WiFi.h>
#include "FS.h"
#include "SPIFFS.h"
#include "AsyncTCP.h"
#include "ESPAsyncWebServer.h"


#include "esp32_digital_led_lib.h"


AsyncWebServer server(80);
AsyncWebSocket ws("/ws"); // access at ws://[esp ip]/ws
AsyncEventSource events("/events"); // event source (Server-Sent events)

const char* ssid = "";
const char* password = "";
const char* http_username = "";
const char* http_password = "";


int NUMPIXELS = 60;
strand_t pStrand = { .rmtChannel = 0,.gpioNum = 18,.ledType = LED_WS2812B_V3,.brightLimit = 32,.numPixels = NUMPIXELS,
.pixels = nullptr,._stateVars = nullptr };


//RGB values read from web server
int r = 0;
int g = 0;
int b = 100;

//RGB values used for pulsing leds
int r_true = 0;
int g_true = 0;
int b_true = 0;



unsigned long t_0 = 0;
unsigned long t_0_1 = 0;
unsigned long divs = 255;
unsigned long blinkDelay = 1000;
unsigned long rampDelay = 3;
bool blinkState = false;
float rampstate = 0.5;
int ledPin = 22;


void onRequest(AsyncWebServerRequest *request)
{
	//Handle Unknown Request
	request->send(404);
}

void onBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
{
	//Handle body
}

void onUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
	//Handle upload
}

void onEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len)
{
	//Handle WebSocket event
}

void setup()
{
	Serial.begin(115200);
	delay(500);

	pinMode(16, OUTPUT);
	digitalWrite(16, LOW);

	if (digitalLeds_initStrands(&pStrand, 1))
	{
		Serial.println("Init FAILURE: halting");
		while (true) {};
	}

	digitalLeds_resetPixels(&pStrand);



	WiFi.mode(WIFI_STA);
	WiFi.begin(ssid, password);


	if (WiFi.waitForConnectResult() != WL_CONNECTED)
	{
		Serial.printf("WiFi Failed!\n");
		return;
	}
	Serial.printf("WiFi On!\n");

	if (!SPIFFS.begin())
	{
		Serial.println("SPIFFS Mount Failed");
		return;
	}

	// attach AsyncWebSocket
	ws.onEvent(onEvent);
	server.addHandler(&ws);

	// attach AsyncEventSource
	server.addHandler(&events);

	server.serveStatic("/bootstrap.min.css", SPIFFS, "/bootstrap.min.css").setCacheControl("max-age=600");
	server.serveStatic("/bootstrap.bundle.min.js", SPIFFS, "/bootstrap.bundle.min.js").setCacheControl("max-age=600");
	server.serveStatic("/jquery.slim.min.js", SPIFFS, "/jquery.slim.min.js").setCacheControl("max-age=600");

	server.on("/index", [](AsyncWebServerRequest *request) {

		int params = request->params();
		for (int i = 0; i<params; i++)
		{
			AsyncWebParameter* p = request->getParam(i);

			// if the field is empty, leave value as it was before
			if (p->name() == "blink")
			{
				
				setValue(p, blinkDelay);
			}

			if (p->name() == "red")
			{
				setValue(p, r, 255);
			}

			if (p->name() == "green")
			{
				setValue(p, g, 255);
			}

			if (p->name() == "blue")
			{
				setValue(p, b, 255);
			}
		}

		request->send(SPIFFS, "/index.html", "text/html");
		//request->redirect("/index");

	});
	// HTTP basic authentication
	server.on("/login", HTTP_GET, [](AsyncWebServerRequest *request) {
		if (!request->authenticate(http_username, http_password))
			return request->requestAuthentication();
		request->send(200, "text/plain", "Login Success!");
	});

	// attach filesystem root at URL /fs
	server.serveStatic("/fs", SPIFFS, "/");

	// Catch-All Handlers
	// Any request that can not find a Handler that canHandle it
	// ends in the callbacks below.
	server.onNotFound(onRequest);
	server.onFileUpload(onUpload);
	server.onRequestBody(onBody);

	server.begin();

	pinMode(ledPin, OUTPUT);

}

void loop()
{
	if (blinkDelay == 0)
	{
		holdLeds();
	}

	else
	{
		pulseLeds();

	}
}


void setValue(AsyncWebParameter* par, int &targetVar, int maxval)
{
	if (par->value() == "")
	{
		return;
	}
	else
	{
		int value = par->value().toInt();
		Serial.print(par->name());
		Serial.print("  |  ");
		Serial.println(value);

		if (value > maxval)
		{
			value = maxval;
		}
		targetVar = value;
	}
	return;
}

void setValue(AsyncWebParameter* par, unsigned long &targetVar)
{
	if (par->value() == "")
	{
		return;
	}
	else
	{
		int value = par->value().toInt();
		Serial.print(par->name());
		Serial.print("  |  ");
		Serial.println(value);

		targetVar = value;
	}
	return;
}

void holdLeds()
{
	for (size_t i = 0; i < NUMPIXELS; i++)
	{
		pStrand.pixels[i] = pixelFromRGBW(r, g, b, 0);
		//pStrand.pixels[i] = pixelFromRGBW(r, g, b, 0);
	}
	digitalLeds_updatePixels(&pStrand);
}

void pulseLeds()
{
	rampDelay = blinkDelay / divs;
	//Serial.println("Loop");
	if (millis() - t_0 > blinkDelay)
	{
		blinkState = !blinkState;


		//for (size_t i = 0; i < 60; i++)
		//{
		//	pStrand.pixels[i] = pixelFromRGBW(r * blinkState, g * blinkState, b * blinkState, 0);
		//	//pStrand.pixels[i] = pixelFromRGBW(r, g, b, 0);
		//}
		//digitalLeds_updatePixels(&pStrand);
		digitalWrite(ledPin, blinkState);
		t_0 = millis();
	}

	if (millis() - t_0_1 > rampDelay)
	{

		if (blinkState == false)
		{
			rampstate -= 0.0045;
		}
		else
		{
			rampstate += 0.0045;
		}
		if (rampstate < 0) { rampstate = 0; }
		if (rampstate > 1) { rampstate = 1; }

		//Serial.println(rampstate);

		r_true = r * rampstate;
		g_true = g * rampstate;
		b_true = b * rampstate;

		if (r_true < 0) { r_true = 0; }
		if (g_true < 0) { g_true = 0; }
		if (b_true < 0) { b_true = 0; }

		if (r_true > 255) { r_true = 255; }
		if (g_true > 255) { g_true = 255; }
		if (b_true > 255) { b_true = 255; }

		for (size_t i = 0; i < NUMPIXELS; i++)
		{
			pStrand.pixels[i] = pixelFromRGBW(r_true, g_true, b_true, 0);
			
		}
		digitalLeds_updatePixels(&pStrand);
		t_0_1 = millis();
	}
}