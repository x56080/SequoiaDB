/*
 * @description 兼容mongodb，mongoose驱动测试
      注意事项：
      1、mongoose中test执行存在特殊性，方法都是后天执行，即：执行方法后不会等返回，方法里面的方法也是一样的，如test中同一个层级的方法。如果B方法依赖A方法的执行结果，则需要将B方法写在A方法里面。
      2、不支持创建/删除database、collection，跑用例前后需要手工干预清理环境，否则可能会受数据残留影响导致用例执行失败
 * @testcase sequoiaDB / Story测试 / 兼容mongodb / 驱动测试 / mongoose驱动
 * @author XiaoNi Huang 2020-07-03
 * @modify XiaoNi Huang 2023-08-20
*/
const Mongoose = require( 'mongoose' );
const Assert = require( 'assert' );

const dbName = 'cs_mongoose_model';
const SequoiaDB_URL = 'mongodb://localhost:11817/' + dbName;
const Schema = Mongoose.Schema( { "_id": Number, "a": Number, "b": Number } );
const modelName = "comm_model";

main();
function main ()
{
   Mongoose.connect( SequoiaDB_URL );

   // seqDB-22461:创建集合
   test_createCollection_22461();
   test_autoCreateCSCL_22461();

   // seqDB-22462:创建/列取索引
   test_createIndexes_22462();

   // seqDB-22463:插入记录
   test_create_22463();
   test_insertMany_22463();
   test_prototype_save_22463();

   // seqDB-22464:更新数据
   test_update_22464();
   test_updateMany_22464();

   // seqDB-22465:删除记录
   test_deleteOne_22465();
   test_deleteMany_22465();
   test_remove_22465();
   test_prototype_delete_22465();
   test_prototype_remove_22465();

   // seqDB-22466:匹配符测试
   test_matcher_22466();

   // seqDB-22467:选择符测试
   test_selector_22467();

   // seqDB-22468:更新符测试
   test_updater_object_22468();
   test_updater_array_22468();

   // seqDB-22469:聚集操作
   test_aggregate_22469();

   // seqDB-22470:数据统计
   test_count_22470();

   // seqDB-22471:去重
   test_distinct_22471();

   // seqDB-22472:自增字段
   test_prototype_increment_22472();

   // seqDB-22505:增删改查大量数据
   test_crud_largeData_22505();

   // test_bulkWrite();  --暂不支持
}

/*
* mongoose没有删除集合的接口，该测试点测试前需要删除已存在的集合
* 存在集合再创建也不会失败
*/
function test_createCollection_22461 ()
{
   const clName = 'cl_createCollection_22461';
   const Test = Mongoose.model( modelName, Schema, clName );
   Test.createCollection().then(
      function()
      {
         // check results
         Test.count( {},
            function( err, rc )
            {
               if( err )
               {
                  throw new Error( err );
               }
               else
               {
                  Assert.equal( rc, 0 );
               }
               console.log( "---test_createCollection success" );
            }
         );
      }
   );
}

function test_autoCreateCSCL_22461 ()
{
   const clName = 'cl_autoCreateCSCL_22461';
   const Test = Mongoose.model( modelName, Schema, clName );
   Test.insertMany( [{ '_id': 1, 'a': 1 }, { '_id': 2, 'a': 2 }],
      function( err )
      {
         if( err )
         {
            throw new Error( err );
         }
         else
         {
            // check results
            // 集合不存在会报错
            Test.remove( {},
               function( err )
               {
                  if( err )
                  {
                     throw new Error( err );
                  }
                  console.log( "---test_autoCreateCSCL success" );
               }
            );
         }
      }
   );
}

/*
* 没有删除索引的接口，创建索引前需要删除索引或者集合
*/
function test_createIndexes_22462 ()
{
   const clName = 'cl_createIndexes_22462';

   // Schema指定索引属性创建索引
   const Schema2 = Mongoose.Schema( {
      "_id": Number,
      "a": { "type": String, "unique": true },
      "b": { "type": String, "unique": false }
   } );
   const Test = Mongoose.model( 'test_createIndexes_22462', Schema2, clName );
   // 没有创建/删除集合的接口，需要插入记录时自动创建，否则下一步remove清理数据时会报集合不存在
   Test.create( [{ "_id": -1 }],
      function( err )
      {
         if( err )
         {
            throw new Error( err );
         }
         else
         {
            // clean, then test                     
            Test.remove( {},
               function( err )
               {
                  if( err )
                  {
                     throw new Error( err );
                  }
                  else
                  {
                     // createIndexes
                     Test.createIndexes( {},
                        function( err )
                        {
                           if( err )
                           {
                              throw new Error( err );
                           }
                           else
                           {
                              // listIndexes                              
                              Test.listIndexes(
                                 function( err, rc )
                                 {
                                    if( err )
                                    {
                                       throw new Error( err );
                                    }
                                    else
                                    {
                                       Assert.equal( objectKeysSort( rc ), objectKeysSort( [{ "key": { "_id": 1 }, "name": "$id", "ns": dbName + "." + clName, "unique": true, "v": 0 }, { "key": { "a": 1 }, "name": "a_1", "ns": dbName + "." + clName, "unique": true, "v": 0 }, { "key": { "b": 1 }, "name": "b_1", "ns": dbName + "." + clName, "v": 0 }] ) );
                                    }
                                 }
                              );

                              // crud
                              Test.create( [{ "_id": 1, "a": 1, "b": 1 }],
                                 function( err, rc )
                                 {
                                    if( err )
                                    {
                                       throw new Error( err );
                                    }
                                    else
                                    {
                                       Assert.equal( objectKeysSort( rc ), objectKeysSort( [{ "__v": 0, "_id": 1, "a": "1", "b": "1" }] ) );

                                       // insert, duplicate key
                                       Test.create( [{ "_id": 3, "a": 1 }],
                                          function( err )
                                          {
                                             if( err )
                                             {
                                                if( err.code !== -38 ) 
                                                {
                                                   throw new Error( err );
                                                }
                                             }
                                             else
                                             {
                                                Test.find( {},
                                                   function( err, rc )
                                                   {
                                                      if( err )
                                                      {
                                                         throw new Error( err );
                                                      }
                                                      else
                                                      {
                                                         Assert.equal( objectKeysSort( rc ), objectKeysSort( [{ "__v": 0, "_id": 1, "a": "1", "b": 1 }, { "__v": 0, "_id": 2, "a": "2", "b": 1 }] ) );
                                                      }
                                                      console.log( "---test_createIndexes success" );
                                                   }
                                                );
                                             }
                                          }
                                       );
                                    }
                                 }
                              );
                           }
                        }
                     );
                  }
               }
            );
         }
      }
   );
}

function test_create_22463 ()
{
   const clName = 'cl_create_22463';
   const Test = Mongoose.model( modelName, Schema, clName );
   // 没有创建/删除集合的接口，需要插入记录时自动创建，否则下一步remove清理数据时会报集合不存在
   Test.create( [{ "_id": -1 }],
      function( err )
      {
         if( err )
         {
            throw new Error( err );
         }
         else
         {
            // clean, then test  
            Test.remove( {},
               function( err )
               {
                  if( err )
                  {
                     throw new Error( err );
                  }
                  else
                  {
                     Test.create( { '_id': 1, 'a': 1 },
                        function( err, rc )
                        {
                           if( err )
                           {
                              throw new Error( err );
                           }
                           else
                           {
                              Assert.equal( objectKeysSort( rc ), objectKeysSort( { "_id": 1, "a": 1, "__v": 0 } ) );

                              // check results
                              Test.find( {},
                                 function( err, rc )
                                 {
                                    if( err )
                                    {
                                       throw new Error( err );
                                    }
                                    else
                                    {
                                       Assert.equal( objectKeysSort( rc ), objectKeysSort( [{ "_id": 1, "a": 1, "__v": 0 }] ) );
                                    }
                                    console.log( "---test_create success" );
                                 }
                              );
                           }
                        }
                     );
                  }
               }
            );
         }
      }
   );
}

function test_insertMany_22463 ()
{
   const clName = 'cl_insertMany_22463';
   const Test = Mongoose.model( modelName, Schema, clName );
   // 没有创建/删除集合的接口，需要插入记录时自动创建，否则下一步remove清理数据时会报集合不存在
   Test.create( [{ "_id": -1 }],
      function( err )
      {
         if( err )
         {
            throw new Error( err );
         }
         else
         {
            // clean, then test  
            Test.remove( {},
               function( err )
               {
                  if( err )
                  {
                     throw new Error( err );
                  }
                  else
                  {
                     Test.insertMany( [{ '_id': 1, 'a': 1 }, { '_id': 2, 'a': 2 }],
                        function( err, rc )
                        {
                           if( err )
                           {
                              throw new Error( err );
                           }
                           else
                           {
                              const expDocs = [{ "_id": 1, "a": 1, "__v": 0 }, { "_id": 2, "a": 2, "__v": 0 }];
                              Assert.equal( objectKeysSort( rc ), objectKeysSort( expDocs ) );

                              // check results
                              Test.find( {},
                                 function( err, rc )
                                 {
                                    if( err )
                                    {
                                       throw new Error( err );
                                    }
                                    else
                                    {
                                       Assert.equal( objectKeysSort( rc ), objectKeysSort( expDocs ) );
                                    }
                                    console.log( "---test_insertMany success" );
                                 }
                              );
                           }
                        }
                     );
                  }
               }
            );
         }
      }
   );
}

