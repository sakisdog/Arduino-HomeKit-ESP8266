#include <Arduino.h>
#include <arduino_homekit_server.h>
#include "wifi_info.h"

#define LOG_D(fmt, ...)   printf_P(PSTR(fmt "\n") , ##__VA_ARGS__);

#include <WS2812FX.h>
#define LED_COUNT 31
#define LED_PIN D4
WS2812FX ws2812fx = WS2812FX(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

bool is_on = false;
float current_brightness =  50.0;
float current_sat = 0.0;
float current_hue = 0.0;
bool fx_on = false;
float fx_brightness =  50.0;
float fx_sat = 0.0;
float fx_hue = 0.0;
int rgb_colors[3];

void setup() {
	Serial.begin(115200);
	wifi_connect(); // in wifi_info.h
	ws2812fx.init();
	ws2812fx.setBrightness(50); // Percentage 0-100
	ws2812fx.setSpeed(200); // 50-5000 - Faster to Slower
	ws2812fx.setMode(FX_MODE_RAINBOW_CYCLE);
	ws2812fx.start();
	rgb_colors[0] = 255;
	rgb_colors[1] = 255;
	rgb_colors[2] = 255;
	my_homekit_setup();
}

void loop() {
	ws2812fx.service();
	my_homekit_loop();
	delay(10);
}

//==============================
// HomeKit setup and loop
//==============================

// access your HomeKit characteristics defined in my_accessory.c

extern "C" homekit_server_config_t accessory_config;
extern "C" homekit_characteristic_t cha_on;
extern "C" homekit_characteristic_t cha_bright;
extern "C" homekit_characteristic_t cha_sat;
extern "C" homekit_characteristic_t cha_hue;
extern "C" homekit_characteristic_t cha_on1;
extern "C" homekit_characteristic_t cha_bright1;
extern "C" homekit_characteristic_t cha_sat1;
extern "C" homekit_characteristic_t cha_hue1;

static uint32_t next_heap_millis = 0;

void my_homekit_setup() {

	cha_on.setter = set_on;
	cha_bright.setter = set_bright;
	cha_sat.setter = set_sat;
	cha_hue.setter = set_hue;
	cha_on1.setter = set_on1;
	cha_bright1.setter = set_bright1;
	cha_sat1.setter = set_sat1;
	cha_hue1.setter = set_hue1;
	arduino_homekit_setup(&accessory_config);

}

void my_homekit_loop() {
	arduino_homekit_loop();
	const uint32_t t = millis();
	if (t > next_heap_millis) {
		// show heap info every 5 seconds
		next_heap_millis = t + 5 * 1000;
		LOG_D("Free heap: %d, HomeKit clients: %d",
				ESP.getFreeHeap(), arduino_homekit_connected_clients_count());

	}
}

void setMode360(float m) {
	//printf("WS2812FX_setMode360: %f", m);
	int mode = map(m, 0, 360, 0, ws2812fx.getModeCount()-1);
	//printf("WS2812FX_setMode: %d", mode);
	ws2812fx.setMode(mode);
}

void set_on(const homekit_value_t v) {
		bool on = v.bool_value;
		cha_on.value.bool_value = on; //sync the value

		if(on) {
			if (!fx_on){
				ws2812fx.setMode(0);
			}
			is_on = true;
			Serial.println("On");
		} else  {
				is_on = false;
				Serial.println("Off");
		}
		updateColor();
}

void set_hue(const homekit_value_t v) {
		Serial.println("set_hue");
		float hue = v.float_value;
		cha_hue.value.float_value = hue; //sync the value
		current_hue = hue;
		updateColor();
}

void set_sat(const homekit_value_t v) {
		Serial.println("set_sat");
		float sat = v.float_value;
		cha_sat.value.float_value = sat; //sync the value
		current_sat = sat;
		updateColor();
}

void set_bright(const homekit_value_t v) {
		Serial.println("set_bright");
		int bright = v.int_value;
		cha_bright.value.int_value = bright; //sync the value
		current_brightness = bright;
		updateColor();
}

void set_on1(const homekit_value_t v) {
		bool on = v.bool_value;
		cha_on1.value.bool_value = on; //sync the value
		if(on) {
				fx_on = true;
				setMode360(fx_hue);
		} else  {
				fx_on = false;
				ws2812fx.setMode(0);
		}
		
}

void set_hue1(const homekit_value_t v) {
		float hue = v.float_value;
		cha_hue1.value.float_value = hue; //sync the value
		fx_hue = hue;
		setMode360(fx_hue);
}

void set_sat1(const homekit_value_t v) {
		float sat = v.float_value;
		cha_sat1.value.float_value = sat; //sync the value
		fx_sat = sat;
}

void set_bright1(const homekit_value_t v) {
		int bright = v.int_value;
		cha_bright1.value.int_value = bright; //sync the value
		fx_brightness = bright;
		int fx_speed = map(fx_brightness,0,100,5000,50);
		ws2812fx.setSpeed(fx_speed);
//    if (fx_brightness > 50) {
//        uint8_t fx_speed = fx_brightness - 50;
//        ws2812fx.setSpeed(fx_speed*5.1);
//        //ws2812fx.setInverted(true);
//    } else {
//        uint8_t fx_speed = abs(fx_brightness - 51);
//        ws2812fx.setSpeed(fx_speed*5.1);
//        //ws2812fx.setInverted(false);
//    }
}
void updateColor()
{
	if (is_on) {
			HSV2RGB(current_hue, current_sat, current_brightness);
			int b = map(current_brightness,0, 100,0, 255);
			ws2812fx.setBrightness(b);
			ws2812fx.setColor(rgb_colors[0],rgb_colors[1],rgb_colors[2]); 
	} else if(!is_on) //lamp - switch to off
	{
			ws2812fx.setBrightness(0);
	}
}

void HSV2RGB(float h,float s,float v) {

	int i;
	float m, n, f;

	s/=100;
	v/=100;

	if(s==0){
		rgb_colors[0]=rgb_colors[1]=rgb_colors[2]=round(v*255);
		return;
	}

	h/=60;
	i=floor(h);
	f=h-i;

	if(!(i&1)){
		f=1-f;
	}

	m=v*(1-s);
	n=v*(1-s*f);

	switch (i) {

		case 0: case 6:
			rgb_colors[0]=round(v*255);
			rgb_colors[1]=round(n*255);
			rgb_colors[2]=round(m*255);
		break;

		case 1:
			rgb_colors[0]=round(n*255);
			rgb_colors[1]=round(v*255);
			rgb_colors[2]=round(m*255);
		break;

		case 2:
			rgb_colors[0]=round(m*255);
			rgb_colors[1]=round(v*255);
			rgb_colors[2]=round(n*255);
		break;

		case 3:
			rgb_colors[0]=round(m*255);
			rgb_colors[1]=round(n*255);
			rgb_colors[2]=round(v*255);
		break;

		case 4:
			rgb_colors[0]=round(n*255);
			rgb_colors[1]=round(m*255);
			rgb_colors[2]=round(v*255);
		break;

		case 5:
			rgb_colors[0]=round(v*255);
			rgb_colors[1]=round(m*255);
			rgb_colors[2]=round(n*255);
		break;
	}
}
