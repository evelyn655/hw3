import paho.mqtt.client as paho
import time

import serial
import time



serdev = '/dev/ttyACM0'
s = serial.Serial(serdev, 9600)

# https://os.mbed.com/teams/mqtt/wiki/Using-MQTT#python-client

# MQTT broker hosted on local machine
mqttc = paho.Client()

# Settings for connection
# TODO: revise host to your IP
host = "172.20.10.2"
topic1 = "Angle selection"
topic2 = "Angle detection"
count = 0
flag = 0


# Callbacks
def on_connect(self, mosq, obj, rc):
    print("Connected rc: " + str(rc))

def on_message(mosq, obj, msg):
    global flag, count
    print("flag: ", flag)
    num = str(msg.payload)
    if flag == 0:
        print("Message: " + str(msg.payload) + "\n")
        s.write(bytes("/Leave_Mode/run\r\n", 'UTF-8'))
        flag = 1
    else:
        if count != 9:
            count+=1
            print("count: ", count)
            print("Message: " + str(msg.payload) + "\n")
        else:
            print("Message: " + str(msg.payload) + "\n")
            s.write(bytes("/Leave_Mode/run\r\n", 'UTF-8'))
            count = 0
            flag = 0

def on_subscribe(mosq, obj, mid, granted_qos):
    print("Subscribed OK")

def on_unsubscribe(mosq, obj, mid, granted_qos):
    print("Unsubscribed OK")

# Set callbacks
mqttc.on_message = on_message
mqttc.on_connect = on_connect
mqttc.on_subscribe = on_subscribe
mqttc.on_unsubscribe = on_unsubscribe

# Connect and subscribe
print("Connecting to " + host + "/" + topic1 + " and " + topic2)
mqttc.connect(host, port=1883, keepalive=60)
mqttc.subscribe(topic1, 0)
mqttc.subscribe(topic2, 0)

# Publish messages from Python
# num = 0
# while num != 5:
#     ret = mqttc.publish(topic, "Message from Python!\n", qos=0)
#     if (ret[0] != 0):
#             print("Publish failed")
#     mqttc.loop()
#     time.sleep(1.5)
#     num += 1

# Loop forever, receiving messages
mqttc.loop_forever()