function test_prototype_save_22463 ()
{
   const clName = 'cl_prototype_save_22463';
   const Test = Mongoose.model( modelName, Schema, clName );
   // 没有创建/删除集合的接口，需要插入记录时自动创建，否则下一步remove清理数据时会报集合不存在
   Test.create( [{ "_id": -1 }],
      function( err )
      {
         if( err )
         {
            throw new Error( err );
         }
         else
         {
            // clean, then test  
            Test.remove( {},
               function( err )
               {
                  if( err )
                  {
                     throw new Error( err );
                  }
                  else
                  {
                     // save
                     const doc = new Test( { '_id': 1, 'a': 1 } );
                     doc.save(
                        function( err, rc )
                        {
                           if( err )
                           {
                              throw new Error( err );
                           }
                           else
                           {
                              Assert.equal( objectKeysSort( rc ), objectKeysSort( { "_id": 1, "a": 1, "__v": 0 } ) );

                              // check results
                              Test.find( {},
                                 function( err, rc )
                                 {
                                    if( err )
                                    {
                                       throw new Error( err );
                                    }
                                    else
                                    {
                                       Assert.equal( objectKeysSort( rc ), objectKeysSort( [{ "_id": 1, "a": 1, "__v": 0 }] ) );
                                    }
                                    console.log( "---test_prototype_save success" );
                                 }
                              );
                           }
                        }
                     );
                  }
               }
            );
         }
      }
   );
}

function test_update_22464 ()
{
   const clName = 'cl_update_22464';
   const Test = Mongoose.model( modelName, Schema, clName );
   // 没有创建/删除集合的接口，需要插入记录时自动创建，否则下一步remove清理数据时会报集合不存在
   Test.create( [{ "_id": -1 }],
      function( err )
      {
         if( err )
         {
            throw new Error( err );
         }
         else
         {
            // clean, then test  
            Test.remove( {},
               function( err )
               {
                  if( err )
                  {
                     throw new Error( err );
                  }
                  else
                  {
                     Test.insertMany( [{ '_id': 1, 'a': 1 }, { '_id': 2, 'a': 2 }],
                        function( err )
                        {
                           if( err )
                           {
                              throw new Error( err );
                           }
                           else
                           {
                              Test.update( { "$inc": { "a": 1 } },
                                 function( err, rc )
                                 {
                                    if( err )
                                    {
                                       throw new Error( err );
                                    }
                                    else
                                    {
                                       Assert.equal( objectKeysSort( rc ), objectKeysSort( { "n": 1, "nModified": 1, "ok": 1 } ) );

                                       // check results
                                       Test.find( {},
                                          function( err, rc )
                                          {
                                             if( err )
                                             {
                                                throw new Error( err );
                                             }
                                             else
                                             {
                                                Assert.equal( objectKeysSort( rc ), objectKeysSort( [{ "_id": 1, "a": 2, "__v": 0 }, { "_id": 2, "a": 2, "__v": 0 }] ) );
                                             }
                                             console.log( "---test_update success" );
                                          }
                                       );
                                    }
                                 }
                              );
                           }
                        }
                     );
                  }
               }
            );
         }
      }
   );
}

function test_updateMany_22464 ()
{
   const clName = 'cl_updateMany_22464';
   const Test = Mongoose.model( modelName, Schema, clName );
   // 没有创建/删除集合的接口，需要插入记录时自动创建，否则下一步remove清理数据时会报集合不存在
   Test.create( [{ "_id": -1 }],
      function( err )
      {
         if( err )
         {
            throw new Error( err );
         }
         else
         {
            // clean, then test  
            Test.remove( {},
               function( err )
               {
                  if( err )
                  {
                     throw new Error( err );
                  }
                  else
                  {
                     Test.insertMany( [{ '_id': 1, 'a': 1 }, { '_id': 2, 'a': 2 }],
                        function( err )
                        {
                           if( err )
                           {
                              throw new Error( err );
                           }
                           else
                           {
                              Test.updateMany( { "$inc": { "a": 1 }, "$set": { "b": 1 } },
                                 function( err, rc )
                                 {
                                    if( err )
                                    {
                                       throw new Error( err );
                                    }
                                    else
                                    {
                                       Assert.equal( objectKeysSort( rc ), objectKeysSort( { "n": 2, "nModified": 2, "ok": 1 } ) );

                                       // check results
                                       Test.find( {},
                                          function( err, rc )
                                          {
                                             if( err )
                                             {
                                                throw new Error( err );
                                             }
                                             else
                                             {
                                                Assert.equal( objectKeysSort( rc ), objectKeysSort( [{ "_id": 1, "a": 2, "__v": 0, "b": 1 }, { "_id": 2, "a": 3, "__v": 0, "b": 1 }] ) );
                                             }
                                             console.log( "---test_updateMany success" );
                                          }
                                       );
                                    }
                                 }
                              );
                           }
                        }
                     );
                  }
               }
            );
         }
      }
   );
}

function test_deleteOne_22465 ()
{
   const clName = 'cl_deleteOne_22465';
   const Test = Mongoose.model( modelName, Schema, clName );
   // 没有创建/删除集合的接口，需要插入记录时自动创建，否则下一步remove清理数据时会报集合不存在
   Test.create( [{ "_id": -1 }],
      function( err )
      {
         if( err )
         {
            throw new Error( err );
         }
         else
         {
            // clean, then test  
            Test.remove( {},
               function( err )
               {
                  if( err )
                  {
                     throw new Error( err );
                  }
                  else
                  {
                     Test.insertMany( [{ '_id': 1, 'a': 1 }, { '_id': 2, 'a': 1 }, { '_id': 3, 'a': 2 }],
                        function( err )
                        {
                           if( err )
                           {
                              throw new Error( err );
                           }
                           else
                           {
                              Test.deleteOne( { "a": 1 },
                                 function( err, rc )
                                 {
                                    if( err )
                                    {
                                       throw new Error( err );
                                    }
                                    else
                                    {
                                       Assert.equal( objectKeysSort( rc ), objectKeysSort( { "n": 1, "ok": 1, "deletedCount": 1 } ) );

                                       // check results
                                       Test.find( {},
                                          function( err, rc )
                                          {
                                             if( err )
                                             {
                                                throw new Error( err );
                                             }
                                             else
                                             {
                                                Assert.equal( objectKeysSort( rc ), objectKeysSort( [{ "_id": 2, "a": 1, "__v": 0 }, { "_id": 3, "a": 2, "__v": 0 }] ) );
                                             }
                                             console.log( "---test_deleteOne success" );
                                          }
                                       );
                                    }
                                 }
                              );
                           }
                        }
                     );
                  }
               }
            );
         }
      }
   );
}

function test_deleteMany_22465 ()
{
   const clName = 'cl_deleteMany_22465';
   const Test = Mongoose.model( modelName, Schema, clName );
   // 没有创建/删除集合的接口，需要插入记录时自动创建，否则下一步remove清理数据时会报集合不存在
   Test.create( [{ "_id": -1 }],
      function( err )
      {
         if( err )
         {
            throw new Error( err );
         }
         else
         {
            // clean, then test  
            Test.remove( {},
               function( err )
               {
                  if( err )
                  {
                     throw new Error( err );
                  }
                  else
                  {
                     Test.insertMany( [{ '_id': 1, 'a': 1 }, { '_id': 2, 'a': 1 }, { '_id': 3, 'a': 2 }],
                        function( err )
                        {
                           if( err )
                           {
                              throw new Error( err );
                           }
                           else
                           {
                              Test.deleteMany( { "a": 1 },
                                 function( err, rc )
                                 {
                                    if( err )
                                    {
                                       throw new Error( err );
                                    }
                                    else
                                    {
                                       Assert.equal( objectKeysSort( rc ), objectKeysSort( { "n": 2, "ok": 1, "deletedCount": 2 } ) );

                                       // check results
                                       Test.find( {},
                                          function( err, rc )
                                          {
                                             if( err )
                                             {
                                                throw new Error( err );
                                             }
                                             else
                                             {
                                                Assert.equal( objectKeysSort( rc ), objectKeysSort( [{ "_id": 3, "a": 2, "__v": 0 }] ) );
                                             }
                                             console.log( "---test_deleteMany success" );
                                          }
                                       );
                                    }
                                 }
                              );
                           }
                        }
                     );
                  }
               }
            );
         }
      }
   );
}

