import paho.mqtt.client as mqtt
import json
import time
import random

BROKER = "127.0.0.1"
PORT = 1883

# Настройки эмуляции для каждого устройства
DEVICES = [
    {
        "id": "A1B2C3D4E5", 
        "name": "Kitchen Sensor",
        "state": {"temp": 22.0, "hum": 45.0, "batt": 95.0},
        "limits": {"temp": (18.0, 28.0), "hum": (30.0, 60.0), "step_temp": 0.4, "step_hum": 0.5}
    },
    {
        "id": "F6G7H8I9J0", 
        "name": "Bedroom Sensor",
        "state": {"temp": 21.0, "hum": 40.0, "batt": 88.0},
        "limits": {"temp": (19.0, 25.0), "hum": (35.0, 55.0), "step_temp": 0.4, "step_hum": 0.3}
    }
]

def update_smoothly(current, min_v, max_v, max_step):
    """Вычисляет следующее значение с плавным переходом"""
    delta = random.uniform(-max_step, max_step)
    new_value = current + delta
    
    # Не даем выходить за границы
    if new_value < min_v:
        new_value = min_v + abs(delta)
    elif new_value > max_v:
        new_value = max_v - abs(delta)
        
    return round(new_value, 2)

client = mqtt.Client()

try:
    client.connect(BROKER, PORT)
    print(f"Имитатор запущен (плавный режим). Подключено к {BROKER}")
    
    while True:
        for device in DEVICES:
            s = device["state"]
            l = device["limits"]

            # Плавно обновляем температуру и влажность
            s["temp"] = update_smoothly(s["temp"], l["temp"][0], l["temp"][1], l["step_temp"])
            s["hum"] = update_smoothly(s["hum"], l["hum"][0], l["hum"][1], l["step_hum"])
            
            # Батарейка просто медленно садится (раз в 10 циклов на 1%)
            if random.random() < 0.05: 
                s["batt"] = max(0, s["batt"] - 1)

            payload = {
                "temperature": {
                    "value": round(s["temp"], 1),
                    "unit": "°C"
                },
                "humidity": {
                    "value": round(s["hum"], 1),
                    "unit": "%"
                },
                "battery": {
                    "value": int(s["batt"]),
                    "unit": "%"
                }
            }
            
            topic = f"devices/{device['id']}/data"
            json_data = json.dumps(payload, ensure_ascii=False)
            client.publish(topic, json_data)
            
            print(f"[{device['name']}] T: {payload['temperature']['value']}°C, H: {payload['humidity']['value']}%")
        
        print("-" * 30)
        time.sleep(3) 

except Exception as e:
    print(f"Ошибка: {e}")
except KeyboardInterrupt:
    print("\nИмитация остановлена.")
    client.disconnect()