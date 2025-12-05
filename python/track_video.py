"""
track_video.py

The main Producer script in the architecture.
1. Captures video frames via OpenCV.
2. Extracts skeletal landmarks using Google MediaPipe.
3. Serializes data to JSON for the C++ Consumer.
"""

import mediapipe as mp
import cv2
import json
import time
import os

# Configuration
VIDEO_PATH = "assets/hand_video.mp4"
OUTPUT_JSON = "assets/current.json"
PAUSE_FLAG = "assets/pause.flag"
DONE_FLAG = "assets/done.flag"

# Cleanup flags from previous runs
for f in [PAUSE_FLAG, DONE_FLAG]:
    if os.path.exists(f): os.remove(f)

# Initialize MediaPipe Hands
# max_num_hands=2 allows simultaneous left/right detection.
mp_hands = mp.solutions.hands
hands = mp_hands.Hands(
    static_image_mode=False, 
    max_num_hands=2, 
    min_detection_confidence=0.5
)

cap = cv2.VideoCapture(VIDEO_PATH)
if not cap.isOpened(): exit(1)

while True:
    # IPC Pause Check
    if os.path.exists(PAUSE_FLAG):
        time.sleep(0.1)
        continue

    ret, frame = cap.read()
    if not ret: break

    # Convert BGR (OpenCV) to RGB (MediaPipe)
    image = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    results = hands.process(image)

    # UI Rendering (Optional debug view)
    cv2.imshow("Original Frame", frame)
    key = cv2.waitKey(1) & 0xFF
    if cv2.getWindowProperty("Original Frame", cv2.WND_PROP_VISIBLE) < 1 or key == ord('q'):
        break

    # -- Data Serialization --
    output_hands = []
    
    if results.multi_hand_landmarks:
        for idx, hand_landmarks in enumerate(results.multi_hand_landmarks):
            # Identify handedness (Left vs Right)
            label = results.multi_handedness[idx].classification[0].label
            
            # Extract normalized coordinates (0.0 - 1.0)
            points = [{"x": lm.x, "y": lm.y, "z": lm.z} for lm in hand_landmarks.landmark]
            
            output_hands.append({
                "label": label,
                "landmarks": points
            })

    # -- Atomic Write Operation --
    # 1. Write to temporary file
    # 2. Rename file (atomic OS operation)
    # This prevents the C++ reader from reading a half-written file.
    temp_path = OUTPUT_JSON + ".tmp"
    with open(temp_path, "w") as f:
        json.dump(output_hands, f)
    os.replace(temp_path, OUTPUT_JSON)

    time.sleep(1/30) # Cap at ~30 FPS

# Cleanup
cap.release()
cv2.destroyAllWindows()

# Signal C++ process to exit
with open(DONE_FLAG, "w") as f: f.write("done")