function test_remove_22465 ()
{
   const clName = 'cl_remove_22465';
   const Test = Mongoose.model( modelName, Schema, clName );
   // 没有创建/删除集合的接口，需要插入记录时自动创建，否则下一步remove清理数据时会报集合不存在
   Test.create( [{ "_id": -1 }],
      function( err )
      {
         if( err )
         {
            throw new Error( err );
         }
         else
         {
            // clean, then test  
            Test.remove( {},
               function( err )
               {
                  if( err )
                  {
                     throw new Error( err );
                  }
                  else
                  {
                     Test.insertMany( [{ '_id': 1, 'a': 1 }, { '_id': 2, 'a': 1 }, { '_id': 3, 'a': 2 }],
                        function( err )
                        {
                           if( err )
                           {
                              throw new Error( err );
                           }
                           else
                           {
                              Test.remove( { "a": 1 },
                                 function( err, rc )
                                 {
                                    if( err )
                                    {
                                       throw new Error( err );
                                    }
                                    else
                                    {
                                       Assert.equal( objectKeysSort( rc ), objectKeysSort( { "n": 2, "ok": 1 } ) );

                                       // check results
                                       Test.find( {},
                                          function( err, rc )
                                          {
                                             if( err )
                                             {
                                                throw new Error( err );
                                             }
                                             else
                                             {
                                                Assert.equal( objectKeysSort( rc ), objectKeysSort( [{ "_id": 3, "a": 2, "__v": 0 }] ) );
                                             }
                                             console.log( "---test_remove success" );
                                          }
                                       );
                                    }
                                 }
                              );
                           }
                        }
                     );
                  }
               }
            );
         }
      }
   );
}

function test_prototype_delete_22465 ()
{
   const clName = 'cl_prototype_delete_22465';
   const Test = Mongoose.model( modelName, Schema, clName );
   // 没有创建/删除集合的接口，需要插入记录时自动创建，否则下一步remove清理数据时会报集合不存在
   Test.create( [{ "_id": -1 }],
      function( err )
      {
         if( err )
         {
            throw new Error( err );
         }
         else
         {
            // clean, then test  
            Test.remove( {},
               function( err )
               {
                  if( err )
                  {
                     throw new Error( err );
                  }
                  else
                  {
                     Test.insertMany( [{ '_id': 1, 'a': 1 }, { '_id': 2, 'a': 2 }],
                        function( err, rc )
                        {
                           if( err )
                           {
                              throw new Error( err );
                           }
                           else
                           {
                              Assert.equal( objectKeysSort( rc ), objectKeysSort( [{ "_id": 1, "a": 1, "__v": 0 }, { "_id": 2, "a": 2, "__v": 0 }] ) );

                              // delete
                              const doc = new Test( { '_id': 1, 'a': 1 } );
                              doc.delete(
                                 function( err, rc )
                                 {
                                    if( err )
                                    {
                                       throw new Error( err );
                                    }
                                    else
                                    {
                                       Assert.equal( objectKeysSort( rc ), objectKeysSort( { "_id": 1, "a": 1 } ) );

                                       // check results
                                       Test.find( {},
                                          function( err, rc )
                                          {
                                             if( err )
                                             {
                                                throw new Error( err );
                                             }
                                             else
                                             {
                                                Assert.equal( objectKeysSort( rc ), objectKeysSort( [{ "_id": 2, "a": 2, "__v": 0 }] ) );
                                             }
                                             console.log( "---test_prototype_delete success" );
                                          }
                                       );
                                    }
                                 }
                              );
                           }
                        }
                     );
                  }
               }
            );
         }
      }
   );
}

function test_prototype_remove_22465 ()
{
   const clName = 'cl_prototype_remove_22465';
   const Test = Mongoose.model( modelName, Schema, clName );
   // 没有创建/删除集合的接口，需要插入记录时自动创建，否则下一步remove清理数据时会报集合不存在
   Test.create( [{ "_id": -1 }],
      function( err )
      {
         if( err )
         {
            throw new Error( err );
         }
         else
         {
            // clean, then test  
            Test.remove( {},
               function( err )
               {
                  if( err )
                  {
                     throw new Error( err );
                  }
                  else
                  {
                     Test.insertMany( [{ '_id': 1, 'a': 1 }, { '_id': 2, 'a': 2 }],
                        function( err, rc )
                        {
                           if( err )
                           {
                              throw new Error( err );
                           }
                           else
                           {
                              Assert.equal( objectKeysSort( rc ), objectKeysSort( [{ "_id": 1, "a": 1, "__v": 0 }, { "_id": 2, "a": 2, "__v": 0 }] ) );

                              // remove
                              const doc = new Test( { '_id': 1, 'a': 1 } );
                              doc.remove(
                                 function( err, rc )
                                 {
                                    if( err )
                                    {
                                       throw new Error( err );
                                    }
                                    else
                                    {
                                       Assert.equal( objectKeysSort( rc ), objectKeysSort( { "_id": 1, "a": 1 } ) );

                                       // check results
                                       Test.find( {},
                                          function( err, rc )
                                          {
                                             if( err )
                                             {
                                                throw new Error( err );
                                             }
                                             else
                                             {
                                                Assert.equal( objectKeysSort( rc ), objectKeysSort( [{ "_id": 2, "a": 2, "__v": 0 }] ) );
                                             }
                                             console.log( "---test_prototype_remove success" );
                                          }
                                       );
                                    }
                                 }
                              );
                           }
                        }
                     );
                  }
               }
            );
         }
      }
   );
}

