import tkinter as tk
from tkinter import filedialog
import subprocess
import threading
import os

TRACKER_SCRIPT = "track_video.py"
VIEWER_EXECUTABLE = "./build/hand_skeleton"
VIDEO_TARGET_PATH = "assets/hand_video.mp4"

def select_video():
    file_path = filedialog.askopenfilename(filetypes=[("MP4 files", "*.mp4"), ("All files", "*.*")])
    if file_path:
        os.makedirs("assets", exist_ok=True)
        # Copy selected video into assets as hand_video.mp4
        with open(file_path, "rb") as src, open(VIDEO_TARGET_PATH, "wb") as dst:
            dst.write(src.read())
        status_label.config(text=f"Selected: {os.path.basename(file_path)}")

def run_tracker():
    subprocess.Popen(["python3", TRACKER_SCRIPT])

def run_viewer():
    subprocess.Popen([VIEWER_EXECUTABLE])

def start_both():
    threading.Thread(target=run_tracker).start()
    threading.Thread(target=run_viewer).start()

# GUI Setup
root = tk.Tk()
root.title("3D Hand Skeleton Launcher")
root.geometry("400x200")

select_button = tk.Button(root, text="Select Video", command=select_video, height=2)
select_button.pack(pady=10)

start_button = tk.Button(root, text="Start Tracker + Viewer", command=start_both, height=2, bg="green", fg="white")
start_button.pack(pady=10)

status_label = tk.Label(root, text="No video selected", fg="gray")
status_label.pack(pady=5)

root.mainloop()
