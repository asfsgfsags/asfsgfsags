import sys
import time

def main():
    # --- دریافت ورودی‌ها (فاز اول) ---
    fruit_passport = input("Enter Your Fruit Passport: ").strip()
    power = int(input('How much is your mining power? >> '))
    capacity = int(input('How much is your mining capacity? >> '))
    deposit_ask = input('Do you want to deposit money in the bank? (Y or N) >> ').lower() == 'y'

    # --- محاسبات اولیه ---
    maining_time = int((capacity / (power / 3600)))
    print(f"\nMine time is: {maining_time} sec\n")

    # --- آنلود ماژول‌های غیرضروری بعد از دریافت ورودی‌ها ---
    del power, capacity  # حذف متغیرهای غیرضروری
    import requests  # تأخیر در ایمپورت برای کاهش مصرف اولیه RAM

    # --- تنظیم هدرها و داده‌ها ---
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

    # --- عملیات اصلی (فاز دوم) ---
    done = lost = 0
    try:
        for _ in range(400):
            start_time = time.time()
            
            # جمع‌آوری طلا (با منطق تلاش مجدد)
            for attempt in range(3):
                try:
                    response = requests.post(
                        "http://iran.fruitcraft.ir/cards/collectgold",
                        data=collect_data,
                        headers=headers,
                        timeout=5
                    )
                    if response.ok:
                        done += 1
                        break
                except (requests.RequestException, ValueError):
                    if attempt == 2:
                        lost += 1
                    time.sleep(0.5)

            # واریز به بانک (فقط اگر فعال باشد)
            if deposit_ask:
                for _ in range(3):
                    try:
                        requests.post(
                            "http://iran.fruitcraft.ir/player/deposittobank",
                            data=deposit_data,
                            headers=headers,
                            timeout=3
                        )
                        break
                    except:
                        pass

            # نمایش وضعیت (سبک‌ترین روش ممکن)
            sys.stdout.write(f"\rDone: {done} | Lost: {lost} | Deposit: {'Y' if deposit_ask else 'N'}")
            sys.stdout.flush()

            # بهینه‌سازی sleep با کسر زمان اجرای عملیات
            elapsed = time.time() - start_time
            time.sleep(max(0, maining_time - elapsed))

    except KeyboardInterrupt:
        print("\nScript stopped by user")
    finally:
        # پاک‌سازی نهایی
        if 'requests' in sys.modules:
            del sys.modules['requests']
        del headers, collect_data, deposit_data

if __name__ == "__main__":
    main()