/*
* 匹配符测试
*/
function test_matcher_22466 ()
{
   const clName = 'cl_matcher_22466';
   const Schema2 = Mongoose.Schema( { "_id": Number, "a": Number, "b": Number, "c": Number, "d": String, "e": Array } );
   const Test = Mongoose.model( 'test_matcher_22466', Schema2, clName );
   // 没有创建/删除集合的接口，需要插入记录时自动创建，否则下一步remove清理数据时会报集合不存在
   Test.create( [{ "_id": -1 }],
      function( err )
      {
         if( err )
         {
            throw new Error( err );
         }
         else
         {
            // clean, then test  
            Test.remove( {},
               function( err )
               {
                  if( err )
                  {
                     throw new Error( err );
                  }
                  else
                  {
                     Test.insertMany( [{ "_id": 1, "a": 1, "b": 1 }, { "_id": 2, "a": 2, "b": 1 },
                     { "_id": 3, "a": 3, "b": 2 }, { "_id": 4, "a": 4, "b": 2 },
                     { "_id": 5, "a": 5, "c": 1 }, { "_id": 6, "d": "abc" }, { "_id": 7, "d": "test" },
                     { "_id": 8, "e": [1, 2] }, { "_id": 9, "e": [1, 2, 3] }],
                        function( err, rc )
                        {
                           if( err )
                           {
                              throw new Error( err );
                           }
                           else
                           {
                              // find by matcher
                              // $ne
                              Test.find( { "a": { "$eq": 2 } }, { "_id": 1, "a": 1, "b": 1 }, { "$sort": { "_id": 1 } },
                                 function( err, rc )
                                 {
                                    if( err )
                                    {
                                       throw new Error( err );
                                    }
                                    else
                                    {
                                       Assert.equal( objectKeysSort( rc ), objectKeysSort( [{ "_id": 2, "a": 2, "b": 1 }] ) );
                                    }
                                 }
                              );

                              // $ne
                              Test.find( { "a": { "$lte": 4 }, "b": { "$ne": 1 } }, { "_id": 1, "a": 1, "b": 1 }, { "$sort": { "_id": 1 } },
                                 function( err, rc )
                                 {
                                    if( err )
                                    {
                                       throw new Error( err );
                                    }
                                    else
                                    {
                                       Assert.equal( objectKeysSort( rc ), objectKeysSort( [{ "_id": 3, "a": 3, "b": 2 }, { "_id": 4, "a": 4, "b": 2 }] ) );
                                    }
                                 }
                              );

                              // $and / $lt / $gt
                              Test.find( { "a": { "$lte": 4 }, "b": { "$ne": 1 } }, { "_id": 1, "a": 1, "b": 1 }, { "$sort": { "_id": 1 } },
                                 function( err, rc )
                                 {
                                    if( err )
                                    {
                                       throw new Error( err );
                                    }
                                    else
                                    {
                                       Assert.equal( objectKeysSort( rc ), objectKeysSort( [{ "_id": 3, "a": 3, "b": 2 }, { "_id": 4, "a": 4, "b": 2 }] ) );
                                    }
                                 }
                              );

                              // $and / $lt / $gt
                              Test.find( { "$and": [{ "a": { "$gt": 1 } }, { "a": { "$lt": 4 } }] }, { "_id": 1, "a": 1, "b": 1 }, { "$sort": { "_id": 1 } },
                                 function( err, rc )
                                 {
                                    if( err )
                                    {
                                       throw new Error( err );
                                    }
                                    else
                                    {
                                       Assert.equal( objectKeysSort( rc ), objectKeysSort( [{ "_id": 2, "a": 2, "b": 1 }, { "_id": 3, "a": 3, "b": 2 }] ) );
                                    }
                                 }
                              );

                              // $or / $lte / $gte
                              Test.find( { "$or": [{ "a": { "$lte": 1 } }, { "a": { "$gte": 4 } }] }, { "_id": 1, "a": 1, "b": 1 }, { "$sort": { "_id": 1 } },
                                 function( err, rc )
                                 {
                                    if( err )
                                    {
                                       throw new Error( err );
                                    }
                                    else
                                    {
                                       Assert.equal( objectKeysSort( rc ), objectKeysSort( [{ "_id": 1, "a": 1, "b": 1 }, { "_id": 4, "a": 4, "b": 2 }, { "_id": 5, "a": 5 }] ) );
                                    }
                                 }
                              );

                              // $exists
                              Test.find( { "c": { "$exists": 1 } }, { "_id": 1, "a": 1, "b": 1, "c": 1 }, { "$sort": { "_id": 1 } },
                                 function( err, rc )
                                 {
                                    if( err )
                                    {
                                       throw new Error( err );
                                    }
                                    else
                                    {
                                       Assert.equal( objectKeysSort( rc ), objectKeysSort( [{ "_id": 5, "a": 5, "c": 1 }] ) );
                                    }
                                 }
                              );

                              // $mod
                              Test.find( { "$and": [{ "a": { "$lt": 5 } }, { "a": { "$mod": [3, 1] } }] }, { "_id": 1, "a": 1, "b": 1 }, { "$sort": { "_id": 1 } },
                                 function( err, rc )
                                 {
                                    if( err )
                                    {
                                       throw new Error( err );
                                    }
                                    else
                                    {
                                       Assert.equal( objectKeysSort( rc ), objectKeysSort( [{ "_id": 1, "a": 1, "b": 1 }, { "_id": 4, "a": 4, "b": 2 }] ) );
                                    }
                                 }
                              );

                              // $regex
                              Test.find( { "d": { "$regex": "^a", "$options": "i" } }, { "_id": 1, "d": 1 }, { "$sort": { "_id": 1 } },
                                 function( err, rc )
                                 {
                                    if( err )
                                    {
                                       throw new Error( err );
                                    }
                                    else
                                    {
                                       Assert.equal( objectKeysSort( rc ), objectKeysSort( [{ "_id": 6, "d": "abc" }] ) );
                                    }
                                 }
                              );

                              // array
                              // $all
                              Test.find( { "e": { "$all": [2, 3] } }, { "_id": 1, "e": 1 }, { "$sort": { "_id": 1 } },
                                 function( err, rc )
                                 {
                                    if( err )
                                    {
                                       throw new Error( err );
                                    }
                                    else
                                    {
                                       Assert.equal( objectKeysSort( rc ), objectKeysSort( [{ "_id": 9, "e": [1, 2, 3] }] ) );
                                    }
                                    console.log( "---test_matcher success" );
                                 }
                              );
                           }
                        }
                     );
                  }
               }
            );
         }
      }
   );
}

/*
* 选择符测试
*/
function test_selector_22467 ()
{
   const clName = 'cl_selector_22467';
   const Schema2 = Mongoose.Schema( { "_id": Number, "a": Number, "b": Number, "c": Array } );
   const Test = Mongoose.model( 'cl_selector_22467', Schema2, clName );
   // 没有创建/删除集合的接口，需要插入记录时自动创建，否则下一步remove清理数据时会报集合不存在
   Test.create( [{ "_id": -1 }],
      function( err )
      {
         if( err )
         {
            throw new Error( err );
         }
         else
         {
            // clean, then test  
            Test.remove( {},
               function( err )
               {
                  if( err )
                  {
                     throw new Error( err );
                  }
                  else
                  {
                     Test.insertMany( [{ "_id": 1, "a": 1, "b": 1, "c": [1] },
                     { "_id": 2, "a": 2, "c": [1, 2, 3, 4, 5] },
                     { "_id": 3, "a": 3, "c": [{ "b1": 100 }, { "b1": 101 }] }],
                        function( err, rc )
                        {
                           if( err )
                           {
                              throw new Error( err );
                           }
                           else
                           {
                              // $elemMatch, mongoose不支持

                              // $slice: int
                              Test.find( { "a": 2 }, { "_id": 1, "a": 1, "c": { "$slice": 2 } },
                                 function( err, rc )
                                 {
                                    if( err )
                                    {
                                       throw new Error( err );
                                    }
                                    else
                                    {
                                       Assert.equal( objectKeysSort( rc ), objectKeysSort( [{ "_id": 2, "a": 2, "c": [1, 2] }] ) );
                                    }
                                 }
                              );

                              // $slice, not include _id
                              Test.find( { "a": 2 }, { "a": 1, "c": { "$slice": [2, 3] } },
                                 function( err, rc )
                                 {
                                    if( err )
                                    {
                                       throw new Error( err );
                                    }
                                    else
                                    {
                                       Assert.equal( objectKeysSort( rc ), objectKeysSort( [{ "a": 2, "c": [3, 4, 5] }] ) );
                                    }
                                 }
                              );

                              // $slice, include _id
                              Test.find( { "a": 2 }, { "_id": 1, "a": 1, "c": { "$slice": [2, 3] } },
                                 function( err, rc )
                                 {
                                    if( err )
                                    {
                                       throw new Error( err );
                                    }
                                    else
                                    {
                                       Assert.equal( objectKeysSort( rc ), objectKeysSort( [{ "_id": 2, "a": 2, "c": [3, 4, 5] }] ) );
                                    }
                                 }
                              );

                              // 选择字段，mongoose不支持排除字段
                              Test.find( { "a": 1 }, { "_id": 1, "a": 1 },
                                 function( err, rc )
                                 {
                                    if( err )
                                    {
                                       throw new Error( err );
                                    }
                                    else
                                    {
                                       Assert.equal( objectKeysSort( rc ), objectKeysSort( [{ "_id": 1, "a": 1 }] ) );
                                    }
                                    console.log( "---test_selector success" );
                                 }
                              );
                           }
                        }
                     );
                  }
               }
            );
         }
      }
   );
}


