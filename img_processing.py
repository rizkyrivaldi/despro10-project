# import image processing
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

# Data Logging Imports
import pandas as pd
import datetime
import xml.etree.ElementTree as ET
import json
# import os

# Import Config
from config import Config

# Global Variable for testing
# Public Variable Testing Image Online
image_num = 0
imagesUrl = [
    "https://upload.wikimedia.org/wikipedia/commons/4/49/Gambar_Buku.png", 
    "https://upload.wikimedia.org/wikipedia/commons/thumb/0/09/Pilot_Urban_MR_Retro_Pop_M_Fountain_Pen_%28no_clip%29.jpg/1200px-Pilot_Urban_MR_Retro_Pop_M_Fountain_Pen_%28no_clip%29.jpg",
    "https://upload.wikimedia.org/wikipedia/commons/thumb/0/09/Pilot_Urban_MR_Retro_Pop_M_Fountain_Pen_%28no_clip%29.jpg/1200px-Pilot_Urban_MR_Retro_Pop_M_Fountain_Pen_%28no_clip%29.jpg",
    "https://static.scientificamerican.com/blogs/cache/file/6C147259-1CEB-4333-B30B7B2A6A01D777_source.jpg",
    "https://i.insider.com/51c083346bb3f7033200001c?width=1000&format=jpeg&auto=webp"]

class Detector():
    def __init__(self, model_path, cfg_path, classes_path, image_path, detected_path):
        self.model_path = model_path
        self.cfg_path = cfg_path
        self.classes_path = classes_path
        self.image_path = image_path
        self.detected_path = detected_path

        self.font = cv2.FONT_HERSHEY_PLAIN

        # Init model
        self.net = cv2.dnn.readNetFromDarknet(self.cfg_path, self.model_path)
        self.model = cv2.dnn_DetectionModel(self.net)
        self.model.setInputParams(size=(416,416), scale=1.0/255.0, swapRB=True, crop=False)
        self.conv_threshold = 0.2
        self.nms_threshold = 0.4

        # Init classes
        with open(self.classes_path, 'r') as file:
            self.classes = file.read().splitlines()

    def __snapImageOnline(self, n):
        # print("Image URL: " + imagesUrl[n])
        req = urllib.request.urlopen(imagesUrl[n])
        arr = np.asarray(bytearray(req.read()), dtype=np.uint8)
        self.img = cv2.imdecode(arr, cv2.IMREAD_COLOR)

    def __snapImageOffline(self):
        self.img = cv2.imread(self.image_path)

    def getPersonCount(self, from_web = False, save_image = True):
        # Image Selection
        if not from_web:
            self.__snapImageOffline()
        else:
            self.__snapImageOnline(4)
        
        # Image processing
        class_ids, confidences, boxes = self.model.detect(self.img, self.conv_threshold, self.nms_threshold)
        indexes = range(0, len(list(class_ids)))
        person_counter = 0
        if len(list(class_ids)) > 0:

            # Detect persons
            for i in indexes:
                if class_ids[i] == 0:
                    x,y,w,h = boxes[i]
                    label = str(self.classes[class_ids[i]])
                    confidence = str(round(confidences[i],2))
                    color = (0, 255, 0)
                    cv2.rectangle(self.img,(x,y),(x+w,y+h),color,2)
                    cv2.putText(self.img,label + " " + confidence, (x,y-10),self.font,1,color,1)
                    person_counter += 1

            # Save the image after detection
            if save_image:
                cv2.imwrite(self.detected_path, self.img)
        
        return person_counter
        
class Firebase():

    def __init__(self, certificate):
        self.cred = credentials.Certificate(certificate)
        firebase_admin.initialize_app(self.cred)
        self.db = firestore.client()

    def send_data_to_firestore(self, data):
        collection_ref = self.db.collection('abc')
        doc_ref = collection_ref.document('test')

        print(f"Send Data to Firestore {data}")
        doc_ref.set(data)

