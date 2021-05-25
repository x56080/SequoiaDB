'''
Description   : seqDB-24073:创建/使用/获取/删除用户
Author        : XiaoNi Huang
CreateTime    : 2021.03.31
LastEditTime  : 2021.05.25
LastEditors   : XiaoNi Huang
'''
#!/usr/bin/python3.5
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
      self.clName = 'pymongo_24073'
      # 创建client，获取cl对象
      self.db = self.client[ self.dbName ]
      self.cl = self.db[ self.clName ]
      # 清理环境
      self.cl.drop()
      # 前置条件
      self.result = self.db.command( { "usersInfo": 1 } )
      self.assertEqual( self.result, {'users': [], 'ok': 1.0} )

   def test_user_auth_1( self ):
      # test: db.command("createUser" / "usersInfo" / "dropUser",....)
      try:
         # createUser
         self.result = self.db.command("createUser", self.userName, pwd=self.userName, roles=["dbAdmin"])
         self.assertEqual( self.result, {'ok': 1} )

         # usersInfo
         self.result = self.db.command( { "usersInfo": 1 } )
         if ( {'user': self.userName} in self.result['users'] ):
            pass
         else:
            raise BaseException( 'not exist user ' + str(self.userName) + ', result = ' + str( self.result) )
         
         # no auth, business operation
         self.cl.insert_one( {'a': 1} )
         # connect to mongodb engine and do business operationn without authentication, not bug 
         self.assertEqual( self.cl.count_documents( {} ), 1 )
         
         # authenticate
         self.result = self.db.authenticate( self.userName, self.userName )
         self.assertEqual( self.result, True )
         # business operation
         self.cl.insert_one( {'a': 1} )
         self.assertEqual( self.cl.count_documents( {} ), 2 )
         
         # dropUser
         self.result = self.db.command( "dropUser", self.userName )
         self.assertEqual( self.result, {'ok': 1.0} )
         # business operation
         self.cl.insert_one( {'a': 2} )
         self.assertEqual( self.cl.count_documents( {} ), 3 )
         # check users
         self.result = self.db.command( { "usersInfo": 1 } )
         if ( {'user': self.userName} in self.result['users'] ):
            raise "failed to drop user"

      finally:
         try:
            self.result = self.db.authenticate( self.userName, self.userName )
            self.db.command( "dropUser", self.userName )
         except:
            self.result = self.db.command( { "usersInfo": 1 } )
            self.assertEqual( self.result, {'users': [], 'ok': 1.0}, self.result )
         
   ''' SEQUOIADBMAINSTREAM-7207
   def test_user_auth_2( self ):
      # test: db.add_user / db.remove_user
      try:
         # createUser
         self.result = self.db.add_user(self.userName, pwd=self.userName, roles=["dbAdmin"])
         self.assertEqual( self.result, None )

         # usersInfo
         self.result = self.db.command( { "usersInfo": 1 } )
         if ( {'user': self.userName} in self.result['users'] ):
            pass
         else:
            raise BaseException( 'not exist user ' + str(self.userName) + ', result = ' + str( self.result) )
         
         # authenticate
         self.result = self.db.authenticate( self.userName, self.userName )
         self.assertEqual( self.result, True )
         # business operation
         self.cl.insert_one( {'a': 1} )
         self.assertEqual( self.cl.count_documents( {} ), 1 )
         
         # dropUser
         self.result = self.db.remove_user( self.userName )
         self.assertEqual( self.result, None )
      finally:
         try:
            self.result = self.db.authenticate( self.userName, self.userName )
            self.db.remove_user( self.userName )
         except:
            self.result = self.db.command( { "usersInfo": 1 } )
            self.assertEqual( self.result, {'users': [], 'ok': 1.0}, self.result )
      '''