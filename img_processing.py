import cv2
import os
# import firestore
import firebase_admin
from firebase_admin import credentials
from firebase_admin import firestore
import schedule
import time
import datetime
import urllib.request
import numpy as np

cred = credentials.Certificate("esp32firebase-41756-firebase-adminsdk-656eb-3be1b739d0.json")
firebase_admin.initialize_app(cred)

# Yolo Paths
model_path = "model/yolov4.weights"
cfg_path =  "model/yolov4.cfg"
classes_path = "model/classes.names"

# Local Image Path
image_path = "assets/snap.jpg"
detected_path = "assets/snap_detected.jpg"

db = firestore.client()

# Frame Paths
# image_path = "assets/street.jpg"
# detected_path = "assets/street_detected.jpg"
# image_path = "assets/____snap.jpg"
# detected_path = "assets/snap_detected.jpg"

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

# Public Variable Testing Image Online
image_num = 0
imagesUrl = [
    "https://upload.wikimedia.org/wikipedia/commons/4/49/Gambar_Buku.png", 
    "https://upload.wikimedia.org/wikipedia/commons/thumb/0/09/Pilot_Urban_MR_Retro_Pop_M_Fountain_Pen_%28no_clip%29.jpg/1200px-Pilot_Urban_MR_Retro_Pop_M_Fountain_Pen_%28no_clip%29.jpg",
    "https://upload.wikimedia.org/wikipedia/commons/thumb/0/09/Pilot_Urban_MR_Retro_Pop_M_Fountain_Pen_%28no_clip%29.jpg/1200px-Pilot_Urban_MR_Retro_Pop_M_Fountain_Pen_%28no_clip%29.jpg",
    "https://static.scientificamerican.com/blogs/cache/file/6C147259-1CEB-4333-B30B7B2A6A01D777_source.jpg",
    "https://i.insider.com/51c083346bb3f7033200001c?width=1000&format=jpeg&auto=webp"]

def snapImageOnline(n):
    # print("Image URL: " + imagesUrl[n])
    req = urllib.request.urlopen(imagesUrl[n])
    arr = np.asarray(bytearray(req.read()), dtype=np.uint8)
    img = cv2.imdecode(arr, cv2.IMREAD_COLOR)
    return img

def snapImageOffline():
    img = cv2.imread(image_path)
    return img

def getPersonCount(frame):
    class_ids, confidences, boxes = model.detect(frame, conv_threshold, nms_threshold)
    indexes = range(0, len(list(class_ids)))
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
    cv2.imwrite(detected_path)
    return person_counter

def send_data_to_firestore():
    current_time = datetime.datetime.now()
    formatted_time = current_time.strftime("%Y-%m-%d %H:%M:%S")
    collection_ref = db.collection('abc')
    doc_ref = collection_ref.document('test')
    data = {
    'personCount': print_person_count_rotation(),
    'time': formatted_time
    }
    print(f"Send Data to Firestore {data}")
    doc_ref.set(data)

def print_person_count_rotation():
    # global image_num

    # Snap Image
    # image = snapImageOnline(image_num)
    image = snapImageOffline()

    # Detect person count
    counter = getPersonCount(image)

    # print("URL : " + imagesUrl[image_num])
    print("Person counted")
    print(counter)

    # if image_num > len(imagesUrl):
    #     image_num = 0
    # else:
    #     image_num += 1

    return counter


# schedule.every(5).seconds.do(send_data_to_firestore)

# while True:
#     schedule.run_pending()
#     time.sleep(1)

# print("Data posted to Firestore.")

# print(class_ids)
# print(confidences)
# print(boxes)
# print("Person detected: " + str(person_counter))

if __name__ == "__main__":
    send_data_to_firestore()