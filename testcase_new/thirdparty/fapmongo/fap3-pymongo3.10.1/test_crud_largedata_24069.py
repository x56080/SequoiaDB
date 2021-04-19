'''
Description   : seqDB-24069:增删改查大量数据
Author        : XiaoNi Huang
CreateTime    : 2021.03.31
LastEditTime  : 2021.04.19
LastEditors   : XiaoNi Huang
'''
''' jira-7023
#!/usr/bin/python3.5
import bson
import pymongo
from pymongo import MongoClient
import utils
import unittest

class TestCrudLargeData24069( utils.TestBase ):   
   def setUp( self ):  
      # 初始化
      self.dbName = self.COMMDBNAME
      self.clName = 'pymongo_24069'
      # 创建client，获取cl对象
      self.db = self.client[ self.dbName ]
      self.cl = self.db[ self.clName ]
      # 清理环境
      self.cl.drop()
      
   def test_crud( self ):
      # 准备数据
      self.docsNum = 2010
      self.insert_docs = []
      self.tmpNum = 0
      while self.tmpNum < self.docsNum:
         self.insert_docs.append( { "_id": self.tmpNum, "a": self.tmpNum, "b": 1 } )
         self.tmpNum += 1
      self.cl.insert_many( self.insert_docs )
      
      # find
      self.expDocsNum = 1001
      self.cursor = self.cl.find().limit( self.expDocsNum ).sort( '_id' )
      self.checkCursor( self.cursor, self.insert_docs[:self.expDocsNum] )
             
      # update
      self.expDocsNum = 1001
      self.expDocs = []
      self.tmpNum = 0
      while self.tmpNum < self.expDocsNum:
         self.expDocs.append( { "_id": self.tmpNum, "a": self.tmpNum, "b": 2 } )
         self.tmpNum += 1
         
      self.result = self.cl.update_many( { "a": { "$lt": self.expDocsNum } }, 
         { "$inc": { "b": 1 } } )
      self.assertEqual( self.result.matched_count, self.expDocsNum )  
      
      self.cursor = self.cl.find().skip( self.expDocsNum ).sort( '_id' )
      self.checkCursor( self.cursor, self.insert_docs[self.expDocsNum:] )

      # delete
      self.expDocsNum = 1001
      self.result = self.cl.delete_many( { 'a': {'$lt': self.expDocsNum } } )
      self.assertEqual( self.result.deleted_count, self.expDocsNum )
      self.assertEqual( self.cl.count_documents( {} ), self.docsNum - self.expDocsNum )  
   
   def tearDown( self ):
      self.cl.drop() 
   '''