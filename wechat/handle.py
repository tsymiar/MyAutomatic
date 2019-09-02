# -*- coding: utf-8 -*-
# filename: handle.py

import hashlib
import reply
import receive
import web
import traceback
import parser

class Handle(object):
    def GET(self):
        try:
            data = web.input()
            print data
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
            print "handle/GET func: hashcode, signature: ", hashcode, signature
            if hashcode == signature:
                return echostr
            else:
                return ""
        except Exception, Argument:
            return Argument
    def POST(self):
        try:
            webData = web.data()
            print "Handle Post webdata:\n", webData
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
                        traceback.print_exc()
                    try:
                        media = content['mediaId']
                    except:
                        traceback.print_exc()
                    print "content: text =", text, ", media =", media
                    if text:
                        replyMsg = reply.TextMsg(toUser, fromUser, text)
                    elif media:
                        try:
                            replyMsg = reply.ImageMsg(toUser, fromUser, media)
                        except:
                            traceback.print_exc()
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
                print "暂且不处理"
                return "success"
        except Exception, Argment:
            return Argment
