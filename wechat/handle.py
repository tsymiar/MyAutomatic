# -*- coding: utf-8 -*-
# filename: handle.py

import hashlib
import reply
import receive
import web
import logging
import traceback
import parser

logging.basicConfig(level=logging.INFO,  
                    format='%(asctime)s %(filename)s[line:%(lineno)d] %(levelname)s %(message)s',  
                    datefmt='%a, %d %b %Y %H:%M:%S',  
                    filename='/tmp/wechat.log',  
                    filemode='w')
class Handle(object):
    def GET(self):
        try:
            data = web.input()
            if len(data) == 0:
                return "hello, handle wechat GET view"
            signature = data.signature
            timestamp = data.timestamp
            nonce = data.nonce
            echostr = data.echostr
            token = "15c86a1edac263a4"

            list = [token, timestamp, nonce]
            list.sort()
            sha1 = hashlib.sha1()
            map(sha1.update, list)
            hashcode = sha1.hexdigest()
            logging.info("handle/GET func: hashcode, signature: " + hashcode + signature)
            if hashcode == signature:
                return echostr
            else:
                return ""
        except Exception, Argument:
            return Argument
    def POST(self):
        try:
            webData = web.data()
            logging.info("Handle Post webdata:\n" + webData)
            recMsg = receive.parse_xml(webData)
            if isinstance(recMsg, receive.MainMsg):
                toUser = recMsg.FromUserName
                fromUser = recMsg.ToUserName
                if recMsg.MsgType == 'text':
                    content = {}
                    content = parser.parse_cmd(recMsg.Content)
                    text = ""
                    media = ""
                    try:
                        text = content['text']
                    except:
                        logging.warning(traceback.format_exc())
                    try:
                        media = content['mediaId']
                    except:
                        logging.warning(traceback.format_exc())
                    logging.info("content: text='" + text + "', media='" + media + "'.")
                    if text:
                        replyMsg = reply.TextMsg(toUser, fromUser, text)
                    elif media:
                        try:
                            replyMsg = reply.ImageMsg(toUser, fromUser, media)
                        except:
                            logging.warning(traceback.format_exc())
                    else:
                        replyMsg = reply.TextMsg(toUser, fromUser, "fail")
                    return replyMsg.send()
                if recMsg.MsgType == 'image':
                    mediaId = recMsg.MediaId
                    replyMsg = reply.ImageMsg(toUser, fromUser, mediaId)
                    return replyMsg.send()
                else:
                    return reply.MainMsg().send()
            else:
                logging.info("暂且不处理")
                return "success"
        except Exception, Argment:
            return Argment