/*
* 更新符测试
*/
function test_updater_object_22468 ()
{
   const clName = 'cl_updater_object_22468';
   const Test = Mongoose.model( modelName, Schema, clName );
   // 没有创建/删除集合的接口，需要插入记录时自动创建，否则下一步remove清理数据时会报集合不存在
   Test.create( [{ "_id": -1 }],
      function( err )
      {
         if( err )
         {
            throw new Error( err );
         }
         else
         {
            // clean, then test  
            Test.remove( {},
               function( err )
               {
                  if( err )
                  {
                     throw new Error( err );
                  }
                  else
                  {
                     // $unset
                     Test.create( { '_id': 0, 'a': 0 },
                        function( err, rc )
                        {
                           if( err )
                           {
                              throw new Error( err );
                           }
                           else
                           {
                              Assert.equal( objectKeysSort( rc ), objectKeysSort( { "_id": 0, "a": 0, "__v": 0 } ) );

                              Test.updateMany( { "_id": { "$eq": 0 } }, { "$unset": { "a": 0 } }, {},
                                 function( err, rc )
                                 {
                                    if( err )
                                    {
                                       throw new Error( err );
                                    }
                                    else
                                    {
                                       Assert.equal( objectKeysSort( rc ), objectKeysSort( { "n": 1, "nModified": 1, "ok": 1 } ) );

                                       // check results
                                       Test.find( { "_id": 0 },
                                          function( err, rc )
                                          {
                                             if( err )
                                             {
                                                throw new Error( err );
                                             }
                                             else
                                             {
                                                Assert.equal( objectKeysSort( rc ), objectKeysSort( [{ "__v": 0, "_id": 0 }] ) );
                                             }
                                          }
                                       );
                                    }
                                 }
                              );
                           }
                        }
                     );


                     // upsert
                     // 记录不存在，upsert: true
                     Test.updateMany( {}, { "$set": { "_id": 1, "a": 1 } }, { "upsert": true },
                        function( err, rc )
                        {
                           if( err )
                           {
                              throw new Error( err );
                           }
                           else
                           {
                              Assert.equal( objectKeysSort( rc ), objectKeysSort( { "n": 1, "nModified": 0, "ok": 1, "upserted": [{ "index": 0, "_id": 1 }] } ) );

                              // 记录不存在，upsert: false
                              Test.updateMany( { "a": 2 }, { "$set": { "_id": 2, "a": 2 } }, { "upsert": false },
                                 function( err, rc )
                                 {
                                    if( err )
                                    {
                                       throw new Error( err );
                                    }
                                    else
                                    {
                                       Assert.equal( objectKeysSort( rc ), objectKeysSort( { "n": 0, "nModified": 0, "ok": 1 } ) );

                                       // check results
                                       Test.find( { "a": { "$in": [1, 2] } },
                                          function( err, rc )
                                          {
                                             if( err )
                                             {
                                                throw new Error( err );
                                             }
                                             else
                                             {
                                                Assert.equal( objectKeysSort( rc ), objectKeysSort( [{ "__v": 0, "_id": 1, "a": 1 }] ) );
                                             }
                                          }
                                       );
                                    }
                                 }
                              );
                           }
                        }
                     );

                     // $setOnInsert, 匹配不存在的记录, upsert:ture|false
                     // $setOnInsert + upsert:true
                     Test.updateMany( { "a": 3 }, { "$setOnInsert": { "_id": 3, "a": 3 } }, { "upsert": true },
                        function( err, rc )
                        {
                           if( err )
                           {
                              throw new Error( err );
                           }
                           else
                           {
                              Assert.equal( objectKeysSort( rc ), objectKeysSort( { "n": 1, "nModified": 0, "ok": 1, "upserted": [{ "index": 0, "_id": 3 }] } ) );

                              // $setOnInsert + upsert:false
                              Test.updateMany( { "a": 4 }, { "$setOnInsert": { "_id": 4, "a": 4 } }, { "upsert": false },
                                 function( err, rc )
                                 {
                                    if( err )
                                    {
                                       if( err.message !== 'Invalid Argument' ) 
                                       {
                                          throw new Error( err );
                                       }

                                       // check results
                                       Test.find( { "a": { "$in": [3, 4] } },
                                          function( err, rc )
                                          {
                                             if( err )
                                             {
                                                throw new Error( err );
                                             }
                                             else
                                             {
                                                Assert.equal( objectKeysSort( rc ), objectKeysSort( [{ "__v": 0, "_id": 3, "a": 3 }] ) );
                                             }
                                          }
                                       );
                                    }
                                 }
                              );
                           }
                        }
                     );

                     // 记录不存在，$set + $setOnInsert组合, upsert:true 
                     Test.updateMany( { "a": 5 }, { "$set": { "_id": 5 }, "$setOnInsert": { "a": 5 } }, { "upsert": true },
                        function( err, rc )
                        {
                           if( err )
                           {
                              throw new Error( err );
                           }
                           else
                           {
                              Assert.equal( objectKeysSort( rc ), objectKeysSort( { "n": 1, "nModified": 0, "ok": 1, "upserted": [{ "index": 0, "_id": 5 }] } ) );

                              // 记录不存在，$inc + $setOnInsert，upsert:true 
                              Test.updateMany( { "a": 6 }, { "$inc": { "a": 6 }, "$setOnInsert": { "_id": 6 } }, { "upsert": true },
                                 function( err, rc )
                                 {
                                    if( err )
                                    {
                                       throw new Error( err );
                                    }
                                    else
                                    {
                                       Assert.equal( objectKeysSort( rc ), objectKeysSort( { "n": 1, "nModified": 0, "ok": 1, "upserted": [{ "index": 0, "_id": 6 }] } ) );

                                       // { "$inc": { "b": 1 } }, { "multi": true }
                                       Test.update( { "_id": { "$in": [5, 6] } }, { "$inc": { "b": 1 } }, { "multi": true },
                                          function( err, rc )
                                          {
                                             if( err )
                                             {
                                                throw new Error( err );
                                             }
                                             else
                                             {
                                                Assert.equal( objectKeysSort( rc ), objectKeysSort( { "n": 2, "nModified": 2, "ok": 1 } ) );

                                                // check results
                                                Test.find( { "_id": { "$in": [5, 6] } },
                                                   function( err, rc )
                                                   {
                                                      if( err )
                                                      {
                                                         throw new Error( err );
                                                      }
                                                      else
                                                      {
                                                         Assert.equal( objectKeysSort( rc ), objectKeysSort( [{ "__v": 0, "_id": 5, "a": 5, "b": 1 }, { "__v": 0, "_id": 6, "a": 12, "b": 1 }] ) );
                                                      }
                                                   }
                                                );
                                             }
                                          }
                                       );
                                    }
                                 }
                              );
                           }
                        }
                     );

                     // $setOnInsert，upsert:true, 字段不包含"_id" 
                     Test.updateMany( { "a": 7 }, { "$set": { "_id": 7 }, "$setOnInsert": { "a": 7 } }, { "upsert": true },
                        function( err, rc )
                        {
                           if( err )
                           {
                              throw new Error( err );
                           }
                           else
                           {
                              Assert.equal( objectKeysSort( rc ), objectKeysSort( { "n": 1, "nModified": 0, "ok": 1, "upserted": [{ "index": 0, "_id": 7 }] } ) );

                              // check results
                              Test.find( { "_id": 7 },
                                 function( err, rc )
                                 {
                                    if( err )
                                    {
                                       throw new Error( err );
                                    }
                                    else
                                    {
                                       Assert.equal( objectKeysSort( rc ), objectKeysSort( [{ "__v": 0, "_id": 7, "a": 7 }] ) );
                                    }
                                    console.log( "---test_updater success" );
                                 }
                              );
                           }
                        }
                     );
                  }
               }
            );
         }
      }
   );
}

function test_updater_array_22468 ()
{
   const clName = 'cl_updater_array_22468';
   const Schema2 = Mongoose.Schema( { "_id": Number, "a": Number, "b": Array, "c": Array } );
   const Test = Mongoose.model( 'test_updater_array_22468', Schema2, clName );
   // 没有创建/删除集合的接口，需要插入记录时自动创建，否则下一步remove清理数据时会报集合不存在
   Test.create( [{ "_id": -1 }],
      function( err )
      {
         if( err )
         {
            throw new Error( err );
         }
         else
         {
            // clean, then test
            Test.remove( {},
               function( err )
               {
                  if( err )
                  {
                     throw new Error( err );
                  }
                  else
                  {
                     Test.insertMany( [{ "_id": 1, "a": 1, "b": [1], "c": [1] },
                     { "_id": 2, "a": 2, "b": [1, 2], "c": [1, 2, 3] },
                     { "_id": 3, "a": 3, "b": [1], "c": [2] },
                     { "_id": 4, "a": 4, "b": [1, 2], "c": [1, 2] },
                     { "_id": 5, "a": 5, "b": [], "c": [1] },
                     { "_id": 6, "a": 6, "b": [1], "c": [1, 2] }],
                        function( err )
                        {
                           if( err )
                           {
                              throw new Error( err );
                           }
                           else
                           {
                              // $pop
                              Test.updateMany( { "a": { "$in": [1, 2] } }, { "$pop": { "b": 1, "c": 2 } },
                                 function( err, rc )
                                 {
                                    if( err )
                                    {
                                       throw new Error( err );
                                    }
                                    else
                                    {
                                       Assert.equal( objectKeysSort( rc ), objectKeysSort( { "n": 2, "nModified": 2, "ok": 1 } ) );

                                       // check results
                                       Test.find( { "a": { "$in": [1, 2] } },
                                          function( err, rc )
                                          {
                                             if( err )
                                             {
                                                throw new Error( err );
                                             }
                                             else
                                             {
                                                Assert.equal( objectKeysSort( rc ), objectKeysSort( [{ "__v": 0, "_id": 1, "a": 1, "b": [], "c": [] }, { "__v": 0, "_id": 2, "a": 2, "b": [1], "c": [1] }] ) );
                                             }
                                          }
                                       );
                                    }
                                 }
                              );

                              // $pull
                              Test.updateMany( { "a": { "$in": [3, 4] } }, { "$pull": { "b": 1, "c": 2 } },
                                 function( err, rc )
                                 {
                                    if( err )
                                    {
                                       throw new Error( err );
                                    }
                                    else
                                    {
                                       Assert.equal( objectKeysSort( rc ), objectKeysSort( { "n": 2, "nModified": 2, "ok": 1 } ) );

                                       // check results
                                       Test.find( { "a": { "$in": [3, 4] } },
                                          function( err, rc )
                                          {
                                             if( err )
                                             {
                                                throw new Error( err );
                                             }
                                             else
                                             {
                                                Assert.equal( objectKeysSort( rc ), objectKeysSort( [{ "__v": 0, "_id": 3, "a": 3, "b": [], "c": [] }, { "__v": 0, "_id": 4, "a": 4, "b": [2], "c": [1] }] ) );
                                             }
                                          }
                                       );
                                    }
                                 }
                              );

                              // $push
                              Test.updateMany( { "a": { "$in": [5, 6] } }, { "$push": { "b": 1, "c": 3 } },
                                 function( err, rc )
                                 {
                                    if( err )
                                    {
                                       throw new Error( err );
                                    }
                                    else
                                    {
                                       Assert.equal( objectKeysSort( rc ), objectKeysSort( { "n": 2, "nModified": 2, "ok": 1 } ) );

                                       // check results
                                       Test.find( { "a": { "$in": [5, 6] } },
                                          function( err, rc )
                                          {
                                             if( err )
                                             {
                                                throw new Error( err );
                                             }
                                             else
                                             {
                                                Assert.equal( objectKeysSort( rc ), objectKeysSort( [{ "__v": 0, "_id": 5, "a": 5, "b": [1], "c": [1, 3] }, { "__v": 0, "_id": 6, "a": 6, "b": [1, 1], "c": [1, 2, 3] }] ) );
                                             }
                                             console.log( "---test_updateMany success" );
                                          }
                                       );
                                    }
                                 }
                              );
                           }
                        }
                     );
                  }
               }
            );
         }
      }
   );
}

