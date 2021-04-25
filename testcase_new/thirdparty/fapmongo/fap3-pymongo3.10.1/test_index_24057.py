'''
Description   : seqDB-24057:创建/列取索引
Author        : XiaoNi Huang
CreateTime    : 2021.03.31
LastEditTime  : 2021.04.25
LastEditors   : XiaoNi Huang
'''
#!/usr/bin/python3.5
import bson
import pymongo
from pymongo import MongoClient
import utils
import unittest

class TestIndex24057( utils.TestBase ):   
   def setUp( self ):  
      # 初始化
      self.dbName = self.COMMDBNAME
      self.clName = 'pymongo_24057'
      # 创建client，获取cl对象
      self.db = self.client[ self.dbName ]
      self.cl = self.db[ self.clName ]
      self.idxName1 = 'idx1'
      self.idxName2 = 'idx2'
      self.idxName3 = 'idx3'
      # 清理环境
      self.cl.drop()
      # 准备数据      
      self.cl.insert_many( [
         { "_id": 1, "a": 1, "b": 1, "c": 1, "d": 1 },
         { "_id": 2, "a": 2, "b": 2, "c": 2, "d": 2 },
         { "_id": 3, "a": 2, "b": 3, "c": 3, "d": 2 }] )
      
   def test_index( self ):
      self.create_drop_list()
      # self.create_drop_indexes()
      self.unique_index()
      # self.create_index_IndexModel
   
   def tearDown( self ):
      self.cl.drop() 
      
   def create_drop_list( self ):      
      # create normal index, not set name
      self.cl.create_index( self.idxName1 );
      self.cl.create_index( [ ( 'a', 1 ) ], name = self.idxName2 );
      self.cl.create_index( [ ( 'b', -1 ) ], name = self.idxName3 );

      # index_information
      self.result = self.cl.index_information()
      self.assertRegex( str( self.result ), self.idxName1 + '_1' )   
      self.assertRegex( str( self.result ), self.idxName2 )   
      self.assertRegex( str( self.result ), self.idxName3 )   
      self.assertRegex( str( self.result ), '\$id' ) 

      # list_indexes
      self.indexes = []
      for self.idx in self.cl.list_indexes():
         self.indexes.append( self.idx )
      self.assertEqual( len( self.indexes ), 4 )

      # drop_index
      # drop normal index
      self.cl.drop_index( 'idx1_1' )
      self.indexes = []
      for self.idx in self.cl.list_indexes():
         self.indexes.append( self.idx )
      self.assertEqual( len( self.indexes ), 3 )

      # drop $id index (mongodb is _id_ index)
      try:
         self.cl.drop_index( '$id' )
      except pymongo.errors.OperationFailure as e:
         pass
      except:
         raise
         
      self.indexes = []
      for self.idx in self.cl.list_indexes():
         self.indexes.append( self.idx )
      self.assertEqual( len( self.indexes ), 3 )

   def create_drop_indexes( self ): 
      ''' jira-6970
      # create_indexes
      print('\n[---create_indexes---]')
      cl.drop_index( 'idx2' )
      cl.drop_index( 'idx3' )

      index1 = IndexModel( [ ( "name", DESCENDING ) ], name = "idx1" ) 
      index2 = IndexModel( [ ( "age", ASCENDING ) ], name = "idx2" ) 
      cl.create_indexes( [ index1, index2 ] ) 
      '''
      
      ''' jira-6979
      # drop_indexes
      print('\n[---drop_indexes---]')
      cl.drop_indexes()

      indexes = []
      for index in cl.list_indexes():
         indexes.append( index )
      print( indexes )
      assert len( indexes ) == 1
      '''

   def unique_index( self ):
      # create_indexes
      self.cl.drop_index( 'idx2' )
      self.cl.drop_index( 'idx3' )

      # exist unique index, insert duplicate key
      self.cl.create_index( [( 'b', 1 )], unique = True, backgroud = True )
      try:
         self.cl.insert_one({'b': 1})
      except pymongo.errors.OperationFailure as e:
         pass
      except:
         raise
         
      # exist duplicate key, create unique index
      try:
         self.cl.create_index( [( 'a', 1 )], unique = True, backgroud = True )
      except pymongo.errors.OperationFailure as e:
         pass
      except:
         raise

   def create_index_IndexModel( self ):
      '''
      cl.create_index( [ IndexModel( [ ( "a", ASCENDING ) ], name = "idx2" )  ] );
      cl.create_index( [ IndexModel( [ ( "a", DESCENDING ) ], name = "idx3" )  ] );
      '''