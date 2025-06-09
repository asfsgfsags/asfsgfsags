import subprocess
import sys

required_packages = ["PyQt6", "requests", "urllib3"]

for package in required_packages:
    try:
        __import__(package)
    except ImportError:
        print(f"Package '{package}' not found. Installing...")
        subprocess.check_call([sys.executable, "-m", "pip", "install", package])
import sys
import threading
import queue
import time
from uuid import uuid4
from json import loads, JSONDecodeError

from PyQt6.QtWidgets import (
    QApplication, QWidget, QVBoxLayout, QHBoxLayout, QLabel, QLineEdit,
    QPushButton, QTextEdit, QMessageBox
)
from PyQt6.QtCore import QTimer
from PyQt6.QtGui import QFont

import requests
from requests.adapters import HTTPAdapter
from urllib3.util.retry import Retry

# ========== تابع اتکر ==========

def attacker_script(restore_key, output_queue, stop_flag):
    pool_size = 10
    pool_connections = 10
    pool_max_retries = 5
    pool_backoff_factor = 0.1

    connection_pool = HTTPAdapter(pool_connections=pool_connections, pool_maxsize=pool_size,
                                  max_retries=Retry(total=pool_max_retries, backoff_factor=pool_backoff_factor))

    session = requests.Session()
    session.mount('http://', connection_pool)
    session.mount('https://', connection_pool)
    session.headers.update({
        'User-Agent': 'Dalvik/2.1.0 (Linux; U; Android 13; 23049PCD8G Build/TKQ1.221114.001)',
        'Accept-Encoding': 'gzip',
        'Content-Type': 'application/x-www-form-urlencoded; charset=UTF-8'
    })

    def load():
        data = {
            'game_version': '1.7.10655',
            'device_name': 'unknown',
            'os_version': '10',
            'model': 'SM-A750F',
            'udid': str(uuid4().int),
            'store_type': 'iraqapps',
            'restore_key': restore_key,
            'os_type': 2
        }
        response = session.post('http://iran.fruitcraft.ir/player/load', data=data, timeout=10)
        return loads(response.text)

    def battle_quest(cards, q, hero_id=None):
        data = {'cards': ','.join(map(str, cards))}
        response = session.get('http://iran.fruitcraft.ir/battle/quest', params=data, timeout=10)
        return loads(response.text)

    def update_cards(cards):
        if cards:
            cards.append(cards[0])
            cards.pop(0)

    try:
        load_data = load()
    except Exception as e:
        output_queue.put(f"[ERROR] Loading data failed: {e}\n")
        return

    cards = [card['id'] for card in load_data['data']['cards'] if card['power'] < 100]

    if len(cards) < 20:
        output_queue.put("[ERROR] You have less than 20 cards!\n")
        return

    q = load_data['data']['q']
    count = 0
    xp = 0
    gold = 0
    level = load_data['data']['level']

    retrying_strategy = Retry(total=5, backoff_factor=0.1)
    adapter = HTTPAdapter(pool_connections=pool_connections, pool_maxsize=pool_size, max_retries=retrying_strategy)
    session.mount('http://', adapter)
    session.mount('https://', adapter)

    while not stop_flag['stop']:
        try:
            count += 1
            q = battle_quest([cards[0]], q)
            update_cards(cards)

            if q['status']:
                xp += q['data']['xp_added']
                gold += q['data']['gold_added']

                if q['data']['xp_added'] == 0:
                    output_queue.put("[INFO] Mission finished or lost. Stopping the bot.\n")
                    break
                elif 'level' in q['data'] and q['data']['level'] > level:
                    level = q['data']['level']
                    output_queue.put(f"[LEVEL UP] Congratulations! You leveled up to level {level}!\n")
            else:
                output_queue.put(f"[ERROR] Mission response error: {q}\n")

        except JSONDecodeError:
            time.sleep(5)
            continue
        except requests.exceptions.RequestException as e:
            output_queue.put(f"[ERROR] Request exception: {e}. Retrying in 10 seconds...\n")
            time.sleep(10)
            continue
        except Exception as e:
            output_queue.put(f"[EXCEPTION] {e}\n")
            break

        stats = (
            f"════════════════════════════════════════════\n"
            f"Level: {level}\n"
            f"Gold: {gold}\n"
            f"Wins: {count}\n"
        )
        output_queue.put(stats)

    output_queue.put("Attacker script finished.\n")

# ======= GUI با PyQt6 ========

class AttackerGUI(QWidget):
    def __init__(self):
        super().__init__()

        self.setWindowTitle("Cactus Attack Bot")
        self.setMinimumSize(700, 500)

        layout = QVBoxLayout()

        input_layout = QHBoxLayout()

        self.restore_key_input = QLineEdit()
        self.restore_key_input.setPlaceholderText("Enter restore key")

        input_layout.addWidget(self.restore_key_input)

        self.start_button = QPushButton("Start Attack")
        self.stop_button = QPushButton("Stop Attack")
        self.stop_button.setEnabled(False)

        input_layout.addWidget(self.start_button)
        input_layout.addWidget(self.stop_button)

        layout.addLayout(input_layout)

        self.output_console = QTextEdit()
        self.output_console.setReadOnly(True)
        self.output_console.setFont(QFont("Courier New", 10))
        layout.addWidget(self.output_console)

        self.setLayout(layout)

        self.output_queue = queue.Queue()
        self.stop_flag = {'stop': False}

        self.timer = QTimer()
        self.timer.setInterval(100)
        self.timer.timeout.connect(self.update_output)
        self.timer.start()

        self.attack_thread = None

        self.start_button.clicked.connect(self.start_attack)
        self.stop_button.clicked.connect(self.stop_attack)

    def start_attack(self):
        if self.attack_thread and self.attack_thread.is_alive():
            QMessageBox.warning(self, "Warning", "Attack already running!")
            return

        restore_key = self.restore_key_input.text().strip()
        if not restore_key:
            QMessageBox.warning(self, "Input Error", "Please enter a restore key.")
            return

        self.output_console.clear()
        with self.output_queue.mutex:
            self.output_queue.queue.clear()

        self.stop_flag['stop'] = False

        self.attack_thread = threading.Thread(
            target=attacker_script,
            args=(restore_key, self.output_queue, self.stop_flag),
            daemon=True
        )
        self.attack_thread.start()
        self.start_button.setEnabled(False)
        self.stop_button.setEnabled(True)

    def stop_attack(self):
        self.stop_flag['stop'] = True
        self.start_button.setEnabled(True)
        self.stop_button.setEnabled(False)
        self.output_console.append("[INFO] Stopping attack...")

    def update_output(self):
        while not self.output_queue.empty():
            text = self.output_queue.get_nowait()
            self.output_console.append(text)

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = AttackerGUI()
    window.show()
    sys.exit(app.exec())
