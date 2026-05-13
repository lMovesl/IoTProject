import paho.mqtt.client as mqtt
import json
import time
import random

BROKER = "127.0.0.1"
PORT = 1883

# Настройки эмуляции
DEVICES = [
    {
        "id": "A1B2C3D4E5", 
        "name": "Kitchen Sensor",
        "enabled": True,  # Флаг работы устройства
        "state": {"temp": 22.0, "hum": 45.0, "batt": 95.0},
        "limits": {"temp": (18.0, 28.0), "hum": (30.0, 60.0), "step_temp": 0.4, "step_hum": 0.5}
    },
    {
        "id": "F6G7H8I9J0", 
        "name": "Bedroom Sensor",
        "enabled": True,
        "state": {"temp": 21.0, "hum": 40.0, "batt": 88.0},
        "limits": {"temp": (19.0, 25.0), "hum": (35.0, 55.0), "step_temp": 0.4, "step_hum": 0.3}
    }
]

def on_connect(client, userdata, flags, rc):
    print(f"Подключено к брокеру с кодом: {rc}")
    # Подписываемся на топики управления для всех устройств
    # devices/+/control поймает команды для любого ID
    client.subscribe("devices/+/control")

def on_message(client, userdata, msg):
    try:
        # Извлекаем ID устройства из топика: devices/ID/control
        topic_parts = msg.topic.split('/')
        device_id = topic_parts[1]
        
        payload = json.loads(msg.payload.decode())
        command = payload.get("command", "").upper()
        
        for device in DEVICES:
            if device["id"] == device_id:
                if command == "ON":
                    device["enabled"] = True
                    print(f" STATUS: [{device['name']}] ВКЛЮЧЕН")
                elif command == "OFF":
                    device["enabled"] = False
                    print(f" STATUS: [{device['name']}] ВЫКЛЮЧЕН")
    except Exception as e:
        print(f"Ошибка при обработке команды: {e}")

def update_smoothly(current, min_v, max_v, max_step):
    delta = random.uniform(-max_step, max_step)
    new_value = current + delta
    if new_value < min_v: new_value = min_v + abs(delta)
    elif new_value > max_v: new_value = max_v - abs(delta)
    return round(new_value, 2)

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

try:
    client.connect(BROKER, PORT)
    client.loop_start() # Запускаем фоновый поток для приема сообщений
    
    print(f"Имитатор запущен. Жду команды ON/OFF...")
    
    while True:
        for device in DEVICES:
            # Если устройство выключено через MQTT, данные не шлем
            if not device["enabled"]:
                continue

            s = device["state"]
            l = device["limits"]

            s["temp"] = update_smoothly(s["temp"], l["temp"][0], l["temp"][1], l["step_temp"])
            s["hum"] = update_smoothly(s["hum"], l["hum"][0], l["hum"][1], l["step_hum"])
            
            if random.random() < 0.05: 
                s["batt"] = max(0, s["batt"] - 1)

            payload = {
                "temperature": {"value": round(s["temp"], 1), "unit": "°C"},
                "humidity": {"value": round(s["hum"], 1), "unit": "%"},
                "battery": {"value": int(s["batt"]), "unit": "%"}
            }
            
            topic = f"devices/{device['id']}/data"
            client.publish(topic, json.dumps(payload))
            print(f"发送 [{device['name']}] Данные отправлены")
        
        time.sleep(3) 

except Exception as e:
    print(f"Ошибка: {e}")
finally:
    client.loop_stop()
    client.disconnect()