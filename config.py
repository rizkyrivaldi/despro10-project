class Config():
    # Firebase
    firebase_certificate = "esp32firebase-41756-firebase-adminsdk-656eb-3be1b739d0.json"

    # Image Detection
    model_path = "model/yolov4.weights"
    cfg_path =  "model/yolov4.cfg"
    classes_path = "model/classes.names"
    image_path = "assets/snap.jpg"
    detected_path = "assets/snap_detected.jpg"

    # Database
    json_latest = "json/latest.json"
    json_chart =  "json/chart.json"
    json_database = "json/database.csv"


