/******************************************************************************
 * @Description   : seqDB-23250:索引不支持数组，非嵌套对象+复合索引，选择条件索引字段不包含排序索引字段
 * @Author        : Xiaoni Huang
 * @CreateTime    : 2021.01.09
<<<<<<< HEAD
 * @LastEditTime  : 2021.10.14
=======
 * @LastEditTime  : 2021.10.15
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
 * @LastEditors   : XiaoNi Huang
 ******************************************************************************/
testConf.clName = CHANGEDPREFIX + "_cl_23250";
testConf.clOpt = { "ReplSize": -1 };

main( test );
<<<<<<< HEAD
function test ( testPara )
{
   var cl = testPara.testCL;
=======

function test ( testPara )
{
   var rgName = commGetCLGroups( db, COMMCSNAME + "." + testConf.clName )[0];
   var cl = testPara.testCL;

>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
   // 创建索引   
   var idxName = "idx";
   cl.createIndex( idxName, { "a": 1, "b": 1, "c": 1, "d": 1 }, { "NotArray": true } )

   // 插入数据，recordsNum不能小于1000（查询条件限制）
   var recordsNum = 2000;
   var records = new Array();
   for( var i = 0; i < recordsNum; i++ )
   {
      records.push( { "a": i, "b": i, "c": ( ( recordsNum - 1 ) - i ), "d": i } );
   }
   cl.insert( records );

<<<<<<< HEAD
   // 1）不等于（如：选择字段为a，排序字段为b）
   try
   {
      // 开启覆盖索引开关
      db.updateConf( { "indexcoveron": true } );

      // 检查索引访问回表
      var cond = { "a": { "$gt": 100 }, "c": { "$lt": 1000 } };
      var sel = { "a": "", "b": "" };
      var sortCond = { "c": 1, "d": 1 };
      var hint = { "": idxName };
      db.analyze( { "Mode": 5, "Collection": COMMCSNAME + "." + testConf.clName } );
      var explainInfo = cl.find( cond, sel ).sort( sortCond ).hint( hint ).explain().toArray();
      assert.equal( JSON.parse( explainInfo[0] ).IndexCover, true, "explainInfo = " + explainInfo );

      // 检查查询数据正确性（开启覆盖索引和不开启覆盖索引相同查询语句结果做对比）
      var cursor1 = cl.find( cond, sel ).sort( sortCond ).hint( hint );
      db.updateConf( { "indexcoveron": false } );
      var cursor2 = cl.find( cond, sel ).sort( sortCond ).hint( hint );
      commCompareResults( cursor2, throughCursor( cursor1 ) );
   }
   finally
   {
      db.updateConf( { "indexcoveron": true } );
   }


   // 2）小于（如：选择字段为a，排序字段为a+b））
   try
   {
      // 开启覆盖索引开关
      db.updateConf( { "indexcoveron": true } );

      // 检查索引访问回表
      var cond = { "a": { "$gt": 100 }, "b": { "$field": "a" } };
      var sel = { "a": "", "b": "" };
      var sortCond = { "a": 1, "b": 1, "c": 1 };
      var hint = { "": idxName };
      db.analyze( { "Mode": 5, "Collection": COMMCSNAME + "." + testConf.clName } );
      var explainInfo = cl.find( cond, sel ).sort( sortCond ).hint( hint ).explain().toArray();
      assert.equal( JSON.parse( explainInfo[0] ).IndexCover, true, "explainInfo = " + explainInfo );

      // 检查查询数据正确性（开启覆盖索引和不开启覆盖索引相同查询语句结果做对比）
      var cursor1 = cl.find( cond, sel ).sort( sortCond ).hint( hint );
      db.updateConf( { "indexcoveron": false } );
      var cursor2 = cl.find( cond, sel ).sort( sortCond ).hint( hint );
      commCompareResults( cursor2, throughCursor( cursor1 ) );
   }
   finally
   {
      db.updateConf( { "indexcoveron": true } );
   }
=======
   try
   {
      // 普通部署模式
      testIndexCover( cl, testConf.clName, idxName );

      // SEQUOIADBMAINSTREAM-6926，RR隔离级别覆盖索引走的内部流程不同，需要跑此场景用例
      if( getConfig( rgName, "mvccon" ) === "TRUE" )
      {
         db.updateConf( { "transisolation": 3 } );
         testIndexCover( cl, testConf.clName, idxName );
      }
   }
   finally
   {
      // 恢复环境
      db.updateConf( { "indexcoveron": true } );
      var indexcoveron = getConfig( rgName, "indexcoveron" );
      if( indexcoveron !== "TRUE" )
      {
         throw new Error( "Expect indexcoveron: " + "TRUE, actual indexcoveron: " + indexcoveron );
      }

      db.updateConf( { "transisolation": 0 } );
      var transisolation = getConfig( rgName, "transisolation" );
      if( transisolation !== 0 )
      {
         throw new Error( "Expect transisolation: " + "0, actual transisolation: " + transisolation );
      }
   }
}

function testIndexCover ( cl, clName, idxName )
{
   // 1）不等于（如：选择字段为a，排序字段为b）
   // 开启覆盖索引开关
   db.updateConf( { "indexcoveron": true } );

   // 检查索引访问不回表，即走覆盖索引
   var cond = { "a": { "$gt": 100 }, "c": { "$lt": 1000 } };
   var sel = { "a": "", "b": "" };
   var sortCond = { "c": 1, "d": 1 };
   var hint = { "": idxName };
   db.analyze( { "Mode": 5, "Collection": COMMCSNAME + "." + clName } );
   var explainInfo = cl.find( cond, sel ).sort( sortCond ).hint( hint ).explain().toArray();
   assert.equal( JSON.parse( explainInfo[0] ).IndexCover, true, "explainInfo = " + explainInfo );

   // 检查查询数据正确性（开启覆盖索引和不开启覆盖索引相同查询语句结果做对比）
   var cursor1 = cl.find( cond, sel ).sort( sortCond ).hint( hint );
   db.updateConf( { "indexcoveron": false } );
   var cursor2 = cl.find( cond, sel ).sort( sortCond ).hint( hint );
   commCompareResults( cursor2, throughCursor( cursor1 ) );


   // 2）小于（如：选择字段为a，排序字段为a+b））
   // 开启覆盖索引开关
   db.updateConf( { "indexcoveron": true } );

   // 检查索引访问不回表，即走覆盖索引
   var cond = { "a": { "$gt": 100 }, "b": { "$field": "a" } };
   var sel = { "a": "", "b": "" };
   var sortCond = { "a": 1, "b": 1, "c": 1 };
   var hint = { "": idxName };
   db.analyze( { "Mode": 5, "Collection": COMMCSNAME + "." + clName } );
   var explainInfo = cl.find( cond, sel ).sort( sortCond ).hint( hint ).explain().toArray();
   assert.equal( JSON.parse( explainInfo[0] ).IndexCover, true, "explainInfo = " + explainInfo );

   // 检查查询数据正确性（开启覆盖索引和不开启覆盖索引相同查询语句结果做对比）
   var cursor1 = cl.find( cond, sel ).sort( sortCond ).hint( hint );
   db.updateConf( { "indexcoveron": false } );
   var cursor2 = cl.find( cond, sel ).sort( sortCond ).hint( hint );
   commCompareResults( cursor2, throughCursor( cursor1 ) );
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
}

function throughCursor ( cursor )
{
   var records = new Array();
   while( cursor.next() )
   {
      var obj = cursor.current();
      records.push( cursor.current().toObj() );
   }
   return records;
}