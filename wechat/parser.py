# -*- coding: utf-8 -*-
# filename: parser.py
import os
import media

def parse_cmd(cmd):
    if len(cmd) == 0:
        return None
    if cmd == '咔':
        name = "material.jpg"
        root = os.path.dirname(__file__)
        return {u'mediaId': media.upload_image(os.path.join(root, name))}
    else:
        return {'text': cmd}