function test_aggregate_22469 ()
{
   const clName = 'cl_aggregate_22469';
   const Schema2 = Mongoose.Schema( { "_id": Number, "a": String, "b": Number, "c": String, "d": Number } );
   const Test = Mongoose.model( 'test_aggregate_22469', Schema2, clName );
   // 没有创建/删除集合的接口，需要插入记录时自动创建，否则下一步remove清理数据时会报集合不存在
   Test.create( [{ "_id": -1 }],
      function( err )
      {
         if( err )
         {
            throw new Error( err );
         }
         else
         {
            // clean, then test  
            Test.remove( {},
               function( err )
               {
                  if( err )
                  {
                     throw new Error( err );
                  }
                  else
                  {
                     const docs = [
                        { "_id": 1, "a": "A", "b": 86, "c": "test", "d": 1 },
                        { "_id": 2, "a": "B", "b": 90, "c": "dev", "d": 1 },
                        { "_id": 3, "a": "C", "b": 100, "c": "test", "d": 2 },
                        { "_id": 4, "a": "D", "b": 79, "c": "dev", "d": 2 }
                     ];
                     Test.insertMany( docs,
                        function( err )
                        {
                           if( err )
                           {
                              throw new Error( err );
                           }
                           else
                           {
                              // $progect
                              // 选择单个普通字段
                              // a:1
                              Test.aggregate( [{ "$project": { "a": 1 } }, { "$sort": { "_id": 1 } }],
                                 function( err, rc )
                                 {
                                    if( err )
                                    {
                                       throw new Error( err );
                                    }
                                    else
                                    {
                                       Assert.equal( objectKeysSort( rc ), objectKeysSort( [{ "_id": 1, "a": "A" }, { "_id": 2, "a": "B" }, { "_id": 3, "a": "C" }, { "_id": 4, "a": "D" }] ) );
                                    }
                                 }
                              );

                              // a:0
                              Test.aggregate( [{ "$project": { "a": 0 } }],
                                 function( err, rc )
                                 {
                                    if( err )
                                    {
                                       if( err.message !== 'Exclusion fields is not supported' )
                                       {
                                          throw new Error( err );
                                       }
                                    }
                                    else
                                    {
                                       throw new Error( "expect fail but success." );
                                    }
                                 }
                              );

                              // _id:1
                              Test.aggregate( [{ "$project": { "_id": 1 } }, { "$sort": { "_id": 1 } }],
                                 function( err, rc )
                                 {
                                    if( err )
                                    {
                                       throw new Error( err );
                                    }
                                    else
                                    {
                                       Assert.equal( objectKeysSort( rc ), objectKeysSort( [{ "_id": 1 }, { "_id": 2 }, { "_id": 3 }, { "_id": 4 }] ) );
                                    }
                                 }
                              );

                              // 排除_id字段，并选择某个普通字段
                              Test.aggregate( [{ "$project": { "_id": 0, "b": 1 } }, { "$sort": { "b": 1 } }],
                                 function( err, rc )
                                 {
                                    if( err )
                                    {
                                       throw new Error( err );
                                    }
                                    else
                                    {
                                       Assert.equal( objectKeysSort( rc ), objectKeysSort( [{ "b": 79 }, { "b": 86 }, { "b": 90 }, { "b": 100 }] ) );
                                    }
                                 }
                              );

                              // 选择_id和普通字段，并排除某个普通字段
                              Test.aggregate( [{ "$project": { "_id": 1, "b": 1, "c": 0, "d": 1 } }, { "$sort": { "b": 1 } }],
                                 function( err, rc )
                                 {
                                    if( err )
                                    {
                                       throw new Error( err );
                                    }
                                    else
                                    {
                                       Assert.equal( objectKeysSort( rc ), objectKeysSort( [{ "_id": 4, "b": 79, "d": 2 }, { "_id": 1, "b": 86, "d": 1 }, { "_id": 2, "b": 90, "d": 1 }, { "_id": 3, "b": 100, "d": 2 }] ) );
                                    }
                                 }
                              );


                              // $group
                              // $first / $last
                              Test.aggregate( [{ "$group": { "_id": "$c", "first_c": { "$first": "$c" }, "max_b": { "$max": "$b" }, "last_c": { "$last": "$c" } } }],
                                 function( err, rc )
                                 {
                                    if( err )
                                    {
                                       throw new Error( err );
                                    }
                                    else
                                    {
                                       Assert.equal( objectKeysSort( rc ), objectKeysSort( [{ "_id": "dev", "first_c": "dev", "max_b": 90, "last_c": "dev" }, { "_id": "test", "first_c": "test", "max_b": 100, "last_c": "test" }] ) );
                                    }
                                 }
                              );

                              // $max / $min
                              Test.aggregate( [{ "$group": { "_id": "$c", "max_b": { "$max": "$b" }, "min_b": { "$min": "$b" } } }, { "$sort": { "_id": 1 } }],
                                 function( err, rc )
                                 {
                                    if( err )
                                    {
                                       throw new Error( err );
                                    }
                                    else
                                    {
                                       Assert.equal( objectKeysSort( rc ), objectKeysSort( [{ "_id": "dev", "max_b": 90, "min_b": 79 }, { "_id": "test", "max_b": 100, "min_b": 86 }] ) );
                                    }
                                 }
                              );

                              // $push   
                              Test.aggregate( [{ "$group": { "_id": "$c", "push_d": { "$push": "$d" } } }, { "$sort": { "_id": 1 } }],
                                 function( err, rc )
                                 {
                                    if( err )
                                    {
                                       throw new Error( err );
                                    }
                                    else
                                    {
                                       Assert.equal( objectKeysSort( rc ), objectKeysSort( [{ "_id": "dev", "push_d": [1, 2] }, { "_id": "test", "push_d": [1, 2] }] ) );
                                    }
                                 }
                              );

                              // $avg / $addToSet  
                              Test.aggregate( [{ "$group": { "_id": "$c", "avg_b": { "$avg": "$b" }, "addtoset_d": { "$addToSet": "$d" } } }, { "$sort": { "_id": 1 } }],
                                 function( err, rc )
                                 {
                                    if( err )
                                    {
                                       throw new Error( err );
                                    }
                                    else
                                    {
                                       Assert.equal( objectKeysSort( rc ), objectKeysSort( [{ "_id": "dev", "avg_b": 84.5, "addtoset_d": [1, 2] }, { "_id": "test", "avg_b": 93, "addtoset_d": [1, 2] }] ) );
                                    }
                                 }
                              );

                              // $sum
                              Test.aggregate( [{ "$group": { "_id": "$c", "sum_b": { "$sum": "$b" } } }, { "$sort": { "_id": 1 } }],
                                 function( err, rc )
                                 {
                                    if( err )
                                    {
                                       throw new Error( err );
                                    }
                                    else
                                    {
                                       Assert.equal( objectKeysSort( rc ), objectKeysSort( [{ "_id": "dev", "sum_b": 169 }, { "_id": "test", "sum_b": 186 }] ) );
                                    }
                                 }
                              );


                              // $match      
                              // 匹配返回多条记录
                              Test.aggregate( [{ "$match": { "b": { "$gte": 90 } } }, { "$project": { "b": 1 } }, { "$sort": { "_id": 1 } }],
                                 function( err, rc )
                                 {
                                    if( err )
                                    {
                                       throw new Error( err );
                                    }
                                    else
                                    {
                                       Assert.equal( objectKeysSort( rc ), objectKeysSort( [{ "_id": 2, "b": 90 }, { "_id": 3, "b": 100 }] ) );
                                    }
                                 }
                              );

                              // $sort
                              // 多个字段正逆序
                              Test.aggregate( [{ "$match": { "b": { "$gte": 70 } } }, { "$sort": { "d": -1, "b": 1 } }],
                                 function( err, rc )
                                 {
                                    if( err )
                                    {
                                       throw new Error( err );
                                    }
                                    else
                                    {
                                       Assert.equal( objectKeysSort( rc ), objectKeysSort( [{ "__v": 0, "_id": 4, "a": "D", "b": 79, "c": "dev", "d": 2 }, { "__v": 0, "_id": 3, "a": "C", "b": 100, "c": "test", "d": 2 }, { "__v": 0, "_id": 1, "a": "A", "b": 86, "c": "test", "d": 1 }, { "__v": 0, "_id": 2, "a": "B", "b": 90, "c": "dev", "d": 1 }] ) );
                                    }
                                 }
                              );

                              // $limit
                              Test.aggregate( [{ "$limit": 3 }, { "$sort": { "b": 1 } }],
                                 function( err, rc )
                                 {
                                    if( err )
                                    {
                                       throw new Error( err );
                                    }
                                    else
                                    {
                                       Assert.equal( objectKeysSort( rc ), objectKeysSort( [{ "__v": 0, "_id": 1, "a": "A", "b": 86, "c": "test", "d": 1 }, { "__v": 0, "_id": 2, "a": "B", "b": 90, "c": "dev", "d": 1 }, { "__v": 0, "_id": 3, "a": "C", "b": 100, "c": "test", "d": 2 }] ) );
                                    }
                                    console.log( "---test_aggregate success" );
                                 }
                              );
                           }
                        }
                     );
                  }
               }
            );
         }
      }
   );
}

