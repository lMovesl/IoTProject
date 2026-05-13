import paho.mqtt.client as mqtt
import json
import time
import random

BROKER = "127.0.0.1"
PORT = 1883

# Устройства с уникальными MAC-адресами и разными типами данных
DEVICES = [
    {
        "mac": "00:1A:22:3B:44:55", 
        "name": "Kitchen Multi-Sensor",
        "enabled": True, 
        "state": {"temp": 22.0, "hum": 45.0, "batt": 98},
        "limits": {"temp": (18, 28), "hum": (30, 60), "step": 0.3}
    },
    {
        "mac": "00:1A:22:3B:AA:BB", 
        "name": "Living Room Air",
        "enabled": True,
        "state": {"co2": 500, "temp": 21.5, "batt": 85},
        "limits": {"co2": (400, 1800), "temp": (19, 25), "step": 15} # step для CO2
    },
    {
        "mac": "44:D8:32:11:CC:DD", 
        "name": "Outdoor Weather",
        "enabled": True,
        "state": {"pressure": 755, "wind": 3.2, "temp": 12.0},
        "limits": {"pressure": (740, 770), "wind": (0, 20), "temp": (-5, 35), "step": 0.5}
    },
    {
        "mac": "DE:AD:BE:EF:00:01", 
        "name": "Garden Light Sensor",
        "enabled": True,
        "state": {"lux": 300, "temp": 18.0},
        "limits": {"lux": (0, 10000), "temp": (10, 40), "step": 100}
    }
]

def on_connect(client, userdata, flags, rc):
    print(f"Connected to Broker. Result code: {rc}")
    # Подписываемся на управление через MAC
    client.subscribe("devices/+/control")

def on_message(client, userdata, msg):
    try:
        # devices/MAC/control
        mac = msg.topic.split('/')[1]
        payload = json.loads(msg.payload.decode())
        command = payload.get("command", "").upper()
        
        for dev in DEVICES:
            if dev["mac"] == mac:
                dev["enabled"] = (command == "ON")
                print(f" COMMAND: [{dev['name']} | {mac}] -> {'ENABLED' if dev['enabled'] else 'DISABLED'}")
    except Exception as e:
        print(f"Error parsing command: {e}")

def update_val(curr, v_min, v_max, step):
    val = curr + random.uniform(-step, step)
    return round(max(min(val, v_max), v_min), 2)

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

try:
    client.connect(BROKER, PORT)
    client.loop_start()
    print("IoT Emulator started with MAC identifiers...")

    while True:
        for dev in DEVICES:
            if not dev["enabled"]: continue

            s, l = dev["state"], dev["limits"]
            payload = {}

            # Генерация данных на основе ключей в state
            if "temp" in s:
                s["temp"] = update_val(s["temp"], l["temp"][0], l["temp"][1], 0.3)
                payload["temperature"] = {"value": s["temp"], "unit": "°C"}
            
            if "hum" in s:
                s["hum"] = update_val(s["hum"], l["hum"][0], l["hum"][1], 0.5)
                payload["humidity"] = {"value": s["hum"], "unit": "%"}

            if "co2" in s:
                s["co2"] = int(update_val(s["co2"], l["co2"][0], l["co2"][1], 20))
                payload["co2_level"] = {"value": s["co2"], "unit": "ppm"}

            if "pressure" in s:
                s["pressure"] = update_val(s["pressure"], l["pressure"][0], l["pressure"][1], 0.2)
                payload["pressure"] = {"value": s["pressure"], "unit": "mmHg"}

            if "leak" in s:
                if random.random() < 0.005: s["leak"] = 1 # Протечка!
                payload["leak_status"] = {"value": s["leak"], "unit": "alarm"}

            if "batt" in s:
                if random.random() < 0.01: s["batt"] = max(0, s["batt"] - 1)
                payload["battery"] = {"value": int(s["batt"]), "unit": "%"}

            topic = f"devices/{dev['mac']}/data"
            client.publish(topic, json.dumps(payload))
            print(f"Published: {dev['mac']} ({dev['name']})")
        
        time.sleep(5)

except KeyboardInterrupt:
    print("Stopping...")
finally:
    client.loop_stop()
    client.disconnect()