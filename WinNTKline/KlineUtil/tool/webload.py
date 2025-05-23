import mechanize
import http.cookiejar
import threading
import sys
from time import ctime, sleep
from bs4 import BeautifulSoup
from http.client import RemoteDisconnected

url = "https://github.com/tsymiar/MyAutomatic"
loop_count = 100


def run():
    for i in range(loop_count):
        content = browse()
        if content:
            print(
                "%s: %d - Content: %s\n" % (threading.current_thread().name, i, content)
            )
        else:
            print(
                "%s: %d - No Useful Content Found\n"
                % (threading.current_thread().name, i)
            )
        sleep(0.01)


def browse():
    try:
        br = mechanize.Browser()
        cj = http.cookiejar.LWPCookieJar()
        br.set_cookiejar(cj)
        br.set_handle_equiv(True)
        br.set_handle_gzip(True)
        br.set_handle_redirect(True)
        br.set_handle_referer(True)
        br.set_handle_robots(False)
        br.set_handle_refresh(mechanize._http.HTTPRefreshProcessor(), max_time=1)
        br.addheaders = [
            (
                "User-agent",
                "Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.9.0.1) Gecko/2008071615 Fedora/3.0.1-1.fc9 Firefox/3.0.1",
            )
        ]

        rd = br.open(url)
        html = rd.read()
        br.close()

        if not html.strip():
            return None

        soup = BeautifulSoup(html, "html.parser")
        paragraphs = soup.find_all("p")
        content = " ".join([p.get_text() for p in paragraphs if p.get_text().strip()])
        return content if content else None
    except RemoteDisconnected:
        print("Remote end closed connection without response")
        return None


if __name__ == "__main__":
    threads = []
    if len(sys.argv) > 1:
        url = sys.argv[1]
    if len(sys.argv) > 2:
        loop_count = int(sys.argv[2])

    print("Refreshing:\t" + url)
    try:
        for i in range(loop_count):
            t1 = threading.Thread(target=run)
            threads.append(t1)
        for t in threads:
            t.daemon = True
            t.start()
            t.join()
    except KeyboardInterrupt:
        print("\nProcess interrupted by user")
    finally:
        print("all works done: %s" % ctime())
