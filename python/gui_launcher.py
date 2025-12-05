# gui_launcher.py
import tkinter as tk
from tkinter import filedialog
import subprocess
import threading
import os
import shutil
import signal
import sys
import time

TRACKER_SCRIPT = "python/track_video.py"          # Python tracker script
VIEWER_EXECUTABLE = "./build/hand_skeleton"       # our built C++ exe file
VIDEO_TARGET_PATH = "assets/hand_video.mp4"       # default video
PAUSE_FLAG = "assets/pause.flag"                  # flag file to pause tracker

# --- child process handles (keep refs so we can stop them later) ---
tracker_proc = None
viewer_proc = None

# --- cross-platform spawn/kill helpers (so we can kill entire process groups) ---
CREATE_NEW_PROCESS_GROUP = getattr(subprocess, "CREATE_NEW_PROCESS_GROUP", 0x00000200)

def popen_group(cmd):
    """
    Launch a subprocess in its own process group/session so we can terminate the whole tree.
    """
    if os.name == "posix":
        return subprocess.Popen(cmd, preexec_fn=os.setsid, close_fds=True)
    else:
        return subprocess.Popen(cmd, creationflags=CREATE_NEW_PROCESS_GROUP, close_fds=True)

def terminate_proc(proc, grace=2.0):
    """
    Try graceful stop then force kill if needed.
    """
    if not proc:
        return
    try:
        if os.name == "posix":
            # SIGTERM the whole group, then SIGKILL if needed
            os.killpg(os.getpgid(proc.pid), signal.SIGTERM)
        else:
            # On Windows, politely terminate the process
            proc.terminate()
        # Wait a bit for it to close
        t0 = time.time()
        while proc.poll() is None and (time.time() - t0) < grace:
            time.sleep(0.05)
        if proc.poll() is None:
            if os.name == "posix":
                os.killpg(os.getpgid(proc.pid), signal.SIGKILL)
            else:
                proc.kill()
    except Exception:
        pass  # already gone / no perms etc.

def stop_both():
    """
    Stop both tracker + viewer and clean flags.
    """
    global tracker_proc, viewer_proc
    # remove pause flag on exit so next run starts clean
    try:
        if os.path.exists(PAUSE_FLAG):
            os.remove(PAUSE_FLAG)
    except Exception:
        pass
    for p in (tracker_proc, viewer_proc):
        terminate_proc(p)
    tracker_proc = None
    viewer_proc = None
    start_button.config(state="normal")
    stop_button.config(state="disabled")

def check_children():
    """
    Poll children; if one died, stop the other.
    """
    global tracker_proc, viewer_proc
    dead = []
    if tracker_proc and tracker_proc.poll() is not None:
        dead.append("tracker")
    if viewer_proc and viewer_proc.poll() is not None:
        dead.append("viewer")

    if dead:
        stop_both()
        status_label.config(text=f"{' & '.join(dead)} exited — stopped both.", fg="red")
    else:
        # keep polling in 0.5s
        root.after(500, check_children)

# --- UI actions ---
# pick a video from folders
def select_video():
    file_path = filedialog.askopenfilename(filetypes=[("MP4 files", "*.mp4"), ("All files", "*.*")])
    if file_path:
        abs_selected = os.path.abspath(file_path)
        abs_tracker = os.path.abspath(TRACKER_SCRIPT)
        abs_target = os.path.abspath(VIDEO_TARGET_PATH)

        # Prevent selecting the script itself
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

# starts a separate background process for our python tracker script
def run_tracker():
    global tracker_proc
    tracker_proc = popen_group(["python3", TRACKER_SCRIPT])

# starts a separate background process for our C++ exe viewer
def run_viewer():
    global viewer_proc
    viewer_proc = popen_group([VIEWER_EXECUTABLE])

# starting them together
# uses threads so that both can be launched without blocking the main GUI loop.
def start_both():
    # avoid double-starts
    if tracker_proc or viewer_proc:
        stop_both()

    # launch both
    threading.Thread(target=run_tracker, daemon=True).start()
    threading.Thread(target=run_viewer, daemon=True).start()

    start_button.config(state="disabled")
    stop_button.config(state="normal")
    status_label.config(text="Running tracker + viewer…", fg="black")

    # begin monitoring
    root.after(300, check_children)

# stop button click
def on_stop_click():
    stop_both()
    status_label.config(text="Stopped.", fg="gray")

# ensure window close stops both
def on_close():
    stop_both()
    root.destroy()

# --- GUI ---
# initializes the main window 
root = tk.Tk()
root.title("3D Hand Skeleton Launcher")
root.geometry("420x260")
root.protocol("WM_DELETE_WINDOW", on_close)  # make sure closing the window stops both

# widget button to select video
select_button = tk.Button(root, text="Select Video", command=select_video, height=2)
select_button.pack(pady=10)

# widget button to start the program
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

# pause/start using file flags
def toggle_pause():
    paused[0] = not paused[0]
    if paused[0]:
        with open(PAUSE_FLAG, "w") as f:  # create flag
            f.write("pause")
        pause_button.config(text="Resume", bg="orange")
    else:
        if os.path.exists(PAUSE_FLAG):  # delete flag
            os.remove(PAUSE_FLAG)
        pause_button.config(text="Pause", bg="gray")

# widget button of pause/continue
pause_button = tk.Button(root, text="Pause", command=toggle_pause, height=2, bg="gray")
pause_button.pack(pady=5)

# widget button to stop both
stop_button = tk.Button(root, text="Stop", command=on_stop_click, height=2)
stop_button.pack(pady=6)
stop_button.config(state="disabled")

root.mainloop()
