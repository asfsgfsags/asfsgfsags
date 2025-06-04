
from rich.console import Console
from rich.progress import Progress, SpinnerColumn, TextColumn
from rich.text import Text
from rich.panel import Panel
import concurrent.futures
import requests
from requests.adapters import HTTPAdapter
from urllib3.util.retry import Retry
from os import system
from json import loads, JSONDecodeError
from uuid import uuid4
from time import sleep
import sys

console = Console()

system("clear")
console.print(Text("Script Mamoriat ROH", style="bold yellow", justify="center"))
restore_key = console.input("[bold magenta][+][/bold magenta][bold blue] Enter restore key: [/bold blue]")

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
    return loads(session.post('http://iran.fruitcraft.ir/player/load', data=data, timeout=10).text)

def battle_quest(cards, q, hero_id=None):
    data = {'cards': ','.join(map(str, cards))}
    return loads(session.get('http://iran.fruitcraft.ir/battle/quest', params=data, timeout=10).text)

def update_cards(cards):
    if cards:
        cards.append(cards[0])
        cards.pop(0)

try:
    load_data = load()
except Exception as e:
    console.print(f"[bold red]Error loading data: {e}[/bold red]")
    sys.exit()

cards = [card['id'] for card in load_data['data']['cards'] if card['power'] < 100]

if len(cards) < 20:
    console.print("[bold red]You have less than 20 cards![/bold red]")
    sys.exit()

q = load_data['data']['q']
count = 0
xp = 0
gold = 0
level = load_data['data']['level']

retrying_strategy = Retry(total=5, backoff_factor=0.1)
adapter = HTTPAdapter(pool_connections=pool_connections, pool_maxsize=pool_size, max_retries=retrying_strategy)
session.mount('http://', adapter)
session.mount('https://', adapter)

with concurrent.futures.ThreadPoolExecutor(max_workers=20) as executor:
    with Progress(SpinnerColumn(), TextColumn("[progress.description]{task.description}"), transient=True) as progress:
        task = progress.add_task("[bold magenta]Running missions...[/bold magenta]", start=False)
        progress.start_task(task)
        while True:
            try:
                count += 1
                future = executor.submit(battle_quest, [cards[0]], q)
                q = future.result()
                update_cards(cards)

                if q['status']:
                    xp += q['data']['xp_added']
                    gold += q['data']['gold_added']

                    if q['data']['xp_added'] == 0:
                        console.print(f"\n[bold orange]Mission finished or lost. Stopping the bot.[/bold orange]")
                        sys.exit()
                    elif 'level' in q['data'] and q['data']['level'] > level:
                        level = q['data']['level']
                        console.print(f"\n[bold blue]Congratulations! You leveled up to level {level}![/bold blue]")
                else:
                    console.print(f"\n[bold red]Error in mission response:[/bold red] {q}\n")

            except JSONDecodeError:
                sleep(5)
                continue
            except KeyboardInterrupt:
                sys.exit()
            except requests.exceptions.RequestException as e:
                console.print(f"\n[bold red]An error occurred: {e}. Retrying in 10 seconds...[/bold red]")
                sleep(10)
                continue

            stats_panel = Panel.fit(
                f"[bold purple]Level: [bright_cyan]{level}[/bright_cyan][/bold purple]\n"
                f"[bold purple]Gold: [bright_cyan]{gold}[/bright_cyan][/bold purple]\n"
                f"[bold purple]Wins: [bright_cyan]{count}[/bright_cyan][/bold purple]",
                title="[bold orange]Live Stats[/bold orange]",
                border_style="white",
                style="white"
            )
            console.clear()
            console.print(Text("Script Mamoriat ROH", style="bold yellow", justify="center"))
            console.print(stats_panel)
