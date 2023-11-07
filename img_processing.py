import cv2
import os
import firebase_admin
from firebase_admin import credentials
import schedule
import time
import datetime

cred = credentials.Certificate("/Users/evansusanto/Documents/Python Despro/despro10-project/esp32firebase-41756-firebase-adminsdk-656eb-3be1b739d0.json")
firebase_admin.initialize_app(cred)

# Yolo Paths
model_path = "model/yolov4.weights"
cfg_path =  "model/yolov4.cfg"
classes_path = "model/classes.names"

db = firestore.client()

# Frame Paths
# image_path = "assets/street.jpg"
# detected_path = "assets/street_detected.jpg"
image_path = "assets/____snap.jpg"
detected_path = "assets/snap_detected.jpg"

print(os.getcwd())

# Open model classes
with open(classes_path, 'r') as file:
    classes = file.read().splitlines()

# Config
font = cv2.FONT_HERSHEY_PLAIN
conv_threshold = 0.2
nms_threshold = 0.4

# Set net
net = cv2.dnn.readNetFromDarknet(cfg_path,model_path)
model = cv2.dnn_DetectionModel(net)
model.setInputParams(size=(416,416), scale=1.0/255.0, swapRB=True, crop=False)

# Get the image from file
frame = cv2.imread(image_path)

# Detect
class_ids, confidences, boxes = model.detect(frame, conv_threshold, nms_threshold)

indexes = range(0, len(list(class_ids)))

def getPersonCount():
    person_counter = 0
    if len(list(class_ids)) > 0:
        for i in indexes:
            x,y,w,h = boxes[i]
            label = str(classes[class_ids[i]])
            confidence = str(round(confidences[i],2))
            color = (0, 255, 0)
            cv2.rectangle(frame,(x,y),(x+w,y+h),color,2)
            cv2.putText(frame,label + " " + confidence, (x,y-10),font,1,color,1)
            if class_ids[i] == 0:
                person_counter += 1
    return person_counter
        

cv2.imwrite(detected_path, frame)

def send_data_to_firestore():
    current_time = datetime.datetime.now()
    formatted_time = current_time.strftime("%Y-%m-%d %H:%M:%S")
    collection_ref = db.collection('abc')
    doc_ref = collection_ref.document('test')
    data = {
    'personCount': getPersonCount(),
    'time': formatted_time
    }
    print("Send Data to Firestore {data}")
    doc_ref.set(data)

schedule.every(5).seconds.do(send_data_to_firestore)

while True:
    schedule.run_pending()
    time.sleep(1)

print("Data posted to Firestore.")

print(class_ids)
print(confidences)
print(boxes)
print("Person detected: " + str(person_counter))
