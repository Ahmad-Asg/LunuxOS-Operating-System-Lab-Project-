# 🚀 LunuxOS – Mini Operating System Simulation in C

## 📖 Overview

LunuxOS is a mini operating system simulation project developed in the C programming language. The project is designed to demonstrate the core concepts of Operating Systems including process management, scheduling, synchronization, memory management, deadlock handling, inter-process communication, file operations, and utility applications.

The main goal of this project is to provide a practical implementation of important operating system concepts in a simple and understandable way. LunuxOS simulates how an operating system manages multiple tasks and system resources while also providing various built-in applications and utilities.

This project was created as an academic operating system project and focuses on modular programming, structured system design, and basic kernel-level concepts.

---

# ✨ Features

## 🖥️ Core Operating System Modules

### ⚙️ Kernel Management

The kernel module is the core component of LunuxOS. It manages system initialization, task execution, communication between modules, and overall system control.

Functions performed by the kernel:
- System boot process
- Task initialization
- System control management
- Resource coordination
- Communication with scheduler and memory modules

---

### 🧠 Process Scheduling

The scheduler module simulates CPU scheduling techniques used in real operating systems.

Functions performed:
- Task queue management
- Process execution handling
- CPU time allocation
- Simulated multitasking
- Task switching

This helps demonstrate how operating systems manage multiple running programs.

---

### 💾 Memory Management

The memory module simulates memory allocation and deallocation.

Functions performed:
- Dynamic memory allocation
- Memory release handling
- Simulated RAM management
- Memory tracking
- Resource optimization

This module helps users understand how operating systems manage system memory efficiently.

---

### 🔄 Synchronization System

The synchronization module demonstrates synchronization concepts used in concurrent systems.

Functions performed:
- Critical section handling
- Shared resource protection
- Synchronization control
- Thread-safe operations

This module helps in understanding race conditions and process coordination.

---

### ⚠️ Deadlock Handling

The deadlock module demonstrates how deadlocks occur and how systems handle them.

Functions performed:
- Deadlock simulation
- Resource allocation handling
- Process dependency management
- Safe execution simulation

This helps explain one of the most important concepts in Operating Systems.

---

### 📡 Inter-Process Communication (IPC)

The IPC module demonstrates communication between processes.

Functions performed:
- Message passing
- Process communication simulation
- Shared information exchange
- Coordination between tasks

This module represents how processes communicate in real operating systems.

---

# 🛠️ Utility Applications and Tasks

LunuxOS also includes multiple built-in utility programs and games that run inside the operating system simulation.

## 📁 File Management Utilities

### 📄 File Creator
Creates new files inside the system.

### 📋 File Copier
Copies contents from one file to another.

### 📂 File Mover
Moves files between directories.

### ❌ File Deleter
Deletes files from the system.

### 🔍 File Information Viewer
Displays details and metadata of files.

These utilities simulate real operating system file handling operations.

---

## 📊 Monitoring Utilities

### 🖥️ CPU Monitor
Displays CPU usage and monitoring information.

### 💽 RAM Monitor
Displays simulated memory usage statistics.

These utilities demonstrate system resource monitoring.

---

## 📌 Productivity Utilities

### ✏️ Text Editor
A simple built-in text editor for writing and editing text files.

### 📝 Notepad
A lightweight text writing utility.

### 🧮 Calculator
Performs basic arithmetic operations.

### 📅 Calendar
Displays dates and calendar information.

### ⏰ Clock
Shows current system time.

### ⌛ Timer
Provides countdown timer functionality.

### ⏱️ Stopwatch
Measures elapsed time.

### ⌨️ Typing Speed Test
Measures typing speed and accuracy.

### 💻 Shell
Simulates a basic command-line shell interface.

---

## 🎮 Entertainment and Mini Games

### ❎ Tic Tac Toe
A console-based Tic Tac Toe game.

### 💣 Minesweeper
A simplified console version of the Minesweeper game.

### 🎯 Guessing Game
A number guessing game for users.

---

## 🔊 Sound Utility

### 🔔 Beep Utility
Produces beep sounds and demonstrates simple audio interaction.

---

# 📂 Project Structure

```bash
LunuxOS/
│
├── kernel/
├── scheduler/
├── memory/
├── synchronization/
├── deadlock/
├── ipc/
├── filesystem/
├── tasks/
├── include/
├── main.c
├── Makefile
└── lunuxos.sh
```
# 🧑‍💻 Technologies Used

- C Programming Language
- GCC Compiler
- Linux Environment / Ubuntu
- Makefile Build System
- Modular Programming Concepts

---

# 📚 Concepts Implemented

This project demonstrates the following Operating System concepts:

- Process Management
- CPU Scheduling
- Memory Management
- Synchronization
- Deadlock Handling
- Inter-Process Communication
- File Handling
- Command Line Interface
- Multitasking Simulation
- Resource Management
- Utility Program Integration

---

# ▶️ How to Run the Project
## Step 1: Clone the Repository
```bash
git clone https://github.com/Ahmad-Asg/LunuxOS-Operating-System-Lab-Project-
```
## Step 2: Move into the Project Directory
```bash
cd LunuxOS
```
## Step 3: Compile the Project
```bash
make
```
## Step 4: Run the Operating System
```bash
./lunuxos
```
# 🖥️ System Requirements

- Linux / Ubuntu Operating System
- GCC Compiler Installed
- Make Utility Installed
- Terminal Environment

---

# 🎓 Educational Purpose

LunuxOS was developed mainly for educational and learning purposes. The project helps students understand how operating systems work internally by implementing simplified versions of important OS components.

The project also improves:

- Problem solving skills
- Modular programming
- System-level programming knowledge
- Understanding of OS architecture
- Team collaboration and software design skills

---

# 🚧 Future Improvements

Possible future enhancements for LunuxOS:

- GUI-based interface
- Advanced process scheduling algorithms
- Better memory paging simulation
- User authentication system
- Virtual file system
- Networking support
- Multi-user environment
- Advanced shell commands
- Improved task management
- Better graphics and animations

---

# 📖 Learning Outcomes

By developing this project, the following concepts were learned and implemented:

- Operating System architecture
- Kernel and scheduler interaction
- Process and resource management
- File handling in C
- Modular project organization
- Memory allocation techniques
- Synchronization concepts
- Team-based software development
- Linux compilation and execution process

---

# 👨‍💻 Contributors

Developed by the LunuxOS Project Team.

Special contributions include:

- Kernel Development
- Scheduling System
- Utility Applications
- Game Development
- File Management Modules
- Synchronization and IPC Modules

---

# ✅ Conclusion

LunuxOS is a complete mini operating system simulation project built using C language. The project combines important operating system concepts with practical utilities and mini applications to provide both educational value and interactive functionality.

It serves as an excellent learning project for students who want to understand the internal working of operating systems and gain hands-on experience in low-level programming and system design.
