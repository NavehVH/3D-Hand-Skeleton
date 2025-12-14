"""
track_video.py

The main Producer script in the architecture.
1. Captures video frames via OpenCV.
2. Extracts skeletal landmarks using Google MediaPipe.
3. Serializes data to JSON for the C++ Consumer.
4. Determines video FPS and sets the tracking speed dynamically.
"""

import mediapipe as mp
import cv2
import json
import time
import os
import sys # <-- NEW IMPORT

# Configuration
VIDEO_PATH = "assets/hand_video.mp4"
OUTPUT_JSON = "assets/current.json"
PAUSE_FLAG = "assets/pause.flag"
DONE_FLAG = "assets/done.flag"
SPEED_FILE = "assets/speed.txt" # <-- NEW FILE

# Cleanup flags from previous runs
for f in [PAUSE_FLAG, DONE_FLAG, SPEED_FILE]:
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
if not cap.isOpened(): 
    sys.stderr.write(f"Error: Could not open video file at {VIDEO_PATH}\n")
    exit(1)

# DYNAMIC FPS LOGIC (For C++ Synchronization)
original_fps = cap.get(cv2.CAP_PROP_FPS)

# Calculate the millisecond delay required to match the video's FPS
if original_fps > 1.0: # Check if FPS is a reasonable value
    timer_ms = int((1.0 / original_fps) * 1000)
else:
    # Default to 33ms (30 FPS) if the rate cannot be read
    timer_ms = 33

# Write the calculated delay to the IPC file for the C++ viewer
with open(SPEED_FILE, "w") as f:
    f.write(str(timer_ms))

print(f"Tracking system synchronized to video speed: {original_fps:.2f} FPS ({timer_ms}ms delay in C++).")
# END DYNAMIC FPS LOGIC


while True:
    # IPC Pause Check
    if os.path.exists(PAUSE_FLAG):
        time.sleep(0.1)
        continue

    ret, frame = cap.read()
    if not ret: break

    # Convert BGR (OpenCV) to RGB (MediaPipe)
    image = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    
    # BOTTLENECK: MediaPipe processing happens here
    results = hands.process(image)

    # UI Rendering (Optional debug view)
    cv2.imshow("Original Frame", frame)
    key = cv2.waitKey(1) & 0xFF 
    if cv2.getWindowProperty("Original Frame", cv2.WND_PROP_VISIBLE) < 1 or key == ord('q'):
        break

    # Data Serialization
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

    # Atomic Write Operation
    temp_path = OUTPUT_JSON + ".tmp"
    with open(temp_path, "w") as f:
        json.dump(output_hands, f)
    os.replace(temp_path, OUTPUT_JSON)

# Cleanup
cap.release()
cv2.destroyAllWindows()

# Signal C++ process to exit
with open(DONE_FLAG, "w") as f: f.write("done")