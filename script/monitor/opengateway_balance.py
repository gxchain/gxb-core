#!/usr/bin/env python
# -*- coding:utf-8 -*-

import json
import urllib2
import urllib

WITNESS_URL = "http://127.0.0.1:28090"
POST_DATA = '{"jsonrpc": "2.0", "method": "call", "params": [0, "get_account_balances", ["1.2.26", ["1.3.0"]]], "id": 1}'

WECHAT_NOTIFY_URL = "http://172.19.19.49:8091/notify/wechat/send?message=%s&agent=EXCHANGE"
NO_RESULT_MSG = "opengateway帐户无法获取余额, 错误信息:"
WARNING_MSG = "opengateway帐余额不足1万, 当前余额:"

def download(url, data = None):
  opener = urllib2.build_opener()
  request = urllib2.Request(url, data)
  response = opener.open(request, timeout=60)
  return response.read()

def main():
  # get error_msg
  error_msg = ""
  try:
    res =  download(WITNESS_URL, POST_DATA)
    js = json.loads(res)
    balance = js['result'][0]['amount']
    if 1000000000 > int(balance):
      error_msg = "%s %f GXC, 请尽快操作" % (WARNING_MSG, float(balance)/100000)
  except Exception as e:
    error_msg = NO_RESULT_MSG + str(e)

  # send error_msg
  if len(error_msg) > 0:
    notify_url = WECHAT_NOTIFY_URL % (urllib.quote(error_msg))
    # print notify_url
    download(notify_url)

if __name__ == "__main__":
  main()
