import paho.mqtt.client as mqtt
import json
import time
import random

BROKER = "127.0.0.1"
PORT = 1883

DEVICES = [
    {"id": "A1B2C3D4E5", "name": "Kitchen Sensor"},
    {"id": "F6G7H8I9J0", "name": "Bedroom Sensor"}
]

client = mqtt.Client()

try:
    client.connect(BROKER, PORT)
    print(f"Имитатор запущен. Подключено к брокеру {BROKER}")
    
    while True:
        for device in DEVICES:
            # Обновленный формат payload: вложенные словари с value и unit
            payload = {
                "temperature": {
                    "value": round(random.uniform(20.0, 28.0), 1),
                    "unit": "°C"
                },
                "humidity": {
                    "value": round(random.uniform(30.0, 50.0), 1),
                    "unit": "%"
                },
                "battery": {
                    "value": random.randint(80, 100),
                    "unit": "%"
                }
            }
            
            topic = f"devices/{device['id']}/data"
            
            # Убедись, что ensure_ascii=False, чтобы °C отправлялось нормально, а не кодами
            json_data = json.dumps(payload, ensure_ascii=False)
            client.publish(topic, json_data)
            
            print(f"[{device['name']}] Отправлено в {topic}:\n {json_data}")
        
        print("-" * 50)
        time.sleep(5) # Пауза 5 секунд перед следующей пачкой данных

except Exception as e:
    print(f"Ошибка: {e}")
except KeyboardInterrupt:
    print("\nИмитация остановлена.")
    client.disconnect()