# -- coding: utf-8 --
'''
Description   : pymongo公共配置文件
Author        : XiaoNi Huang
CreateTime    : 2021.03.31
LastEditTime  : 2021.04.13
LastEditors   : XiaoNi Huang
'''
#!/usr/bin/python3.5
import json
import os
import os.path

class Config( object ):
   def __init__( self ):
      self.get_config()

   def get_config( self ):
      config_json = os.path.join( os.getcwd(), "config.json" )
      fp = open( config_json )
      configs = json.load( fp )
      # FAP主机名，默认"localhost"
      self.host = configs['FAPHOST']
      # FAP端口号，默认11810
      self.port = configs['FAPPORT']
      # 公共CS前缀，默认"local_host"
      self.db_prefix = configs['CHANGEDPREFIX']
      #用例存放临时文件的目录
      self.workdir = configs['WORKDIR']
      
      fp.close()

config = Config()
