import tkinter as tk
from tkinter import filedialog
import subprocess
import threading
import os
import shutil

TRACKER_SCRIPT = "python/track_video.py"
VIEWER_EXECUTABLE = "./build/hand_skeleton" # our built C++ exe file
VIDEO_TARGET_PATH = "assets/hand_video.mp4" # default video
PAUSE_FLAG = "assets/pause.flag"

#pick a video from folders
def select_video():
    file_path = filedialog.askopenfilename(filetypes=[("MP4 files", "*.mp4"), ("All files", "*.*")])
    if file_path:
        abs_selected = os.path.abspath(file_path)
        abs_tracker = os.path.abspath(TRACKER_SCRIPT)
        abs_target = os.path.abspath(VIDEO_TARGET_PATH)

        # Prevent selecting the script
        if abs_selected == abs_tracker:
            status_label.config(text="Cannot select the tracker script!", fg="red")
            return

        # Prevent selecting the same video already in use
        if abs_selected == abs_target:
            status_label.config(text="Video already selected.", fg="orange")
            return

        os.makedirs("assets", exist_ok=True)
        shutil.copyfile(abs_selected, VIDEO_TARGET_PATH)
        status_label.config(text=f"Selected: {os.path.basename(file_path)}", fg="black")


# starts a separate background process for our python script
def run_tracker():
    subprocess.Popen(["python3", TRACKER_SCRIPT])

# starts a separate background process for our C++ exe
def run_viewer():
    subprocess.Popen([VIEWER_EXECUTABLE])

# starting them together
# uses threads so that both can be launched without blocking the main GUI loop.
def start_both():
    threading.Thread(target=run_tracker).start()
    threading.Thread(target=run_viewer).start()

# GUI Setup
# initializes the main window 
root = tk.Tk()
root.title("3D Hand Skeleton Launcher")
root.geometry("400x250")

# widget button to select video
select_button = tk.Button(root, text="Select Video", command=select_video, height=2)
select_button.pack(pady=10)

#widget button to start the program
start_button = tk.Button(root, text="Start Tracker + Viewer", command=start_both, height=2, bg="green", fg="white")
start_button.pack(pady=10)

# Check if a default video already exists
if os.path.exists(VIDEO_TARGET_PATH):
    initial_status = f"Selected: {os.path.basename(VIDEO_TARGET_PATH)}"
    fg_color = "black"
else:
    initial_status = "No video selected"
    fg_color = "gray"

# Widget text to define status regarding video chosen
status_label = tk.Label(root, text=initial_status, fg=fg_color)
status_label.pack(pady=5)

# Pause / Resume data
paused = [False]  # mutable closure flag (toggle_pause() function can modify it)

# pause\start using file flags
def toggle_pause():
    paused[0] = not paused[0]
    if paused[0]:
        with open(PAUSE_FLAG, "w") as f: # create flag
            f.write("pause")
        pause_button.config(text="Resume", bg="orange")
    else:
        if os.path.exists(PAUSE_FLAG): # delete flag
            os.remove(PAUSE_FLAG)
        pause_button.config(text="Pause", bg="gray")

# widget button of pause\continue
pause_button = tk.Button(root, text="Pause", command=toggle_pause, height=2, bg="gray")
pause_button.pack(pady=5)

root.mainloop()
