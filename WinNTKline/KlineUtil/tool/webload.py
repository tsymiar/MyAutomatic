import mechanize
import http.cookiejar
import threading
import sys
from time import ctime, sleep
url = 'https://github.com/tsymiar/MyAutomatic'
def run():
    for i in range(100):
        if browse() == True:
            print('%s: %d\n' % (threading.current_thread().name, i))
            # ,"\n"
        sleep(0.01)
def browse():
    br = mechanize.Browser()
    cj = http.cookiejar.LWPCookieJar()
    br.set_cookiejar(cj)
    br.set_handle_equiv(True)
    br.set_handle_gzip(True)
    br.set_handle_redirect(True)
    br.set_handle_referer(True)
    br.set_handle_robots(False)
    br.set_handle_refresh(mechanize._http.HTTPRefreshProcessor(), max_time = 1)
    br.addheaders = [('User-agent', 'Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.9.0.1) Gecko/2008071615 Fedora/3.0.1-1.fc9 Firefox/3.0.1')]
    # global url
    rd = br.open(url)
    html = rd.read()
    br.close()
    if not html.strip():
        return False
    else:
        return True
if __name__ == '__main__':
    threads = []
    if len(sys.argv) > 1:
        url = sys.argv[1]
    #else
    #    url = raw_input("Please enter a web address: \n> ")
    print("Refreshing:\t" + url)
    for i in range(300):
        t1 = threading.Thread(target = run)
        threads.append(t1)
    for t in threads:
        t.daemon = True
        t.start()
        t.join()
    print("all over %s" %ctime())