class Database():

    def __init__(self, json_latest, json_chart, json_database, current_time):
        self.json_latest = json_latest
        self.json_chart = json_chart
        self.json_database = json_database
        self.current_time = current_time
        self.empty_database = False

    def __loadCsv(self):
        try:
            self.df = pd.read_csv(self.json_database, header=None)
            if len(self.df) == 0:
                print("Database is available, but empty")
                self.empty_database = True
        except:
            print("No database available, creating empty database")
            self.empty_database = True

    def __exportLatest(self):
        # Convert the first row to list
        latest_index = len(self.df) - 1
        latest_data = self.df.loc[latest_index, :].values.flatten().tolist()

        # Export latest to json
        json_latest_dict = {
            'personCount': str(latest_data[1]),
            'time': latest_data[0],
        }
        json_latest_string = json.dumps(json_latest_dict, indent=4)
        with open(self.json_latest, 'w') as file:
            file.write(json_latest_string)
        file.close()

    def __exportHistory(self):
        # Get dataset length
        data_length = len(self.df)

        # Mendapatkan tanggal, waktu, dan jumlah orang
        num_people = self.df[1]
        access_datetime = pd.to_datetime(self.df[0], format = "%Y-%m-%d %H:%M:%S")
        current_time = self.current_time.replace(microsecond=0) # Remove microsecond accuracy

        # Membuat list
        json_dict_format = ['1hrs', '2hrs', '3hrs', '6hrs', '12hrs', '1day', '3day', '7day']
        json_dict_hours = [1, 3, 6, 12, 24, 24*3, 24*7]
        json_dict = dict()
        json_dict['now'] = {
            'count' : str(num_people[data_length-1]),
            'date' : access_datetime[data_length-1].strftime("%Y-%m-%d %H:%M:%S")
        }
        json_dict['history'] = {}

        # Create dictionary nesting
        for nest in json_dict_format:
            json_dict['history'][nest] = {
                'length' : str(0),
                'time' : [],
                'count' : []
            }

        # Mengisi dict dengan data yang ada pada database
        for i in range(data_length):
            for logging_format in range(len(json_dict_hours)):
                if access_datetime[i] >= current_time - datetime.timedelta(hours=json_dict_hours[logging_format]):
                    # Update data length
                    current_length = int(json_dict['history'][json_dict_format[logging_format]]['length'])
                    json_dict['history'][json_dict_format[logging_format]]['length'] = str(current_length+1)

                    # Update time
                    json_dict['history'][json_dict_format[logging_format]]['time'].append(access_datetime[i].strftime("%Y-%m-%d %H:%M:%S"))

                    # Update counter
                    json_dict['history'][json_dict_format[logging_format]]['count'].append(str(num_people[i])) 
                    break

        # print(json_dict)
        json_chart_string = json.dumps(json_dict, indent=4)
        
        with open(self.json_chart, 'w') as file:
            file.write(json_chart_string)
        file.close()
        
    def appendCsv(self, data):
        # Mendapatkan tanggal, waktu, dan jumlah orang
        access_datetime = data["time"]
        num_people = data["personCount"]

        # Membuat DataFrame dengan informasi yang diperoleh
        logging_data = pd.DataFrame({
            "Waktu": [access_datetime],
            "Jumlah_Orang": [num_people]
        })

        # Simpan DataFrame dalam format CSV
        csv_filename = self.json_database
        logging_data.to_csv(csv_filename, mode="a", header=not os.path.exists(csv_filename), index=False)

        # Set flag empty database to false
        self.empty_database = False

    def toJson(self):
        self.__loadCsv()

        if not self.empty_database:
            # Export latest to json
            self.__exportLatest()
            # Export history to json
            self.__exportHistory()

        else:
            print("Unable to export to JSON since the database is empty")
    

if __name__ == "__main__":
    # Determine time this program runs
    current_time = datetime.datetime.now()
    formatted_time = current_time.strftime("%Y-%m-%d %H:%M:%S")

    # Init classes
    cfg = Config()
    firebase = Firebase(cfg.firebase_certificate)
    detector = Detector(cfg.model_path, cfg.cfg_path, cfg.classes_path, cfg.image_path, cfg.detected_path)
    database = Database(cfg.json_latest, cfg.json_chart, cfg.json_database, current_time)

    # Detect person count
    # person_count = detector.getPersonCount(from_web = True)
    person_count = detector.getPersonCount()

    # parse data
    data = {
        'personCount': person_count,
        'time': formatted_time
    }

    # Send data to firebase
    # firebase.send_data_to_firestore(data)

    # Save the data to csv
    database.appendCsv(data)
    
    # Export the data to json
    database.toJson()





# cred = credentials.Certificate("esp32firebase-41756-firebase-adminsdk-656eb-3be1b739d0.json")
# firebase_admin.initialize_app(cred)

# Yolo Paths
# model_path = "model/yolov4.weights"
# cfg_path =  "model/yolov4.cfg"
# classes_path = "model/classes.names"

# Local Image Path
# image_path = "assets/snap.jpg"
# detected_path = "assets/snap_detected.jpg"

