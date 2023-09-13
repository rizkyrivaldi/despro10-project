import cv2
import os

# Yolo Paths
model_path = "model/yolov4.weights"
cfg_path =  "model/yolov4.cfg"
classes_path = "model/classes.names"

# Frame Paths
# image_path = "assets/street.jpg"
# detected_path = "assets/street_detected.jpg"
image_path = "assets/snap.jpg"
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
        

cv2.imwrite(detected_path, frame)

print(class_ids)
print(confidences)
print(boxes)
print("Person detected: " + str(person_counter))
