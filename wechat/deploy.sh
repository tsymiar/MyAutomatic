#!/bin/bash
if ! command -v virtualenv &> /dev/null; then
    echo "virtualenv not found, installing it now..."
    pip install virtualenv #--break-system-packages
    if [ $? -ne 0 ]; then
        echo "Failed to install virtualenv, please check your pip configuration."
        exit 1
    fi
fi

PYC=python
virtualenv -p $(which python2) venv
source venv/bin/activate
if [ $? -ne 0 ]; then
    echo "Activate venv failed, please check your python2 environment."
    PYC=$(which python2)
fi

if [ "$WECHAT_APPID" == "" ]; then
    echo "Please set WECHAT_APPID:"
    read wechat_appid
    if [ -z "$wechat_appid" ]; then
        echo "No input provided. Setting WECHAT_APPID to default value..."
        exit 1
    else
        export WECHAT_APPID="$wechat_appid"
    fi
fi
if [ "$WECHAT_APPSECRET" == "" ]; then
    echo "Please set WECHAT_APPSECRET:"
    read wechat_appsecret
    if [ -z "$wechat_appsecret" ]; then
        echo "No input provided. Setting WECHAT_APPSECRET to default value..."
        exit 1
    else
        export WECHAT_APPSECRET="$wechat_appsecret"
    fi
fi
if [ "$WECHAT_TOKEN" == "" ]; then
    echo "Please set WECHAT_TOKEN:"
    read wechat_token
    if [ -z "$wechat_token" ]; then
        echo "No input provided. Setting WECHAT_TOKEN to default value..."
        exit 1
    else
        export WECHAT_TOKEN="$wechat_token"
    fi
fi

if [ -f "requirements.txt" ]; then pip install -r requirements.txt; fi;
$PYC main.py
