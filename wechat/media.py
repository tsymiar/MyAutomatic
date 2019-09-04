# -*- coding: utf-8 -*-
# filename: media.py
import urllib2
import json
from basic import Basic
import poster.encode
from poster.streaminghttp import register_openers
import traceback
import sys
import logging

logging.basicConfig(level=logging.INFO,  
                    format='%(asctime)s %(filename)s[line:%(lineno)d] %(levelname)s %(message)s',  
                    datefmt='%a, %d %b %Y %H:%M:%S',  
                    filename='/tmp/wechat.log',  
                    filemode='w')                    
def upload_image(filePath):
    myMedia = Media()
    accessToken = Basic().get_access_token()
    mediaType = "image"
    return myMedia.uplaod(accessToken, filePath, mediaType)
def download_image(mediaId, saveName = "material.jpg"):
    myMedia = Media()
    accessToken = Basic().get_access_token()
    myMedia.get(accessToken, mediaId, saveName)

class Media(object):
    def __init__(self):
        register_openers()
    def dict_get(self, dict_str, key_t):
        for key in dict_str.keys():
            if key==key_t:
                return dict_str.get(key_t)
        return None
    def get(self, accessToken, mediaId, saveName):
        postUrl = "https://api.weixin.qq.com/cgi-bin/media/get?access_token=%s&media_id=%s" % (accessToken, mediaId)
        urlResp = urllib2.urlopen(postUrl)
        headers = urlResp.info().__dict__['headers']
        if ('Content-Type: application/json\r\n' in headers) or ('Content-Type: text/plain\r\n' in headers):
            jsonDict = json.loads(urlResp.read())
            logging.info("GET jsonDict: " + jsonDict)
        else:
            buffer = urlResp.read()   #素材的二进制
            mediaFile = file(saveName, "wb")
            mediaFile.write(buffer)
            logging.info("get successful")
    #上传图片
    def uplaod(self, accessToken, filePath, mediaType):
        try:
            openFile = open(filePath, "rb")
        except:
            logging.warning(traceback.format_exc())
        param = {'media': openFile}
        postData, postHeaders = poster.encode.multipart_encode(param)
        postUrl = "https://api.weixin.qq.com/cgi-bin/media/upload?access_token=%s&type=%s" % (accessToken, mediaType)
        request = urllib2.Request(postUrl, postData, postHeaders)
        urlResp = urllib2.urlopen(url=request, timeout=15)
        try:
            logging.info('Upload status:\n[' + bytes(urlResp.getcode()) + "] : " + urlResp.geturl() + "\n" + str(urlResp.info()))
        except:
            logging.error(traceback.format_exc())
        jsonDict = dict()
        try:
            jsonDict = json.loads(urlResp.read())#.decode(urlResp.info().get_param('charset') or 'utf-8'))
        except:
            logging.warning(traceback.format_exc())
            if type(jsonDict) is not dict:
                sys.exit(0)
        logging.info("POST jsonDict =" + str(jsonDict))
        mediaId = self.dict_get(jsonDict, 'media_id')
        if self.dict_get(jsonDict, 'type') == "image" and mediaId:
            return mediaId
        else:
            return None
