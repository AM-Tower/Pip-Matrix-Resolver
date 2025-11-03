# This Python file uses the following encoding: utf-8
import os
import subprocess
import sys
import platform
import tkinter as tk
from tkinter import ttk, messagebox, scrolledtext

# ---------------- SETTINGS ----------------
PYTHON_VERSION = "3.11"
PIP_TOOLS_VERSION = "7.4.1"
REQUIREMENTS_FILE = "requirements.in"  # pip-tools input file
VENV_NAME = "venv_running"
# ------------------------------------------

# Detect OS details
def get_os_info():
    return platform.system(), platform.release(), platform.version()

# Install OS-level dependencies if needed
def install_os_dependencies():
    os_name, release, version = get_os_info()
    log_to_terminal(f"Detected OS: {os_name} {release} {version}")
    if os_name == "Linux":
        subprocess.run(["sudo", "apt-get", "update"])
        subprocess.run(["sudo", "apt-get", "install", "-y", f"python{PYTHON_VERSION}", "python3-venv"])
    elif os_name == "Darwin":  # macOS
        subprocess.run(["brew", "install", f"python@{PYTHON_VERSION}"])
    elif os_name == "Windows":
        log_to_terminal("Ensure Python is installed manually on Windows.")
    else:
        log_to_terminal("Unsupported OS.")

# Create virtual environment
def create_venv():
    subprocess.run([sys.executable, "-m", "venv", VENV_NAME])
    activate_script = os.path.join(VENV_NAME, "Scripts" if platform.system() == "Windows" else "bin", "activate")
    return activate_script

# Install pip-tools in venv
def install_pip_tools():
    pip_path = os.path.join(VENV_NAME, "bin" if platform.system() != "Windows" else "Scripts", "pip")
    subprocess.run([pip_path, "install", f"pip-tools=={PIP_TOOLS_VERSION}"])

# Compile requirements using pip-tools
def compile_requirements():
    pip_compile_path = os.path.join(VENV_NAME, "bin" if platform.system() != "Windows" else "Scripts", "pip-compile")
    result = subprocess.run([pip_compile_path, REQUIREMENTS_FILE], capture_output=True, text=True)
    log_to_terminal(result.stdout)
    if result.stderr:
        log_to_terminal(result.stderr)

# Sync requirements using pip-tools
def sync_requirements():
    pip_sync_path = os.path.join(VENV_NAME, "bin" if platform.system() != "Windows" else "Scripts", "pip-sync")
    result = subprocess.run([pip_sync_path], capture_output=True, text=True)
    log_to_terminal(result.stdout)
    if result.stderr:
        log_to_terminal(result.stderr)

# Get installed pip packages
def get_installed_packages():
    pip_path = os.path.join(VENV_NAME, "bin" if platform.system() != "Windows" else "Scripts", "pip")
    result = subprocess.run([pip_path, "list"], capture_output=True, text=True)
    return result.stdout

# ---------------- GUI ----------------
root = tk.Tk()
root.title("Virtual Environment Manager")
root.geometry("900x650")

tab_control = ttk.Notebook(root)
menu_tab = ttk.Frame(tab_control)
terminal_tab = ttk.Frame(tab_control)

tab_control.add(menu_tab, text="Menu")
tab_control.add(terminal_tab, text="Terminal")
tab_control.pack(expand=1, fill="both")

# Terminal output area
terminal_output = scrolledtext.ScrolledText(terminal_tab, wrap=tk.WORD)
terminal_output.pack(expand=True, fill="both")

def log_to_terminal(text):
    terminal_output.insert(tk.END, text + "\n")
    terminal_output.see(tk.END)

# Actions
def on_create_venv():
    log_to_terminal("Checking OS dependencies...")
    install_os_dependencies()
    log_to_terminal("Creating virtual environment...")
    activate_script = create_venv()
    log_to_terminal(f"Virtual environment created at {activate_script}")
    log_to_terminal("Installing pip-tools...")
    install_pip_tools()
    log_to_terminal("pip-tools installed.")
    tab_control.select(terminal_tab)  # Switch to Terminal tab

def on_status():
    packages = get_installed_packages()
    messagebox.showinfo("Installed Packages", packages)

def on_compile():
    log_to_terminal("Running pip-compile...")
    compile_requirements()

def on_sync():
    log_to_terminal("Running pip-sync...")
    sync_requirements()

# Run arbitrary shell command inside venv
def run_shell_command():
    cmd = shell_entry.get()
    if not cmd.strip():
        log_to_terminal("No command entered.")
        return
    log_to_terminal(f"Executing in venv: {cmd}")
    # Prepend venv bin path to PATH so commands run inside venv
    env = os.environ.copy()
    venv_bin = os.path.join(VENV_NAME, "Scripts" if platform.system() == "Windows" else "bin")
    env["PATH"] = venv_bin + os.pathsep + env["PATH"]
    result = subprocess.run(cmd, shell=True, capture_output=True, text=True, env=env)
    if result.stdout:
        log_to_terminal(result.stdout)
    if result.stderr:
        log_to_terminal(result.stderr)

# Menu tab UI
create_btn = ttk.Button(menu_tab, text="Create Venv", command=on_create_venv)
create_btn.pack(pady=10)

status_btn = ttk.Button(menu_tab, text="Status", command=on_status)
status_btn.pack(pady=10)

compile_btn = ttk.Button(menu_tab, text="Compile Requirements", command=on_compile)
compile_btn.pack(pady=10)

sync_btn = ttk.Button(menu_tab, text="Sync Requirements", command=on_sync)
sync_btn.pack(pady=10)

# Terminal tab UI for shell commands
shell_frame = ttk.Frame(terminal_tab)
shell_frame.pack(fill="x", pady=5)

shell_entry = ttk.Entry(shell_frame, width=80)
shell_entry.pack(side="left", padx=5)

shell_btn = ttk.Button(shell_frame, text="Run Command", command=run_shell_command)
shell_btn.pack(side="left", padx=5)

root.mainloop()
# if __name__ == "__main__":
#     pass
