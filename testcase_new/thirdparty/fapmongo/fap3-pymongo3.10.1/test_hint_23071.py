'''
Description   : seqDB-24071:hint走索引
Author        : XiaoNi Huang
CreateTime    : 2021.03.31
LastEditTime  : 2021.04.13
LastEditors   : XiaoNi Huang
'''
#!/usr/bin/python3.5
import bson
import pymongo
from pymongo import MongoClient
import utils
import unittest

class TestHint23071( utils.TestBase ):   
   def setUp( self ):  
      # 初始化
      self.dbName = self.COMMDBNAME
      self.clName = 'pymongo_23071'
      # 创建client，获取cl对象
      self.db = self.client[ self.dbName ]
      self.cl = self.db[ self.clName ]
      # 清理环境
      self.cl.drop()
      # 准备数据      
      self.cl.insert_many( [
         { "_id": 1, "a": 1, "b": 1 },
         { "_id": 2, "a": 4, "b": 2 },
         { "_id": 3, "a": 3, "b": 3 },
         { "_id": 4, "a": 2, "b": 4 }] )
      
   def test_hint( self ):
      # create index
      self.cl.create_index( [ ( 'a', 1 ) ], name = 'idx' )

      # find.hint by key
      self.cursor = self.cl.find( { "b": 1 } ).hint( [('a', 1)] )
      self.expDocs = [{'_id': 1, 'a': 1, 'b': 1}]
      self.checkCursor( self.cursor, self.expDocs )
      
      # find.hint by index name
      self.cursor = self.cl.find( { "b": 1 } ).hint( "idx" )
      self.expDocs = [{'_id': 1, 'a': 1, 'b': 1}]
      self.checkCursor( self.cursor, self.expDocs )
   
   def tearDown( self ):
      self.cl.drop() 