function test_count_22470 ()
{
   const clName = 'cl_count_22470';
   const Test = Mongoose.model( modelName, Schema, clName );
   // 没有创建/删除集合的接口，需要插入记录时自动创建，否则下一步remove清理数据时会报集合不存在
   Test.create( [{ "_id": -1 }],
      function( err )
      {
         if( err )
         {
            throw new Error( err );
         }
         else
         {
            // clean, then test  
            Test.remove( {},
               function( err )
               {
                  if( err )
                  {
                     throw new Error( err );
                  }
                  else
                  {
                     const docsNum = 2100;
                     var docs = [];
                     for( var i = 0; i < docsNum; i++ )
                     {
                        docs.push( { "_id": i, "a": i } );
                     }
                     Test.insertMany( docs,
                        function( err )
                        {
                           if( err )
                           {
                              throw new Error( err );
                           }
                           else
                           {
                              // count, match all docs
                              Test.count( {},
                                 function( err, rc )
                                 {
                                    if( err )
                                    {
                                       throw new Error( err );
                                    }
                                    else
                                    {
                                       Assert.equal( rc, docsNum );
                                    }
                                 }
                              );

                              // count, not match docs
                              Test.count( { "a": { "$lt": 0 } },
                                 function( err, rc )
                                 {
                                    if( err )
                                    {
                                       throw new Error( err );
                                    }
                                    else
                                    {
                                       Assert.equal( rc, 0 );
                                    }
                                 }
                              );

                              // countDocuments, match party docs 
                              Test.countDocuments( { "a": { "$lt": 999 } },
                                 function( err, rc )
                                 {
                                    if( err )
                                    {
                                       throw new Error( err );
                                    }
                                    else
                                    {
                                       Assert.equal( rc, 999 );
                                    }
                                 }
                              );

                              Test.countDocuments( { "a": { "$lt": 1000 } },
                                 function( err, rc )
                                 {
                                    if( err )
                                    {
                                       throw new Error( err );
                                    }
                                    else
                                    {
                                       Assert.equal( rc, 1000 );
                                    }
                                 }
                              );

                              Test.countDocuments( { "a": { "$lt": 1001 } },
                                 function( err, rc )
                                 {
                                    if( err )
                                    {
                                       throw new Error( err );
                                    }
                                    else
                                    {
                                       Assert.equal( rc, 1001 );
                                    }
                                    console.log( "---test_count success" );
                                 }
                              );
                           }
                        }
                     );
                  }
               }
            );
         }
      }
   );
}

function test_distinct_22471 ()
{
   const clName = 'cl_distinct_22471';
   const Test = Mongoose.model( modelName, Schema, clName );
   // 没有创建/删除集合的接口，需要插入记录时自动创建，否则下一步remove清理数据时会报集合不存在
   Test.create( [{ "_id": -1 }],
      function( err )
      {
         if( err )
         {
            throw new Error( err );
         }
         else
         {
            // clean, then test  
            Test.remove( {},
               function( err )
               {
                  if( err )
                  {
                     throw new Error( err );
                  }
                  else
                  {
                     Test.insertMany( [{ '_id': 1, 'a': 1 }, { '_id': 2, 'a': 1 },
                     { '_id': 3, 'a': 3 }, { '_id': 4, 'a': 4 }, { '_id': 5, 'a': 4 }],
                        function( err, rc )
                        {
                           if( err )
                           {
                              throw new Error( err );
                           }
                           else
                           {
                              // distinct, match multi docs, exist repeat value
                              Test.distinct( "a", { "a": { "$gte": 3 } },
                                 function( err, rc )
                                 {
                                    if( err )
                                    {
                                       throw new Error( err );
                                    }
                                    else
                                    {
                                       Assert.equal( JSON.stringify( rc ), '[3,4]' );
                                    }
                                 }
                              );

                              // distinct, not match doc
                              Test.distinct( "a", { "a": { "$lt": 0 } },
                                 function( err, rc )
                                 {
                                    if( err )
                                    {
                                       throw new Error( err );
                                    }
                                    else
                                    {
                                       Assert.equal( JSON.stringify( rc ), '[]' );
                                    }
                                 }
                              );

                              // distinct, field not exist
                              Test.distinct( "notExist", {},
                                 function( err, rc )
                                 {
                                    if( err )
                                    {
                                       throw new Error( err );
                                    }
                                    else
                                    {
                                       Assert.equal( JSON.stringify( rc ), '[]' );
                                    }
                                    console.log( "---test_distinct success" );
                                 }
                              );
                           }
                        }
                     );
                  }
               }
            );
         }
      }
   );
}

function test_prototype_increment_22472 ()
{
   const clName = 'cl_prototype_increment_22472';
   const Test = Mongoose.model( modelName, Schema, clName );
   // 没有创建/删除集合的接口，需要插入记录时自动创建，否则下一步remove清理数据时会报集合不存在
   Test.create( [{ "_id": -1 }],
      function( err )
      {
         if( err )
         {
            throw new Error( err );
         }
         else
         {
            // clean, then test  
            Test.remove( {},
               function( err )
               {
                  if( err )
                  {
                     throw new Error( err );
                  }
                  else
                  {
                     Test.insertMany( [{ '_id': 1, 'a': 1 }, { '_id': 2, 'a': 2 }],
                        function( err, rc )
                        {
                           if( err )
                           {
                              throw new Error( err );
                           }
                           else
                           {
                              Assert.equal( objectKeysSort( rc ), objectKeysSort( [{ "__v": 0, "_id": 1, "a": 1 }, { "__v": 0, "_id": 2, "a": 2 }] ) );

                              // increment
                              Test.findById( 1,
                                 function( err, doc )
                                 {
                                    if( err )
                                    {
                                       throw new Error( err );
                                    }
                                    else
                                    {
                                       doc.increment();
                                       doc.save(
                                          function( err, rc )
                                          {
                                             if( err )
                                             {
                                                throw new Error( err );
                                             }
                                             else
                                             {
                                                Assert.equal( objectKeysSort( rc ), objectKeysSort( { "__v": 1, "_id": 1, "a": 1 } ) );

                                                // check results
                                                Test.find( {},
                                                   function( err, rc )
                                                   {
                                                      if( err )
                                                      {
                                                         throw new Error( err );
                                                      }
                                                      else
                                                      {
                                                         Assert.equal( objectKeysSort( rc ), objectKeysSort( [{ "__v": 1, "_id": 1, "a": 1 }, { "__v": 0, "_id": 2, "a": 2 }] ) );
                                                      }
                                                      console.log( "---test_prototype_increment success" );
                                                   }
                                                );
                                             }
                                          }
                                       );
                                    }
                                 }
                              );
                           }
                        }
                     );
                  }
               }
            );
         }
      }
   );
}

