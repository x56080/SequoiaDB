'''
Description   : seqDB-24065:聚集操作
Author        : XiaoNi Huang
CreateTime    : 2021.03.31
LastEditTime  : 2021.04.13
'''
#!/usr/bin/python3.5
import bson
import pymongo
from pymongo import MongoClient
import utils
import unittest

class TestAggregate24065( utils.TestBase ):   
   def setUp( self ):  
      # 初始化
      self.dbName = self.COMMDBNAME
      self.clName = 'pymongo_24065'
      # 创建client，获取cl对象
      self.db = self.client[ self.dbName ]
      self.cl = self.db[ self.clName ]
      # 清理环境
      self.cl.drop()
      # 准备数据
      self.cl.insert_many( [{ "_id": 1, "a": "A", "b": 86, "c": "test", "d": 1 },
         { "_id": 2, "a": "B", "b": 90, "c": "dev", "d": 1 },
         { "_id": 3, "a": "C", "b": 100, "c": "test", "d": 1 },
         { "_id": 4, "a": "D", "b": 79, "c": "dev", "d": 1 }] )
         
   def test_aggregate( self ):
      self.aggregate_progect_01()
   
   def tearDown( self ):
      self.cl.drop()       

   def aggregate_progect_01( self ):
      # $project，单个字段
      # test1: a:1 
      self.pipeline = [ { "$project": { "a": 1 } } ]
      self.expectDocs = [{'_id': 1, 'a': 'A'}, {'_id': 2, 'a': 'B'}, 
         {'_id': 3, 'a': 'C'}, {'_id': 4, 'a': 'D'}]
      self.aggregate_check( self.pipeline, self.expectDocs )
      
      # test2: a:0
      self.pipeline = [ { "$project": { "a": 0 } } ]
      try:
         self.cl.aggregate( self.pipeline )
      except pymongo.errors.OperationFailure as e:
         pass
      except:
         raise
      
      # test3: _id:1
      self.pipeline = [ { "$project": { "_id": 1 } } ]
      self.expectDocs = [{'_id': 1}, {'_id': 2}, {'_id': 3}, {'_id': 4}]
      self.aggregate_check( self.pipeline, self.expectDocs )
         
      # test3: _id:0
      self.pipeline = [ { "$project": { "_id": 0 } } ]
      try:
         self.cl.aggregate( self.pipeline )
      except pymongo.errors.OperationFailure as e:
         pass
      except:
         raise      

   def aggregate_progect_02( self ):
      # $project，多个字段
      # test1: a:1,b:1
      self.pipeline = [ { "$project": { "a": 1, "b": 1 } } ]
      self.expectDocs = [{'b': 86, 'a': 'A', '_id': 1}, {'b': 90, 'a': 'B', '_id': 2}, {'b': 100, 'a': 'C', '_id': 3}, {'b': 79, 'a': 'D', '_id': 4}]
      self.aggregate_check( self.pipeline, self.expectDocs )
      
      # test2: a:1,b:0
      self.pipeline = [ { "$project": { "a": 1, "b": 0 } } ]
      self.expectDocs = [{'a': 'A', '_id': 1}, {'a': 'B', '_id': 2}, {'a': 'C', '_id': 3}, {'a': 'D', '_id': 4}]
      self.aggregate_check( self.pipeline, self.expectDocs )
      
      # test3: _id:1,b:1
      self.pipeline = [ { "$project": { "_id": 1, "b": 1 } } ]
      self.expectDocs = [{'b': 86, '_id': 1}, {'b': 90, '_id': 2}, {'b': 100, '_id': 3}, {'b': 79, '_id': 4}]
      self.aggregate_check( self.pipeline, self.expectDocs )
      
      # test4: _id:0,b:1
      self.pipeline = [ { "$project": { "_id": 0, "b": 1 } } ]
      self.expectDocs = [{'b': 86}, {'b': 90}, {'b': 100}, {'b': 79}]
      self.aggregate_check( self.pipeline, self.expectDocs )
      
      # test5: _id:0,b:1,c:0
      self.pipeline = [ { "$project": { "_id": 0, "b": 1, "c": 0 } } ]
      self.expectDocs = [{'b': 86}, {'b': 90}, {'b': 100}, {'b': 79}]
      self.aggregate_check( self.pipeline, self.expectDocs )
      
      # test5: _id:0,b:1,c:1
      self.pipeline = [ { "$project": { "_id": 0, "b": 1, "c": 1 } } ]
      self.expectDocs = [{'c': 'test', 'b': 86}, {'c': 'dev', 'b': 90}, {'c': 'test', 'b': 100}, {'c': 'dev', 'b': 79}]
      self.aggregate_check( self.pipeline, self.expectDocs )
      
      # test6: _id:0,b:0
      self.pipeline = [ { "$project": { "_id": 0, "a": 0 } } ]      
      try:
         self.cl.aggregate( self.pipeline )
      except pymongo.errors.OperationFailure as e:
         pass
      except:
         raise

   def aggregate_match_group_sort_limit( self ):
      # test1: $match + $group + $sort(1) + $limit
      self.pipeline = [ { "$match": { "b": { "$gt": 80 } } }, { "$group": { "_id": "$b", "total": { "$sum": "$b" } } }, { "$sort": { "total": 1 } }, { "$limit": 2 } ]
      self.expectDocs = [{'total': 86.0, '_id': 86}, {'total': 90.0, '_id': 90}]
      self.aggregate_check( self.pipeline, self.expectDocs )
         
      # test2: $match + $group + $sort(-1) + $skip
      self.pipeline = [ { "$match": { "b": { "$gt": 80 } } }, { "$group": { "_id": "$b", "total": { "$sum": "$b" } } }, { "$sort": { "total": -1 } }, { "$skip": 1 } ]
      self.expectDocs = [{'total': 90.0, '_id': 90}, {'total': 86.0, '_id': 86}]
      self.aggregate_check( self.pipeline, self.expectDocs )

   def aggregate_limit( self ):
      # test1: limit = 0
      self.pipeline = [ { "$limit": 0 } ]
      self.expectDocs = []
      self.aggregate_check( self.pipeline, self.expectDocs )
      
      # test2: 0 < limit < 总记录数
      self.pipeline = [ { "$limit": 2 } ]
      self.expectDocs = []
      self.aggregate_check( self.pipeline, self.expectDocs )
      
      # test3: limit = 总记录数
      self.pipeline = [ { "$limit": len( insert_docs ) }, { "$sort": { "b": 1 } } ]
      self.expectDocs = [{'d': 1, 'a': 'D', '_id': 4, 'c': 'dev', 'b': 79}, {'d': 1, 'a': 'A', '_id': 1, 'c': 'test', 'b': 86}, {'d': 1, 'a': 'B', '_id': 2, 'c': 'dev', 'b': 90}, {'d': 1, 'a': 'C', '_id': 3, 'c': 'test', 'b': 100}]
      self.aggregate_check( self.pipeline, self.expectDocs )

   def aggregate_skip( self ):
      # test1: skip = 0
      self.pipeline = [ { "$skip": 0 }, { "$sort": { "b": 1 } } ]
      self.expectDocs = [{'d': 1, 'a': 'D', '_id': 4, 'c': 'dev', 'b': 79}, {'d': 1, 'a': 'A', '_id': 1, 'c': 'test', 'b': 86}, {'d': 1, 'a': 'B', '_id': 2, 'c': 'dev', 'b': 90}, {'d': 1, 'a': 'C', '_id': 3, 'c': 'test', 'b': 100}]
      self.aggregate_check( self.pipeline, self.expectDocs )
      
      # test2: 0 < skip < 总记录数
      self.pipeline = [ { "$skip": 3 }, { "$sort": { "b": 1 } } ]
      self.expectDocs = []
      self.aggregate_check( self.pipeline, self.expectDocs )
      
      # test3: skip = 总记录数
      self.pipeline = [ { "$skip": len( insert_docs ) }, { "$sort": { "b": 1 } } ]
      self.expectDocs = []
      self.aggregate_check( self.pipeline, self.expectDocs )

   def aggregate_sort( self ):
      # 单个字段排序在其他匹配符已覆盖
      # 多个字段排序
      self.pipeline = [ { "$sort": { "a": -1, "c": -1 } }, { "$limit": 2 } ]
      self.expectDocs = []
      self.aggregate_check( self.pipeline, self.expectDocs )      
      
      # 字段不存在
      self.pipeline = [ { "$sort": { "notEixst": -1 } }, { "$limit": 2 } ]
      self.expectDocs = [{'a': 'A', 'd': 1, '_id': 1, 'c': 'test', 'b': 86}, {'a': 'B', 'd': 1, '_id': 2, 'c': 'dev', 'b': 90}]
      self.aggregate_check( self.pipeline, self.expectDocs )

   def aggregate_group( self ):
      # test1: $first / $last      
      ''' EQUOIADBMAINSTREAM-5656
      self.pipeline = [{ "$group": { "_id": "$c", "first_b": { "$first": "$b" }, "max_b": { "$max": "$b" }, "last_b": { "$last": "$b" } } }]
      self.expectDocs = [{"_id":"dev","first_b":79,"max_b":90,"last_b":90},{"_id":"test","first_b":86,"max_b":100,"last_b":100}]
      self.aggregate_check( self.pipeline, self.expectDocs )
      '''
      
      # test2: $max / $min
      self.pipeline = [{ "$group": { "_id": "$c", "max_b": { "$max": "$b" }, "min_b": { "$min": "$b" }, "push_d": { "$push": "$d" } } }, { "$sort": { "_id": 1 } }]
      self.expectDocs = [{'push_d': [1, 1], '_id': 'dev', 'max_b': 90, 'min_b': 79}, {'push_d': [1, 1], '_id': 'test', 'max_b': 100, 'min_b': 86}]
      self.aggregate_check( self.pipeline, self.expectDocs )

      # test3: $avg, $addToSet
      self.pipeline = [{ "$group": { "_id": "$c", "avg_b": { "$avg": "$b" }, "addtoset_d": { "$addToSet": "$d" } } }, { "$sort": { "_id": 1 } }]
      self.expectDocs = [{'avg_b': 84.5, 'addtoset_d': [1], '_id': 'dev'}, {'avg_b': 93.0, 'addtoset_d': [1], '_id': 'test'}]
      self.aggregate_check( self.pipeline, self.expectDocs )
      
      # test4: $sum
      self.pipeline = [{ "$group": { "_id": "$c", "sum_b": { "$sum": "$b" } } }, { "$sort": { "_id": 1 } }]
      self.expectDocs = [{'sum_b': 169.0, '_id': 'dev'}, {'sum_b': 186.0, '_id': 'test'}]
      self.aggregate_check( self.pipeline, self.expectDocs )

   def aggregate_invalid_arg( self ):
      try:
         self.cl.aggregate( [] )
      except pymongo.errors.OperationFailure as e:
         pass
      except:
         raise
         
      try:
         self.cl.aggregate( "" )
      except TypeError as e:
         pass
      except:
         raise
         
      try:
         self.cl.aggregate( {} )
      except TypeError as e:
         pass
      except:
         raise
         
      try:
         self.cl.aggregate()
      except TypeError as e:
         pass
      except:
         raise
      
   def aggregate_check( self, pipeline, expectDocs ):
      self.docs = []
      for self.doc in self.cl.aggregate( self.pipeline ):
          self.docs.append( self.doc )
      self.assertEqual( self.docs, self.expectDocs )