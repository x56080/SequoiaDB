'''
Description   : seqDB-24073:创建/使用/获取/删除用户
Author        : XiaoNi Huang
CreateTime    : 2021.03.31
LastEditTime  : 2021.04.01
LastEditors   : XiaoNi Huang
'''
import bson
import pymongo
from pymongo import MongoClient
import utils
import unittest

class TestAuth24073( utils.TestBase ):   
   def setUp( self ):  
      # 初始化
      self.userName = 'pymongo_24073'
      self.dbName = self.COMMDBNAME
      self.clName = 'pymongo_23066'
      # 创建client，获取cl对象
      self.db = self.client[ self.dbName ]
      self.cl = self.db[ self.clName ]
      # 清理环境
      self.cl.drop()
      # 前置条件
      self.result = self.db.command( { "usersInfo": 1 } )
      self.assertEqual( self.result, {'users': [], 'ok': 1.0} )
         
   def test_user_auth( self ):
      self.result = self.db.command("createUser","admin", pwd="admin",roles=["read"])
      self.assertEqual( self.result, {'ok': 1} )      
      
      self.result = self.db.command( { "usersInfo": 1 } )
      self.assertEqual( self.result, {'users': [], 'ok': 1.0} )
   

      ''' jira-6980
      # createUser
      result = db.command("createUser","admin",pwd="admin",roles=["read"])
      print( result )
      assert result == []

      # usersInfo
      result = db.command( { "usersInfo": 1 } )
      print( result )
      if userName in result['users']:
         print( 'ok' )
      except:
         raise 'Failed to check usersInfo'
         
      # authenticate
      result = client.test.authenticate('admin',"admin")
      assert result == True

      # dropUser
      result = db.command("dropUser","admin")
      print( result )
      assert result == {'users': [], 'ok': 1.0}
      '''