function test_crud_largeData_22505 ()
{
   const clName = 'cl_crud_largeData_22505';
   const Test = Mongoose.model( modelName, Schema, clName );
   // 没有创建/删除集合的接口，需要插入记录时自动创建，否则下一步remove清理数据时会报集合不存在
   Test.create( [{ "_id": -1 }],
      function( err )
      {
         if( err )
         {
            throw new Error( err );
         }
         else
         {
            // clean, then test
            Test.remove( {},
               function( err )
               {
                  if( err )
                  {
                     throw new Error( err );
                  }
                  else
                  {
                     // ready data
                     const docsNumArray = [2100, 10000, 50000];
                     const docsNum = docsNumArray[Math.floor( Math.random() * docsNumArray.length )];
                     console.log( "---test_crud_largeData_22505 docsNum: " + docsNum );

                     const updateDocsNum = docsNum - 100;
                     const deleteDocsNum = docsNum - 100;
                     const findDocsNum = docsNum - 100;

                     var docs = [];
                     var expDocsForInsert = [];
                     var expDocsForUpdate = [];
                     for( var i = 0; i < docsNum; i++ )
                     {
                        docs.push( { '_id': i, 'a': i, 'b': 1 } );
                        expDocsForInsert.push( { '_id': i, 'a': i, 'b': 1, "__v": 0 } );
                        expDocsForUpdate.push( { '_id': i, 'a': i, 'b': 2, "__v": 0 } );
                     }

                     // insert large data
                     Test.insertMany( docs,
                        function( err, rc )
                        {
                           if( err )
                           {
                              throw new Error( err );
                           }
                           else
                           {
                              checkLargeData( rc, expDocsForInsert );

                              // check results
                              Test.find( {},
                                 function( err, rc )
                                 {
                                    if( err )
                                    {
                                       throw new Error( err );
                                    }
                                    else
                                    {
                                       checkLargeData( rc, expDocsForInsert );

                                       // update large data
                                       Test.updateMany( { "a": { "$lt": updateDocsNum } }, { "$inc": { "b": 1 } }, {},
                                          function( err, rc )
                                          {
                                             if( err )
                                             {
                                                throw new Error( err );
                                             }
                                             else
                                             {
                                                // 返回 { "n": updateDocsNum, "nModified": updateDocsNum, "ok": 1 } ，但字段顺序不固定
                                                Assert.equal( rc.ok, 1 );
                                                Assert.equal( rc.n, updateDocsNum );
                                                Assert.equal( rc.nModified, updateDocsNum );

                                                // check results
                                                Test.find( { "a": { "$lt": updateDocsNum } },
                                                   function( err, rc )
                                                   {
                                                      if( err )
                                                      {
                                                         throw new Error( err );
                                                      }
                                                      else
                                                      {
                                                         checkLargeData( rc, expDocsForUpdate.slice( 0, updateDocsNum ) );

                                                         // delete large data
                                                         Test.deleteMany( { "a": { "$lt": deleteDocsNum } },
                                                            function( err, rc )
                                                            {
                                                               if( err )
                                                               {
                                                                  throw new Error( err );
                                                               }
                                                               else
                                                               {
                                                                  Assert.equal( objectKeysSort( rc ), objectKeysSort( { "n": deleteDocsNum, "ok": 1, "deletedCount": deleteDocsNum } ) );

                                                                  // check results
                                                                  Test.find( {},
                                                                     function( err, rc )
                                                                     {
                                                                        if( err )
                                                                        {
                                                                           throw new Error( err );
                                                                        }
                                                                        else
                                                                        {
                                                                           checkLargeData( rc, expDocsForInsert.slice( deleteDocsNum ) );
                                                                        }
                                                                     }
                                                                  );

                                                                  // find, rc num = 1
                                                                  Test.find( { "a": findDocsNum },
                                                                     function( err, rc )
                                                                     {
                                                                        if( err )
                                                                        {
                                                                           throw new Error( err );
                                                                        }
                                                                        else
                                                                        {
                                                                           Assert.equal( objectKeysSort( rc ), objectKeysSort( [{ "__v": 0, "_id": findDocsNum, "a": findDocsNum, "b": 1 }] ) );
                                                                        }
                                                                     }
                                                                  );

                                                                  // find, rc num = 0
                                                                  Test.find( { "a": 0 },
                                                                     function( err, rc )
                                                                     {
                                                                        if( err )
                                                                        {
                                                                           throw new Error( err );
                                                                        }
                                                                        else
                                                                        {
                                                                           Assert.equal( JSON.stringify( rc ), JSON.stringify( [] ) );
                                                                        }

                                                                        console.log( "---test_crud_largeData success" );
                                                                     }
                                                                  );
                                                               }
                                                            }
                                                         );
                                                      }
                                                   }
                                                );
                                             }
                                          }
                                       );
                                    }
                                 }
                              );
                           }
                        }
                     );
                  }
               }
            );
         }
      }
   );
}

/* 暂不支持
// lack: replaceOne / deleteMany
function test_bulkWrite ()
{
   const clName = 'cl_bulkWrite';
   const Schema2 = Mongoose.Schema( { "_id": Number, "a": Number, "b": Number, "c": Number } );
   const Test = Mongoose.model( 'test_bulkWrite', Schema2, clName );
   try
   {
      const hanldResult = Test.bulkWrite(
         [
            { // clean, then test
               insertOne:
               {
                  "document": { '_id': 1 }
               }
            },
            {
               deleteMany:
               {
                  "filter": {}
               }
            },
            {
               insertOne:
               {
                  "document": { '_id': 1, 'a': 1, 'b': 1, 'c': 1 }
               }
            },
            {
               insertOne:
               {
                  "document": { '_id': 2, 'a': 1, 'b': 1, 'c': 1 }
               }
            },
            {
               updateOne:
               {
                  "filter": { "a": 1 },
                  "update": { "b": 2 }
               }
            },
            {
               updateMany:
               {
                  "filter": { "a": 1 },
                  "update": { "$inc": { "c": 1 } }
               }
            },
            {
               deleteOne:
               {
                  "filter": { "a": 1 }
               }
            }
         ] ).then( hanldResult );
   }
   finally
   {
      // check results
      Test.find( {},
         function( err, rc )
         {
            if( err )
            {
               throw new Error( err );
            }
            else
            {
               Assert.equal( objectKeysSort( rc ), objectKeysSort( [{ "_id": "1", "a": 3 }] ) );
            }
            console.log( "---test_bulkWrite success" );
         }
      );
   }
}
*/


/*
* @parameter object bson( {...} ) or array [{...}, {...}]
* @return string of new object
*/
function objectKeysSort ( object )
{
   // 返回的记录keys有隐藏字段，需要先转string，再转bson
   var object = JSON.parse( JSON.stringify( object ) );
   // sort
   var objLen = object.length;
   if( objLen > 0 )
   {
      // array
      var newObjArr = [];
      for( var j = 0; j < objLen; j++ )
      {
         var obj = object[j];
         var newKeys = Object.keys( obj ).sort();
         var newObj = {};
         for( var i = 0; i < newKeys.length; i++ )
         {
            newObj[newKeys[i]] = obj[newKeys[i]]
         }
         newObjArr.push( newObj );
      }
      return JSON.stringify( newObjArr );
   }
   else
   {
      // bson
      var newKeys = Object.keys( object ).sort();
      var newObj = {};
      for( var i = 0; i < newKeys.length; i++ )
      {
         newObj[newKeys[i]] = object[newKeys[i]]
      }
      return JSON.stringify( newObj );
   }
}

function checkLargeData ( actDocs, expDocs )
{
   // 检查长度，无论长度是否相等均遍历记录正确性
   if( actDocs.length != expDocs.length )
   {
      console.log( "------actDocs.length = " + actDocs.length + ", expDocs.length = " + expDocs.length );

   }
   // 检查记录正确性
   // 返回的记录keys有隐藏字段，需要先转string，再转bson
   var actDocs = JSON.parse( JSON.stringify( actDocs ) );
   var expDocs = JSON.parse( JSON.stringify( expDocs ) );
   // 数组元素排序
   var actDocsNew = arraySort( actDocs );
   var expDocsNew = arraySort( expDocs );
   // 遍历检查每条记录，不符合预期则抛出异常
   for( var i = 0; i < actDocs.length; i++ )
   {
      if( JSON.stringify( actDocsNew[i] ) != JSON.stringify( expDocsNew[i] ) )
      {
         console.log( "------actDoc = " + JSON.stringify( actDocsNew[i] ) + ", " + "expDoc = " + JSON.stringify( expDocsNew[i] ) );
         throw new Error( "------checkLargeData failed" );
      }
   }
   // 如果实际记录数少于预期记录数，输出多余预期记录并抛出异常
   if( actDocs.length < expDocs.length )
   {
      console.log( "------expDocs, last n docs = " + JSON.stringify( expDocsNew.slice( actDocsNew.length - 1, expDocsNew.length - 1 ) ) );
      throw new Error( "------checkLargeData failed" );
   }
}

function arraySort ( docs )
{
   // 返回的记录keys有隐藏字段，需要先转string，再转bson
   var object = JSON.parse( JSON.stringify( docs ) );
   // sort
   var objLen = docs.length;
   // array
   var newDocs = [];
   for( var j = 0; j < objLen; j++ )
   {
      var obj = docs[j];
      var newKeys = Object.keys( obj ).sort();
      var newObj = {};
      for( var i = 0; i < newKeys.length; i++ )
      {
         newObj[newKeys[i]] = obj[newKeys[i]]
      }
      newDocs.push( newObj );
   }
   return newDocs;
}
