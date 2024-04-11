import paho.mqtt.client as mqtt #import the client1
broker_address="INSERT ADDRESS" 
#broker_address="iot.eclipse.org" #use external broker
client = mqtt.Client("P1") #create new instance
client.connect(broker_address) #connect to broker
client.publish("test","From VSCode")#publishImport paho.mqtt.client as mqtt: