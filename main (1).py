# Bibliotecas utilizadas no código 

import socket
import numpy as np
import cv2
from ultralytics import YOLO
import json

# Configuração do servidor

HOST = socket.gethostbyname(socket.gethostname()) 
PORT = 5000       

# Carregar o modelo YOLOv12

model = YOLO('v12att4.pt') #ultimo modelo validado (pesos ajustados da época 200)

# Função para processamento da imagem recebida.

def processa_imagem(image_path):

    imagem = cv2.imread(image_path) # abre a imagem.

    # Caso o "arquivo" de imagem esteja vazio, então eu devo exibir um erro de carregamento da imagem.
    if imagem is None:
        print(f"Erro ao carregar a imagem: {image_path}")
        return []
    
    # Realizar a detecção de objetos e armazenar em um objeto do tipo dict (dicionário python)

    results = model(imagem)
    detections = []
    detection = {}

    for result in results:
        for box in result.boxes:
            #print(f"Box: {box}")  
            #print(f"cls: {box.cls}, conf: {box.conf}, bbox: {box.xyxy}") 
            
            cls = box.cls.item() 
            conf = box.conf.item() 
            bbox = [float(coord) for coord in box.xyxy[0]]

            detection = {
                "class": cls,
                "confidence": conf,
                "bbox": bbox
            }
            
            detections.append(detection)

    return detections # retorno da função

# função responsável por receber todos os bytes da imagem

def receive_image(conn):
    # Recebe uma imagem do ESP32 via socket TCP e a salva como arquivo 
    try:
        # Receber tamanho da imagem primeiro (4 bytes)
        img_size = int.from_bytes(conn.recv(4), byteorder="little")
    
        if img_size == 0:
            print("❌ Tamanho da imagem inválido.")
            return
        print(f'Tamanho da imagem: {img_size}')

        # Receber os bytes da imagem
        img_data = b""
        while len(img_data) < img_size:
            packet = conn.recv(4096)  # Receber em pacotes de 4096 bytes, até completar a imagem.
            if not packet:
                break
            img_data += packet
        # Converter bytes para imagem, uma vez que essa imagem viaja como uma string.
        img_array = np.frombuffer(img_data, dtype=np.uint8)
        img = cv2.imdecode(img_array, cv2.IMREAD_COLOR)
        
        # Salva a imagem localmente

        if img is not None:
            cv2.imwrite("imagem_recebida.jpg", img)
            print("✅ Imagem salva com sucesso!")
        else:
            print("❌ Falha ao decodificar imagem.")
    
    # Exibe uma mensagem de carro caso não seja possível receber a imagem por qualquer motivo.

    except Exception as e:
        print(f"❌ Erro ao receber imagem: {e}")

# Criar servidor socket

server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_socket.bind((HOST, PORT))
server_socket.listen(5)

print(f"🚀 Servidor escutando em {HOST}:{PORT}")

try:
    while True:  # Loop infinito para receber imagens continuamente
        conn, addr = server_socket.accept()
        print(f"📡 Conexão recebida de {addr}")
        receive_image(conn)
        result = processa_imagem("imagem_recebida.jpg")

        # Mensagem a ser enviada a partir a estrutura de decisão do algoritmo (presença das classes "Tooling" e "Part" ao mesmo tempo).
        classes = []
        num0 = 0
        num1 = 0
        for i in result:
            classes.append(i['class'])

        print(classes)
        if 0.0 in classes: num0+=1
        if 1.0 in classes: num1+=1
        
        if (num0+num1) == 2:
            mensagem = json.dumps(1)

        else: mensagem = json.dumps(0)

        print(len(mensagem))

        # Envia a mensagem para o ESP com o comando que o sistema deve tomar
        # 0 mantém o led vermelho aceso -> não é recomendado operar a máquina por questões de segurança e operação.
        # 1 acionar o led verde -> é possível operar a máquina.

        print(f"Mensagem enviada para o esp: {mensagem}")
        conn.sendall(mensagem.encode('utf-8'))
        conn.close()

# É possível para o servidor com o comando Crtl + C no terminal.
  
except KeyboardInterrupt:
    print("\n🛑 Servidor encerrado pelo usuário.")
    server_socket.close()

# Emojis utilizados no código disponíveis em: https://emojiterra.com/pt/