# db = firestore.client()

# Frame Paths
# image_path = "assets/street.jpg"
# detected_path = "assets/street_detected.jpg"
# image_path = "assets/____snap.jpg"
# detected_path = "assets/snap_detected.jpg"

# print(os.getcwd())

# Open model classes
# with open(classes_path, 'r') as file:
#     classes = file.read().splitlines()

# Config
# font = cv2.FONT_HERSHEY_PLAIN
# conv_threshold = 0.2
# nms_threshold = 0.4

# Set net
# net = cv2.dnn.readNetFromDarknet(cfg_path,model_path)
# model = cv2.dnn_DetectionModel(net)
# model.setInputParams(size=(416,416), scale=1.0/255.0, swapRB=True, crop=False)

# # Public Variable Testing Image Online
# image_num = 0
# imagesUrl = [
#     "https://upload.wikimedia.org/wikipedia/commons/4/49/Gambar_Buku.png", 
#     "https://upload.wikimedia.org/wikipedia/commons/thumb/0/09/Pilot_Urban_MR_Retro_Pop_M_Fountain_Pen_%28no_clip%29.jpg/1200px-Pilot_Urban_MR_Retro_Pop_M_Fountain_Pen_%28no_clip%29.jpg",
#     "https://upload.wikimedia.org/wikipedia/commons/thumb/0/09/Pilot_Urban_MR_Retro_Pop_M_Fountain_Pen_%28no_clip%29.jpg/1200px-Pilot_Urban_MR_Retro_Pop_M_Fountain_Pen_%28no_clip%29.jpg",
#     "https://static.scientificamerican.com/blogs/cache/file/6C147259-1CEB-4333-B30B7B2A6A01D777_source.jpg",
#     "https://i.insider.com/51c083346bb3f7033200001c?width=1000&format=jpeg&auto=webp"]

# def snapImageOnline(n):
#     # print("Image URL: " + imagesUrl[n])
#     req = urllib.request.urlopen(imagesUrl[n])
#     arr = np.asarray(bytearray(req.read()), dtype=np.uint8)
#     img = cv2.imdecode(arr, cv2.IMREAD_COLOR)
#     return img

# def snapImageOffline():
#     img = cv2.imread(image_path)
#     return img

# def getPersonCount(frame):
#     class_ids, confidences, boxes = model.detect(frame, conv_threshold, nms_threshold)
#     indexes = range(0, len(list(class_ids)))
#     person_counter = 0
#     if len(list(class_ids)) > 0:
#         for i in indexes:
#             x,y,w,h = boxes[i]
#             label = str(classes[class_ids[i]])
#             confidence = str(round(confidences[i],2))
#             color = (0, 255, 0)
#             cv2.rectangle(frame,(x,y),(x+w,y+h),color,2)
#             cv2.putText(frame,label + " " + confidence, (x,y-10),font,1,color,1)
#             if class_ids[i] == 0:
#                 person_counter += 1
#     # cv2.imwrite(frame, detected_path)
#     return person_counter

# def send_data_to_firestore():
#     current_time = datetime.datetime.now()
#     formatted_time = current_time.strftime("%Y-%m-%d %H:%M:%S")
#     collection_ref = db.collection('abc')
#     doc_ref = collection_ref.document('test')

#     # Save to database
#     # Fungsi argumen (formatted_time, person_count)

#     # Fungsi untuk dapetin data json
#     # fungsi argumen () return list waktu +  person_count 1 jam terakhir, 3 jam terakhir, dst
#     data = {
#     'personCount': print_person_count_rotation(),
#     'time': formatted_time
#     }
#     print(f"Send Data to Firestore {data}")
#     doc_ref.set(data)

# def print_person_count_rotation():
#     global image_num

#     # Snap Image
#     print("Image Number")
#     print(image_num)
#     image = snapImageOnline(image_num)
#     # image = snapImageOffline()

#     # Detect person count
#     counter = getPersonCount(image)

#     # print("URL : " + imagesUrl[image_num])
#     print("Person counted")
#     print(counter)

#     if image_num < len(imagesUrl):
#         image_num += 1
#     else:
#         image_num = 0

#     return counter


# schedule.every(5).seconds.do(send_data_to_firestore)

# while True:
#     schedule.run_pending()
#     time.sleep(1)

# print("Data posted to Firestore.")

# print(class_ids)
# print(confidences)
# print(boxes)
# print("Person detected: " + str(person_counter))

# if __name__ == "__main__":
#     send_data_to_firestore()