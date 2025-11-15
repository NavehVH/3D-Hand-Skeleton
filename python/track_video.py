import mediapipe as mp
import cv2
import json
import time
import os
import shutil

VIDEO_PATH = "assets/hand_video.mp4" # input
OUTPUT_JSON = "assets/current.json" # output
PAUSE_FLAG = "assets/pause.flag"
DONE_FLAG = "assets/done.flag"

# Clean old flags on start
for f in [PAUSE_FLAG, DONE_FLAG]:
    if os.path.exists(f):
        os.remove(f)

# Setup MediaPipe Hands
mp_hands = mp.solutions.hands
hands = mp_hands.Hands(static_image_mode=False, max_num_hands=2) # enable video stream mode and process only 1 hand

# Open video file using opencv
cap = cv2.VideoCapture(VIDEO_PATH)

if not cap.isOpened():
    print("Failed to open video.")
    exit(1)

frame_count = 0

while True: # frame by frame loop
    if os.path.exists(PAUSE_FLAG): # if flag exists, we make it stop the loop
        time.sleep(0.1)
        continue

    # reads next frame, if no frame is left, stop
    ret, frame = cap.read() 
    if not ret:
        break

    # mediapipe expect RGB but opencv gave us BGR, so we convert it
    image = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    results = hands.process(image) # get landmarks

    # Show original frame
    cv2.imshow("Original Frame", frame)
    key = cv2.waitKey(1) & 0xFF

    # Detect window close (must come after waitKey)
    if cv2.getWindowProperty("Original Frame", cv2.WND_PROP_VISIBLE) < 1: #clicked close (X)
        print("Window closed by user. Exiting.")
        break

    if key == ord('q'): # allow also keyboard exit
        break

    # Save landmarks if detected, builds a list of 21 landmark dictionaries
    if results.multi_hand_landmarks:
        all_hands = []
        for hand_landmarks in results.multi_hand_landmarks:
            hand_points = [{"id": i, "x": lm.x, "y": lm.y, "z": lm.z}
                           for i, lm in enumerate(hand_landmarks.landmark)]
            all_hands.append(hand_points)

        temp_path = OUTPUT_JSON + ".tmp"
        with open(temp_path, "w") as f:
            json.dump(all_hands, f)
        os.replace(temp_path, OUTPUT_JSON)

    frame_count += 1
    time.sleep(1/30)

# Cleanup (video ended)
cap.release()
cv2.destroyAllWindows()

# tells the viewer it's done
with open(DONE_FLAG, "w") as f:
    f.write("done")

print("Video finished.")
