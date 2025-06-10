import concurrent.futures
import requests
from requests.adapters import HTTPAdapter
from urllib3.util.retry import Retry
from time import sleep
import sys
from uuid import uuid4
from json import loads

# تنظیمات اولیه
RESTORE_KEY = input("Enter restore key: ")
POOL_SIZE = 10
POOL_CONNECTIONS = 10
MAX_RETRIES = 5
BACKOFF_FACTOR = 0.1

# ایجاد session بهینه‌شده
def create_session():
    session = requests.Session()
    retry_strategy = Retry(
        total=MAX_RETRIES,
        backoff_factor=BACKOFF_FACTOR,
        status_forcelist=[500, 502, 503, 504]
    )
    adapter = HTTPAdapter(
        pool_connections=POOL_CONNECTIONS,
        pool_maxsize=POOL_SIZE,
        max_retries=retry_strategy
    )
    session.mount('http://', adapter)
    session.mount('https://', adapter)
    session.headers.update({
        'User-Agent': 'Dalvik/2.1.0 (Linux; U; Android 13; 23049PCD8G Build/TKQ1.221114.001)',
        'Accept-Encoding': 'gzip',
        'Content-Type': 'application/x-www-form-urlencoded; charset=UTF-8'
    })
    return session

# توابع اصلی
def load_data(session):
    data = {
        'game_version': '1.7.10655',
        'device_name': 'unknown',
        'os_version': '10',
        'model': 'SM-A750F',
        'udid': str(uuid4().int),
        'store_type': 'iraqapps',
        'restore_key': RESTORE_KEY,
        'os_type': 2
    }
    return loads(session.post('http://iran.fruitcraft.ir/player/load', data=data, timeout=10).text)

def battle_quest(session, cards, q):
    data = {'cards': ','.join(map(str, cards))}
    return loads(session.get('http://iran.fruitcraft.ir/battle/quest', params=data, timeout=10).text)

def main():
    session = create_session()
    
    try:
        load_result = load_data(session)
        cards = [card['id'] for card in load_result['data']['cards'] if card['power'] < 100]
        
        if len(cards) < 20:
            print("You have less than 20 cards!")
            return

        q = load_result['data']['q']
        count = xp = gold = 0
        level = load_result['data']['level']

        with concurrent.futures.ThreadPoolExecutor(max_workers=20) as executor:
            while True:
                try:
                    future = executor.submit(battle_quest, session, [cards[0]], q)
                    q = future.result()
                    
                    # Rotate cards
                    if cards:
                        cards.append(cards.pop(0))

                    if q['status']:
                        xp += q['data']['xp_added']
                        gold += q['data']['gold_added']

                        if q['data']['xp_added'] == 0:
                            print("\nMission finished or lost. Stopping the bot.")
                            break
                        elif 'level' in q['data'] and q['data']['level'] > level:
                            level = q['data']['level']
                            print(f"\nLevel up to {level}!")

                    print(f"\rLevel: {level} | Gold: {gold} | Wins: {count}", end="")
                    count += 1

                except KeyboardInterrupt:
                    print("\nStopped by user")
                    break
                except Exception as e:
                    print(f"\nError: {e}. Retrying...")
                    sleep(5)
                    continue

    finally:
        session.close()

if __name__ == "__main__":
    main()
