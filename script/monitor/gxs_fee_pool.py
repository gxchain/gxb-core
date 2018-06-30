#!/usr/bin/env python
# -*- coding:utf-8 -*-

import json
import urllib2
import urllib

WITNESS_URL = "http://127.0.0.1:28090"
POST_DATA = '{"jsonrpc": "2.0", "method": "call", "params": [0, "get_objects", [["2.3.1"]]], "id": 1}'

GXS_THRESHOLD = 5000000000
WECHAT_NOTIFY_URL = "http://172.19.19.49:8091/notify/wechat/send?message=%s&agent=GXCHAIN"
NO_RESULT_MSG = "GXS手续费资金池监控，无法获取手续费资金池余额, 错误信息:"
WARNING_MSG = "GXS手续费资金池余额不足5万, 当前余额:"

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
    fee_pool_balance = js['result'][0]['fee_pool']
    if GXS_THRESHOLD > int(fee_pool_balance):
      error_msg = "%s %f GXC, 请尽快操作" % (WARNING_MSG, float(fee_pool_balance)/100000)
  except Exception as e:
    error_msg = NO_RESULT_MSG + str(e)

  # send error_msg
  if len(error_msg) > 0:
    notify_url = WECHAT_NOTIFY_URL % (urllib.quote(error_msg))
    #print notify_url
    download(notify_url)

if __name__ == "__main__":
  main()
