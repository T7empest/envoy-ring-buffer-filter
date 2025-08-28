import requests
import concurrent.futures
import time
import random
from colorama import Fore, Style, init

init(autoreset=True)

BASE_URL = "http://localhost:10000"

def fetch(name, path):
    """Send a request and return status + headers summary"""
    url = BASE_URL + path
    try:
        r = requests.get(url, timeout=10)
        status = r.status_code
        hit_status = r.headers.get("cache-hit-status", "none")
        color = {
            "cache_miss": Fore.YELLOW,
            "coalesced_hit": Fore.CYAN,
            "cache_hit": Fore.GREEN,
        }.get(hit_status, Fore.WHITE)

        return f"{Fore.MAGENTA}[{name}]{Style.RESET_ALL} {color}{status}, {hit_status}{Style.RESET_ALL}, len={len(r.text)}"
    except Exception as e:
        return f"{Fore.MAGENTA}[{name}]{Style.RESET_ALL} {Fore.RED}FAILED: {e}{Style.RESET_ALL}"

def demo_coalescing():
    # use a unique path so cache is empty
    unique_path = f"/api/slow/{random.randint(1000,9999)}"
    print(f"{Fore.BLUE}>>> Testing coalescing on fresh path: {unique_path}{Style.RESET_ALL}")

    # send two requests at once
    with concurrent.futures.ThreadPoolExecutor(max_workers=2) as executor:
        futures = [
            executor.submit(fetch, "req1", unique_path),
            executor.submit(fetch, "req2", unique_path),
        ]
        for f in concurrent.futures.as_completed(futures):
            print(f.result())

    print()

def demo_cache_hit():
    path = "/api/slow/hit"
    print(f"{Fore.BLUE}>>> Warm up cache for {path}{Style.RESET_ALL}")
    print(fetch("warmup", path))

    time.sleep(1)
    print(f"{Fore.BLUE}>>> Request again, should be cache_hit{Style.RESET_ALL}")
    print(fetch("cache_hit", path))
    print()

def main():
    demo_coalescing()
    demo_coalescing()
    demo_cache_hit()
    print(f"{Fore.GREEN}>>> Demo complete{Style.RESET_ALL}")

if __name__ == "__main__":
    main()