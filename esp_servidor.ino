#include "WiFi.h" // Biblioteca para conexão Wifi
#include "esp_camera.h"  // Biblioteca da câmera ESP32-CAM
// Bibliotecas que permitem acessar registradores internos da ESP32, usados para desativar o sistema de proteção contra baixa tensão (brownout)
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

//📌 Configuração Wi-Fi
const char* ssid = "*********"; // Nome da rede Wifi (Credenciais particulares)
const char* password = "***********"; // Senha da rede Wifi (Credenciais particulares)
const char* server_ip = "10.13.133.242";  // IP do servidor Python
const int server_port = 5000;   // Porta do servidor

//📌Definições do modelo da câmera 
#define CAMERA_MODEL_AI_THINKER // Has PSRAM

//📌Definição dos pinos da câmera, de acordo com o datasheet da AI-THINKER
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

#define LED_GPIO_NUM   4

// Definicao de pinos digitais dos leds
#define VERDE_LED_GPIO 16
#define VERMELHO_LED_GPIO 12

void startCamera() {
  camera_config_t config; // Estrutura de configuração da câmera
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
  config.frame_size = FRAMESIZE_UXGA; // Resolução da imagem (UXGA = 1600x1200)
  config.pixel_format = PIXFORMAT_JPEG; // Formato da imagem (JPEG, ideal para envio por rede)
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;

   if (config.pixel_format == PIXFORMAT_JPEG) {
    // Se PSRAM está disponível, aumenta a qualidade da imagem e usa buffer duplo
    if (psramFound()) {  
      config.jpeg_quality = 10;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      // Se PSRAM não disponível, reduz qualidade e resolução
      config.frame_size = FRAMESIZE_SVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
   
// Inicializa a câmera e verifica se houve erro
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.println("❌ Falha na inicialização da câmera");
    Serial.println(err);
    return;
  }
  Serial.println("📸 Câmera inicializada com sucesso!");
}


// 📌 Função setup: executada uma vez ao ligar
void setup() {
  Serial.begin(115200); // Inicia comunicação serial
  WiFi.begin(ssid, password); // Conecta ao Wi-Fi

  // Aguarda conexão com Wi-Fi
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Conectado!");
  server.begin();
  startCamera(); // Inicializa a câmera
  sendPhoto();  // Enviar foto ao conectar
  
  // pinMode é o comando que utiliza para definir se determinado tipo de componente é entrada ou saída de dados
  pinMode(VERDE_LED_GPIO, OUTPUT);
  pinMode(VERMELHO_LED_GPIO, OUTPUT);
  
  // Estado inicial: vermelho ligado
 //digitalWrite é comando que você utiliza para ligar e desligar o led. Basicamente segue essa estrutrua: digitalWrite(led, valor). Esse valor pode ser ligado = HIGH = 1 ou desligado = LOW = 0
 digitalWrite(VERDE_LED_GPIO, LOW); // Desligado 
 digitalWrite(VERMELHO_LED_GPIO, HIGH);    // Ligado 
}

// 📌 Função que captura e envia a foto para o servidor
void sendPhoto() {
  
  WiFiClient client = server.available();
  // Conecta ao servidor Python via IP e porta
  if (!client.connect(server_ip, server_port)) {
    Serial.println("❌ Falha na conexão com o servidor");
    return;
  }

  Serial.println("✅ Conectado ao servidor!");
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());
  
//Captura uma imagem da câmera
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("❌ Falha ao capturar imagem");
    return;
  }else {
  Serial.println("📸 Imagem capturada com sucesso!");
}

  //Enviar tamanho da imagem primeiro

  client.write((const uint8_t*)&fb->len, sizeof(fb->len));

  Serial.print(" tamanho da imagem: ");
  Serial.println(sizeof(fb->len));
  
  //Enviar os bytes da imagem
  client.write(fb->buf, fb->len);
  
  // Recebe resposta do servidor
  String response = "";
  while (client.connected()) {
    if (client.available()){
      char c = client.read();
      response += c;
    }
    Serial.println("Nada a receber");
  }
  Serial.println("Resposta do servidor: " + response);

  if (response == "1.0") {
   Serial.println("✅ Ferramental e peça são reconhecidos — LED verde ligado!");
   digitalWrite(VERDE_LED_GPIO, HIGH;   // Liga verde 
   digitalWrite(VERMELHO_LED_GPIO, LOW);    // Desliga vermelho
  }else {
   Serial.println("❌ Ferramental e peça NÃO reconhecidos — LED vermelho ligado!");
   digitalWrite(VERDE_LED_GPIO, LOW);  // Desliga verde
   digitalWrite(VERMELHO_LED_GPIO, HIGH);     // Liga vermelho
  }
    
  delay(500); // Pausa
  esp_camera_fb_return(fb); // Libera a memória do frame
  client.stop(); // Encerra conexão
  Serial.println("📸 Imagem enviada com sucesso!");
  

}

// 📌 Loop principal: a cada 10 segundos envia uma nova imagem
void loop() {
  delay(10000); // A cada 10 segundos, enviar uma nova imagem
  sendPhoto();
}
