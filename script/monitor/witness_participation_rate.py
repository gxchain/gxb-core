#!/usr/bin/env python
# -*- coding:utf-8 -*-

import json
import urllib2
import urllib

WITNESS_URL = "http://127.0.0.1:28090"
POST_DATA = '{"jsonrpc": "2.0", "method": "call", "params": [0, "get_witness_participation_rate", []], "id": 1}'

WECHAT_NOTIFY_URL = "http://172.19.19.49:8091/notify/wechat/send?message=%s&agent=GXCHAIN"
NO_RESULT_MSG = "见证人监控程序，无法获取参与率, 错误信息:"
WARNING_MSG = "见证人参与率预期为100%，当前参与率:"

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
    participation_rate = js['result']
    if 10000 <> participation_rate:
      error_msg = "%s %f %%" % (WARNING_MSG, float(participation_rate)/100)
  except Exception as e:
    error_msg = NO_RESULT_MSG + str(e)

  # send error_msg
  if len(error_msg) > 0:
    notify_url = WECHAT_NOTIFY_URL % (urllib.quote(error_msg))
    #print notify_url
    download(notify_url)

if __name__ == "__main__":
  main()
