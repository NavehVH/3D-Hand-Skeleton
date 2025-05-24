import mediapipe as mp
import cv2
import json
import sys

# Load the image
image = cv2.imread("assets/hand.jpg")
if image is None:
    print("❌ Image not found: assets/hand.jpg")
    sys.exit(1)

# MediaPipe setup
mp_hands = mp.solutions.hands
hands = mp_hands.Hands(static_image_mode=True, max_num_hands=1)

# Process the image
results = hands.process(cv2.cvtColor(image, cv2.COLOR_BGR2RGB))

# Extract landmarks
if not results.multi_hand_landmarks:
    print("❌ No hand detected.")
    sys.exit(1)

hand_landmarks = results.multi_hand_landmarks[0]
points = []

for i, lm in enumerate(hand_landmarks.landmark):
    points.append({"id": i, "x": lm.x, "y": lm.y, "z": lm.z})

# Save to JSON
with open("assets/hand_landmarks.json", "w") as f:
    json.dump(points, f, indent=2)

print("✅ Saved 21 landmarks to assets/hand_landmarks.json")
