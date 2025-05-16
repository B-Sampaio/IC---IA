#include "WiFi.h" // Biblioteca para conexão Wifi
#include "esp_camera.h"  // Biblioteca da câmera ESP32-CAM
// Bibliotecas que permitem acessar registradores internos da ESP32, usados para desativar o sistema de proteção contra baixa tensão (brownout)
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// 📌 Configuração Wi-Fi
const char* ssid = "UFRN-IoT"; // Nome da rede Wifi
const char* password = "@IOT_UFRN-044cd9#"; // Senha da rede Wifi
const char* server_ip = "10.13.133.242";  // IP do servidor Python
const int server_port = 5000;   // Porta do servidor

// Definições do modelo da câmera 
#define CAMERA_MODEL_AI_THINKER // Has PSRAM

// Definição dos pinos da câmera
#define PWDN_GPIO_NUM  32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM  0
#define SIOD_GPIO_NUM  26
#define SIOC_GPIO_NUM  27

#define Y9_GPIO_NUM    35
#define Y8_GPIO_NUM    34
#define Y7_GPIO_NUM    39
#define Y6_GPIO_NUM    36
#define Y5_GPIO_NUM    21
#define Y4_GPIO_NUM    19
#define Y3_GPIO_NUM    18
#define Y2_GPIO_NUM    5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM  23
#define PCLK_GPIO_NUM  22

// 4 for flash led or 33 for normal led
#define LED_GPIO_NUM   4

void startCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_UXGA;
  config.pixel_format = PIXFORMAT_JPEG;  // for streaming
  //config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;

   if (config.pixel_format == PIXFORMAT_JPEG) {
    if (psramFound()) {
      config.jpeg_quality = 10;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      // Limit the frame size when PSRAM is not available
      config.frame_size = FRAMESIZE_SVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  } else {
    // Best option for face detection/recognition
    config.frame_size = FRAMESIZE_240X240;
#if CONFIG_IDF_TARGET_ESP32S3
    config.fb_count = 2;
#endif
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif
  

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.println("❌ Falha na inicialização da câmera");
    Serial.println(err);
    return;
  }
  Serial.println("📸 Câmera inicializada com sucesso!");
}



void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Conectado!");
  server.begin();
  startCamera();
  sendPhoto();  // Enviar foto ao conectar
}

void sendPhoto() {
  WiFiClient client = server.available();
  if (!client.connect(server_ip, server_port)) {
    Serial.println("❌ Falha na conexão com o servidor");
    return;
  }

  Serial.println("✅ Conectado ao servidor!");
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());

  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("❌ Falha ao capturar imagem");
    return;
  }else {
  Serial.println("📸 Imagem capturada com sucesso!");
}

  // 📌 Enviar tamanho da imagem primeiro

  client.write((const uint8_t*)&fb->len, sizeof(fb->len));


//  Serial.println(" tamanho da imagem: "+(const char)((const uint8_t*)&fb->len, sizeof(fb->len)));
  Serial.print(" tamanho da imagem: ");
  Serial.println(sizeof(fb->len));
  
  // 📌 Enviar os bytes da imagem
  client.write(fb->buf, fb->len);

  String response = "";
  while (client.connected()) {
    if (client.available()){
      char c = client.read();
      response += c;
    }
    Serial.println("Nada a receber");
  }
  //Serial.println("");
  Serial.println("Resposta do servidor: " + response);
  delay(500);
  esp_camera_fb_return(fb);
  client.stop();
  Serial.println("📸 Imagem enviada com sucesso!");
  

}

void loop() {
  delay(10000); // A cada 10 segundos, enviar uma nova imagem
  sendPhoto();
}
