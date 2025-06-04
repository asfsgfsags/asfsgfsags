from requests import post
from time import sleep
import sys
from colorama import Fore, Back 
from os import system


fruit_passport = str(input("Enter Your Fruit Passport: "))


headers = {
    'Content-Type': 'application/x-www-form-urlencoded; charset=UTF-8',
    'User-Agent': 'Dalvik/2.1.0 (Linux; U; Android 10; SM-A750F Build/QP1A.190711.020)',
    'Host': 'iran.fruitcraft.ir',
    'Connection': 'Keep-Alive',
    'Accept-Encoding': 'gzip',
    'cookie': f'FRUITPASSPORT={fruit_passport}'}


collect_data = "edata=Gk4KXVpRXRJDSEMTfmMXSA%3D%3D"
deposit_data = "edata=Gk4KUEFQQERbUDpPAwkBAVRZRFQ4UB4aWwoEEA5GW05bAlUECgRTQ1JIBVQEUAVdFwhSQAAJCFsDF1BRBVoMBhFJ"


print("\n\n\n")


def collect_gold(data, headers):
    retry_count = 0
    while retry_count < 3:
        try:
            collect = post("http://iran.fruitcraft.ir/cards/collectgold",
                           data=data,
                           headers=headers)
            return collect
        except Exception as e:
            print(e)
            retry_count += 1
            sleep(0.5)
    return None


def deposit_to_bank(data, headers):
    retry_count = 0
    while retry_count < 3:
        try:
            deposit = post("http://iran.fruitcraft.ir/player/deposittobank",
                           data=data,
                           headers=headers)
            return deposit
        except Exception as e:
            print(e)
            retry_count += 1
            sleep(0.5)
    return None


def start():
    done = 0
    lost = 0
    
    for i in range(400):
        try:
            collect_result = collect_gold(collect_data, headers)
            if collect_result is not None:
                if deposit_ask:
                    deposit_result = deposit_to_bank(deposit_data, headers)
                    if deposit_result is None:
                        lost += 1
                done += 1
        except Exception as e:
            print(e)
            lost += 1
        finally:
            sys.stdout.write(f"\r{Fore.LIGHTBLUE_EX}╔════════════════════════════════════════════╗\n"
                             f"║          {Fore.RESET}• Gold Mine Done: {Fore.GREEN}{str(done)}{Fore.RESET}                  ║\n"
                             f"║          {Fore.RESET}• Gold Mine Lost: {Fore.RED}{str(lost)}{Fore.RESET}                 ║\n"
                             f"║          {Fore.RESET}• Bank Deposit: {str(deposit_ask)}               ║\n"
                             f"╚════════════════════════════════════════════╝")
            sys.stdout.flush()
        sleep(maining_time)


system("clear")
power = input('How much is your mining power? >> ') 

capacity = input('How much is your mining capacity? >> ') 

deposit_ask = input('Do you want to deposit money in the bank? (Y or N) >> ') 
deposit_ask = True if deposit_ask.lower() == "y" else False
    

maining_time = int((int(capacity) / (int(power) / 3600)))

print("\n\n\n\n" , f"{Fore.LIGHTBLUE_EX}╔════════════════════════════════════════════╗\n"
                     f"║       {Fore.RESET}Mine time is: {Fore.CYAN}{maining_time}{Fore.RESET} sec        ║\n"
                     f"╚════════════════════════════════════════════╝\n")

print(f"\n\n Maedan bot edited by: {Fore.RED}Cactus")

start()
