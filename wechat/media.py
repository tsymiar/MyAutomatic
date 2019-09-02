# -*- coding: utf-8 -*-
# filename: media.py
import urllib2
import json
from basic import Basic
import poster.encode
from poster.streaminghttp import register_openers
import traceback
import sys

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
            print "GET jsonDict: ", jsonDict
        else:
            buffer = urlResp.read()   #素材的二进制
            mediaFile = file(saveName, "wb")
            mediaFile.write(buffer)
            print "get successful"
    #上传图片
    def uplaod(self, accessToken, filePath, mediaType):
        try:
            openFile = open(filePath, "rb")
        except:
            traceback.print_exc()
        param = {'media': openFile}
        postData, postHeaders = poster.encode.multipart_encode(param)
        postUrl = "https://api.weixin.qq.com/cgi-bin/media/upload?access_token=%s&type=%s" % (accessToken, mediaType)
        request = urllib2.Request(postUrl, postData, postHeaders)
        urlResp = urllib2.urlopen(url=request, timeout=15)
        print 'Upload status:\n[', urlResp.getcode(), "] : ", urlResp.geturl(), "\n", urlResp.info()
        jsonDict = dict()
        try:
            jsonDict = json.loads(urlResp.read())#.decode(urlResp.info().get_param('charset') or 'utf-8'))
            if type(jsonDict) is not dict:
                sys.exit(0)
        except:
            traceback.print_exc()
        print "POST jsonDict =", jsonDict
        mediaId = self.dict_get(jsonDict, 'media_id')
        if self.dict_get(jsonDict, 'type') == "image" and mediaId:
            return mediaId
        else:
            return None
