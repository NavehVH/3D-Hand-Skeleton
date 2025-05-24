import mediapipe as mp
import cv2
import json
import time
import os

VIDEO_PATH = "assets/hand_video.mp4"
OUTPUT_JSON = "assets/current.json"

# Setup MediaPipe Hands
mp_hands = mp.solutions.hands
hands = mp_hands.Hands(static_image_mode=False, max_num_hands=1)

# Open video file
cap = cv2.VideoCapture(VIDEO_PATH)

if not cap.isOpened():
    print("❌ Failed to open video.")
    exit(1)

frame_count = 0

while True:
    ret, frame = cap.read()
    if not ret:
        break

    image = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    results = hands.process(image)

    if results.multi_hand_landmarks:
        hand_landmarks = results.multi_hand_landmarks[0]
        points = [{"id": i, "x": lm.x, "y": lm.y, "z": lm.z} for i, lm in enumerate(hand_landmarks.landmark)]

        # Safe write via temporary file
        temp_path = OUTPUT_JSON + ".tmp"
        with open(temp_path, "w") as f:
            json.dump(points, f)
        os.replace(temp_path, OUTPUT_JSON)

    frame_count += 1
    time.sleep(1/30)  # ~30 FPS

cap.release()
print("Video finished.")
