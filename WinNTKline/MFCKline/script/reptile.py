import mechanize
import cookielib
import threading
import sys
from time import ctime,sleep
url = 'https://www.bilibili.com/p/m/264478'
def run():
    for i in range(100):
        if browse() == True: 
            print i 
			#,"\n" 
        sleep(1)
def browse():
    global url
    br = mechanize.Browser()
    cj = cookielib.LWPCookieJar()
    br.set_cookiejar(cj)
    br.set_handle_equiv(True)
    br.set_handle_gzip(True)
    br.set_handle_redirect(True)
    br.set_handle_referer(True)
    br.set_handle_robots(False)
    br.set_handle_refresh(mechanize._http.HTTPRefreshProcessor(), max_time=1)
    br.addheaders = [('User-agent', 'Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.9.0.1) Gecko/2008071615 Fedora/3.0.1-1.fc9 Firefox/3.0.1')]
    r = br.open(url)
    html = r.read()
	br.close()
    #print html
    return True
if __name__ == '__main__':
    threads = []
    if len(sys.argv) > 1:
        url=sys.argv[1]
    '''    
    else    
        url = raw_input("Please enter a web address: \n> ")
    '''    
    print "refreshing:\t" + url
    for i in range(387):
        t1=threading.Thread(target=run)
        threads.append(t1)
    for t in threads:
        t.setDaemon(True)
        t.start()
    print "all over %s" %ctime()
#run()
