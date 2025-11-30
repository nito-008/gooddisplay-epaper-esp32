#include <ESP32epdx.h>
#include <WiFi.h>
#include <time.h>
#include "EPD.h"
#include "secrets.h"

// ==========================================
// 設定エリア
// ==========================================

// 日本時間設定
const char *ntpServer = "ntp.nict.jp";
const long gmtOffset_sec = 9 * 3600; // JST UTC+9
const int daylightOffset_sec = 0;

// キャンバスの中心
const int centerX = EPD_WIDTH / 2;
const int centerY = EPD_HEIGHT / 2;

// バッファメモリ
unsigned char imageBuffer[EPD_ARRAY];

// ==========================================
// 時計ロジック
// ==========================================

void drawClockFace()
{
	// 外枠
	Paint_DrawCircle(centerX, centerY, 200, BLACK, DRAW_FILL_EMPTY, DOT_PIXEL_3X3);

	// 目盛り (12, 3, 6, 9時は赤、その他は黒)
	for (int i = 0; i < 12; i++)
	{
		float angle = i * 30 * PI / 180.0;
		int r_out = 200;
		int r_in = (i % 3 == 0) ? 170 : 190; // 3時間ごとは長く
		int thick = (i % 3 == 0) ? 4 : 2;

		int x1 = centerX + sin(angle) * r_out;
		int y1 = centerY - cos(angle) * r_out;
		int x2 = centerX + sin(angle) * r_in;
		int y2 = centerY - cos(angle) * r_in;

		Paint_DrawLine(x1, y1, x2, y2, BLACK, LINE_STYLE_SOLID, DOT_PIXEL_1X1);
	}
}

void drawHands(int hours, int minutes)
{
	// 分針 (黒)
	float minAngle = (minutes / 60.0) * 2 * PI;
	int minLen = 160;
	int mx = centerX + sin(minAngle) * minLen;
	int my = centerY - cos(minAngle) * minLen;
	Paint_DrawLine(centerX, centerY, mx, my, BLACK, LINE_STYLE_SOLID, DOT_PIXEL_4X4);

	// 時針 (黒)
	// 分による進みを考慮: hours + minutes/60
	float hourAngle = ((hours % 12) + minutes / 60.0) / 12.0 * 2 * PI;
	int hourLen = 100;
	int hx = centerX + sin(hourAngle) * hourLen;
	int hy = centerY - cos(hourAngle) * hourLen;
	Paint_DrawLine(centerX, centerY, hx, hy, BLACK, LINE_STYLE_SOLID, DOT_PIXEL_4X4);
	Paint_DrawCircle(centerX, centerY, 8, BLACK, DRAW_FILL_FULL, DOT_PIXEL_1X1);
}

void updateClock(struct tm *info)
{
	Paint_NewImage(imageBuffer, EPD_WIDTH, EPD_HEIGHT, 0, WHITE);
	Paint_SelectImage(imageBuffer);
	Paint_Clear(WHITE);

	// 描画処理
	drawClockFace();
	drawHands(info->tm_hour, info->tm_min);

	EPD_Init();
	EPD_Display_BW(imageBuffer);

	EPD_DeepSleep(); // 省電力モードへ
}

// ==========================================
// メイン処理
// ==========================================
void setup()
{
	pinMode(13, INPUT);	 // BUSY
	pinMode(12, OUTPUT); // RES
	pinMode(14, OUTPUT); // DC
	pinMode(27, OUTPUT); // CS
	// SPI
	SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE0));
	SPI.begin();
	// Serial
	Serial.begin(115200);
	Serial.print("Serial OK\n");

	// WiFi接続
	Serial.printf("Connecting to %s ", WIFI_SSID);
	WiFi.begin(WIFI_SSID, WIFI_PASS);
	while (WiFi.status() != WL_CONNECTED)
	{
		delay(500);
		Serial.print(".");
	}
	Serial.println(" CONNECTED");

	// 時間同期 (NTP)
	configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

	struct tm timeinfo;
	if (!getLocalTime(&timeinfo))
	{
		Serial.println("Failed to obtain time");
		return;
	}
	Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");

	// imageBuffer = (unsigned char *)malloc(EPD_ARRAY);

	EPD_Init();
	EPD_WhiteScreen_Black();
	EPD_DeepSleep();
}

int prevMin = -1;

void loop()
{
	struct tm timeinfo;
	if (!getLocalTime(&timeinfo))
	{
		Serial.println("Failed to obtain time");
		delay(1000);
		return;
	}

	// 分が変わったときだけ更新
	if (timeinfo.tm_min != prevMin)
	{
		Serial.println("Updating Clock...");
		updateClock(&timeinfo);
		prevMin = timeinfo.tm_min;
	}

	delay(1000); // 1秒ごとにチェック
}
