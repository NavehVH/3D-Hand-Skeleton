"""
gui_launcher.py

Entry point for the application.
Manages the lifecycle of the Python tracker and C++ viewer processes,
ensuring clean startup and shutdown via process groups.
"""

import tkinter as tk
from tkinter import filedialog
import subprocess
import threading
import os
import shutil
import signal
import time

# Constants
TRACKER_SCRIPT = "python/track_video.py"
VIEWER_EXECUTABLE = "./build/hand_skeleton"
VIDEO_TARGET_PATH = "assets/hand_video.mp4"
PAUSE_FLAG = "assets/pause.flag"

# Process Handles
tracker_proc = None
viewer_proc = None

# Cross-platform process group flags
CREATE_NEW_PROCESS_GROUP = getattr(subprocess, "CREATE_NEW_PROCESS_GROUP", 0x00000200)

def popen_group(cmd):
    """Spawns a subprocess in a new session for clean termination."""
    if os.name == "posix":
        return subprocess.Popen(cmd, preexec_fn=os.setsid, close_fds=True)
    else:
        return subprocess.Popen(cmd, creationflags=CREATE_NEW_PROCESS_GROUP, close_fds=True)

def terminate_proc(proc):
    """Gracefully terminates a process group."""
    if not proc: return
    try:
        if os.name == "posix":
            os.killpg(os.getpgid(proc.pid), signal.SIGTERM)
        else:
            proc.terminate()
    except Exception:
        pass

def stop_both():
    """Stops both producer and consumer processes."""
    global tracker_proc, viewer_proc
    if os.path.exists(PAUSE_FLAG): os.remove(PAUSE_FLAG)
    terminate_proc(tracker_proc)
    terminate_proc(viewer_proc)
    tracker_proc = None
    viewer_proc = None
    start_button.config(state="normal")
    stop_button.config(state="disabled")

def check_children():
    """Polls child processes to detect unexpected crashes."""
    global tracker_proc, viewer_proc
    if (tracker_proc and tracker_proc.poll() is not None) or \
       (viewer_proc and viewer_proc.poll() is not None):
        stop_both()
        status_label.config(text="Process exited unexpectedly.", fg="red")
    else:
        root.after(500, check_children)

def select_video():
    file_path = filedialog.askopenfilename(filetypes=[("MP4 files", "*.mp4")])
    if file_path:
        os.makedirs("assets", exist_ok=True)
        shutil.copyfile(os.path.abspath(file_path), VIDEO_TARGET_PATH)
        status_label.config(text=f"Selected: {os.path.basename(file_path)}", fg="black")

def start_both():
    if tracker_proc or viewer_proc: stop_both()
    
    # Launch in separate threads to prevent GUI freezing
    threading.Thread(target=lambda: globals().update(tracker_proc=popen_group(["python3", TRACKER_SCRIPT])), daemon=True).start()
    threading.Thread(target=lambda: globals().update(viewer_proc=popen_group([VIEWER_EXECUTABLE])), daemon=True).start()

    start_button.config(state="disabled")
    stop_button.config(state="normal")
    status_label.config(text="Running...", fg="green")
    root.after(300, check_children)

def toggle_pause():
    paused = not os.path.exists(PAUSE_FLAG)
    if paused:
        with open(PAUSE_FLAG, "w") as f: f.write("pause")
        pause_button.config(text="Resume", bg="orange")
    else:
        os.remove(PAUSE_FLAG)
        pause_button.config(text="Pause", bg="gray")

# -- GUI Initialization --
root = tk.Tk()
root.title("3D Hand Launcher")
root.geometry("400x300")
root.protocol("WM_DELETE_WINDOW", lambda: (stop_both(), root.destroy()))

tk.Button(root, text="Select Video", command=select_video, height=2).pack(pady=10)
start_button = tk.Button(root, text="Start System", command=start_both, height=2, bg="green", fg="white")
start_button.pack(pady=10)
status_label = tk.Label(root, text="Ready", fg="gray")
status_label.pack(pady=5)
pause_button = tk.Button(root, text="Pause", command=toggle_pause, height=2, bg="gray")
pause_button.pack(pady=5)
stop_button = tk.Button(root, text="Stop", command=stop_both, height=2, state="disabled")
stop_button.pack(pady=6)

root.mainloop()