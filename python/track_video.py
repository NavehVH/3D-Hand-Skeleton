import mediapipe as mp
import cv2
import json
import time
import os

VIDEO_PATH = "assets/hand_video.mp4"
OUTPUT_JSON = "assets/current.json"
PAUSE_FLAG = "assets/pause.flag"
DONE_FLAG = "assets/done.flag"

for f in [PAUSE_FLAG, DONE_FLAG]:
    if os.path.exists(f): os.remove(f)

mp_hands = mp.solutions.hands
# שינוי: max_num_hands=2 כדי לקלוט את שתי הידיים
hands = mp_hands.Hands(static_image_mode=False, max_num_hands=2, min_detection_confidence=0.5)

cap = cv2.VideoCapture(VIDEO_PATH)
if not cap.isOpened(): exit(1)

while True:
    if os.path.exists(PAUSE_FLAG):
        time.sleep(0.1)
        continue

    ret, frame = cap.read()
    if not ret: break

    image = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    results = hands.process(image)

    cv2.imshow("Original Frame", frame)
    key = cv2.waitKey(1) & 0xFF
    if cv2.getWindowProperty("Original Frame", cv2.WND_PROP_VISIBLE) < 1 or key == ord('q'):
        break

    if results.multi_hand_landmarks:
        output_hands = []
        # רצים במקביל על הנקודות ועל התיוג (ימין/שמאל)
        for idx, hand_landmarks in enumerate(results.multi_hand_landmarks):
            # זיהוי צד היד (Left/Right)
            label = results.multi_handedness[idx].classification[0].label
            
            points = [{"x": lm.x, "y": lm.y, "z": lm.z} for lm in hand_landmarks.landmark]
            
            output_hands.append({
                "label": label,   # הוספנו את התיוג
                "landmarks": points
            })

        temp_path = OUTPUT_JSON + ".tmp"
        with open(temp_path, "w") as f:
            json.dump(output_hands, f)
        os.replace(temp_path, OUTPUT_JSON)
    else:
        # אם אין ידיים, נכתוב רשימה ריקה כדי שה-C++ לא יתקע עם מידע ישן
        with open(OUTPUT_JSON, "w") as f:
            json.dump([], f)

    time.sleep(1/30)

cap.release()
cv2.destroyAllWindows()
with open(DONE_FLAG, "w") as f: f.write("done")