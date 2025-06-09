import subprocess
import sys

required_packages = ["PyQt6", "requests", "colorama"]

for package in required_packages:
    try:
        __import__(package)
    except ImportError:
        print(f"Package '{package}' not found. Installing...")
        subprocess.check_call([sys.executable, "-m", "pip", "install", package])

# بقیه کد از اینجا شروع میشه
import threading
import queue
import time
from PyQt6.QtWidgets import (
    QApplication, QWidget, QVBoxLayout, QLabel, QLineEdit, QPushButton,
    QTextEdit, QHBoxLayout, QCheckBox, QMessageBox
)
from PyQt6.QtCore import Qt, QTimer

import requests
from colorama import Fore
import sys
import threading
import queue
import time
from PyQt6.QtWidgets import (
    QApplication, QWidget, QVBoxLayout, QLabel, QLineEdit, QPushButton,
    QTextEdit, QHBoxLayout, QCheckBox, QMessageBox
)
from PyQt6.QtCore import Qt, QTimer

import requests
from colorama import Fore  # فقط برای رنگ‌آمیزی است، در GUI لازم نیست

# ---------- اسکریپت ماینر در قالب تابع (کمی تغییر داده شده برای خروجی گرفتن) -----------

def miner_script(fruit_passport, power, capacity, deposit_ask, output_queue):

    headers = {
        'Content-Type': 'application/x-www-form-urlencoded; charset=UTF-8',
        'User-Agent': 'Dalvik/2.1.0 (Linux; U; Android 10; SM-A750F Build/QP1A.190711.020)',
        'Host': 'iran.fruitcraft.ir',
        'Connection': 'Keep-Alive',
        'Accept-Encoding': 'gzip',
        'cookie': f'FRUITPASSPORT={fruit_passport}'
    }

    collect_data = "edata=Gk4KXVpRXRJDSEMTfmMXSA%3D%3D"
    deposit_data = "edata=Gk4KUEFQQERbUDpPAwkBAVRZRFQ4UB4aWwoEEA5GW05bAlUECgRTQ1JIBVQEUAVdFwhSQAAJCFsDF1BRBVoMBhFJ"

    def collect_gold():
        retry_count = 0
        while retry_count < 3:
            try:
                collect = requests.post("http://iran.fruitcraft.ir/cards/collectgold",
                                        data=collect_data,
                                        headers=headers)
                return collect
            except Exception as e:
                output_queue.put(f"[ERROR] collect_gold: {e}\n")
                retry_count += 1
                time.sleep(0.5)
        return None

    def deposit_to_bank():
        retry_count = 0
        while retry_count < 3:
            try:
                deposit = requests.post("http://iran.fruitcraft.ir/player/deposittobank",
                                        data=deposit_data,
                                        headers=headers)
                return deposit
            except Exception as e:
                output_queue.put(f"[ERROR] deposit_to_bank: {e}\n")
                retry_count += 1
                time.sleep(0.5)
        return None

    done = 0
    lost = 0

    maining_time = int((int(capacity) / (int(power) / 3600)))
    output_queue.put(f"Mine time is: {maining_time} sec\n")
    output_queue.put("Starting mining...\n")

    for i in range(400):
        try:
            collect_result = collect_gold()
            if collect_result is not None:
                if deposit_ask:
                    deposit_result = deposit_to_bank()
                    if deposit_result is None:
                        lost += 1
                done += 1
            else:
                lost += 1
        except Exception as e:
            output_queue.put(f"[EXCEPTION] {e}\n")
            lost += 1
        finally:
            status = (
                f"════════════════════════════════════════════\n"
                f"• Gold Mine Done: {done}\n"
                f"• Gold Mine Lost: {lost}\n"
                f"• Bank Deposit: {deposit_ask}\n"
            )
            output_queue.put(status)
        time.sleep(maining_time)

    output_queue.put("Mining finished.\n")

# ------------- GUI با PyQt6 -----------------

from PyQt6.QtGui import QFont

class MinerGUI(QWidget):
    def __init__(self):
        super().__init__()

        self.setWindowTitle("Mine Bot")
        self.setMinimumSize(700, 500)

        layout = QVBoxLayout()

        # ورودی‌ها
        form_layout = QHBoxLayout()

        self.passport_input = QLineEdit()
        self.passport_input.setPlaceholderText("Fruit Passport")

        self.power_input = QLineEdit()
        self.power_input.setPlaceholderText("Mining Power ")

        self.capacity_input = QLineEdit()
        self.capacity_input.setPlaceholderText("Mining Capacity ")

        self.deposit_checkbox = QCheckBox("Deposit money to bank")

        form_layout.addWidget(self.passport_input)
        form_layout.addWidget(self.power_input)
        form_layout.addWidget(self.capacity_input)
        form_layout.addWidget(self.deposit_checkbox)

        layout.addLayout(form_layout)

        # دکمه استارت
        self.start_button = QPushButton("Start Mining")
        layout.addWidget(self.start_button)

        # خروجی ترمینال شبیه
        self.output_console = QTextEdit()
        self.output_console.setReadOnly(True)
        self.output_console.setFont(QFont("Courier New", 10))
        layout.addWidget(self.output_console)

        self.setLayout(layout)

        # صف برای ارتباط بین thread و GUI
        self.output_queue = queue.Queue()

        # تایمر برای خواندن خروجی از صف و نمایش در GUI
        self.timer = QTimer()
        self.timer.setInterval(100)  # هر 100 میلی‌ثانیه
        self.timer.timeout.connect(self.update_output)
        self.timer.start()

        # اتصال دکمه
        self.start_button.clicked.connect(self.start_mining)

        self.mining_thread = None

    def start_mining(self):
        if self.mining_thread and self.mining_thread.is_alive():
            QMessageBox.warning(self, "Warning", "Mining already running!")
            return

        passport = self.passport_input.text().strip()
        power = self.power_input.text().strip()
        capacity = self.capacity_input.text().strip()
        deposit = self.deposit_checkbox.isChecked()

        if not passport or not power.isdigit() or not capacity.isdigit():
            QMessageBox.warning(self, "Input Error", "Please enter valid inputs.")
            return

        self.output_console.clear()
        self.output_queue.queue.clear()  # پاک کردن صف خروجی

        self.mining_thread = threading.Thread(
            target=miner_script,
            args=(passport, int(power), int(capacity), deposit, self.output_queue),
            daemon=True
        )
        self.mining_thread.start()

    def update_output(self):
        while not self.output_queue.empty():
            text = self.output_queue.get_nowait()
            self.output_console.append(text)

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = MinerGUI()
    window.show()
    sys.exit(app.exec())
