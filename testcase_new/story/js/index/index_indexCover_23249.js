/******************************************************************************
 * @Description   : seqDB-23249:索引不支持数组，非嵌套对象+复合索引，查询条/选择/排序字段均为多索引键字段
 * @Author        : Xiaoni Huang
 * @CreateTime    : 2021.01.09
 * @LastEditTime  : 2021.05.08
 * @LastEditors   : XiaoNi Huang
 ******************************************************************************/
testConf.clName = CHANGEDPREFIX + "_cl_23249";
testConf.clOpt = { "ReplSize": -1 };

main( test );
function test ( testPara )
{
   var rgName = commGetCLGroups( db, COMMCSNAME + "." + testConf.clName )[0];
   var cl = testPara.testCL;

   try
   {
      // 普通部署模式
      testIndexCover( cl, testConf.clName );

      // SEQUOIADBMAINSTREAM-6926，RR隔离级别覆盖索引走的内部流程不同，需要跑此场景用例
      if( getConfig( rgName, "mvccon" ) === "TRUE" )
      {
         db.updateConf( { "transisolation": 3 } );
         testIndexCover( cl, testConf.clName );
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

function testIndexCover ( cl, clName )
{
   // 创建索引   
   var idxName = "idx";
   cl.createIndex( idxName, { "a": 1, "b": 1, "c": 1 }, { "NotArray": true } )

   // 插入数据，recordsNum不能小于1000（查询条件限制）
   var recordsNum = 2000;
   var records = new Array();
   for( var i = 0; i < recordsNum; i++ )
   {
      records.push( { "a": i, "b": i, "c": ( ( recordsNum - 1 ) - i ), "d": i } );
   }
   cl.insert( records );

   // 1）查询条件、选择条件、排序字段均为同一多索引字段，选择条件非object格式
   // 开启覆盖索引开关
   db.updateConf( { "indexcoveron": true } );

   // 检查索引访问不回表
   var cond = { "a": { "$gt": 100 }, "c": { "$lt": 1000 } };
   var sel = { "a": "", "b": "", "c": "" };
   var sortCond = { "a": 1, "c": 1 };
   var hint = { "": idxName };
   db.analyze( { "Mode": 5, "Collection": COMMCSNAME + "." + clName } );
   var explainInfo = cl.find( cond, sel ).sort( sortCond ).hint( hint ).explain().toArray();
   assert.equal( JSON.parse( explainInfo[0] ).IndexCover, true, "explainInfo = " + explainInfo );

   // 检查查询数据正确性（开启覆盖索引和不开启覆盖索引相同查询语句结果做对比）
   var obj1 = cl.find( cond, sel ).sort( sortCond ).hint( hint ).toArray();

   db.updateConf( { "indexcoveron": false } );
   var obj2 = cl.find( cond, sel ).sort( sortCond ).hint( hint ).toArray();

   commCompareObject( obj1, obj2 );

   // 2）查询条件、选择条件为不同的多索引字段，且选择条件索引字段包含排序索引字段，选择条件非object格式； 其他条件均满足覆盖索引条件
   // 开启覆盖索引开关
   db.updateConf( { "indexcoveron": true } );

   // 检查索引访问不回表
   var cond = { "a": { "$gt": 100 }, "b": { "$field": "a" } };
   var sel = { "a": "", "b": "", "c": "" };
   var sortCond = { "b": 1, "c": 1 };
   var hint = { "": idxName };
   db.analyze( { "Mode": 5, "Collection": COMMCSNAME + "." + clName } );
   var explainInfo = cl.find( cond, sel ).sort( sortCond ).hint( hint ).explain().toArray();
   assert.equal( JSON.parse( explainInfo[0] ).IndexCover, true, "explainInfo = " + explainInfo );

   // 检查查询数据正确性（开启覆盖索引和不开启覆盖索引相同查询语句结果做对比）
   var obj1 = cl.find( cond, sel ).sort( sortCond ).hint( hint ).toArray();

   db.updateConf( { "indexcoveron": false } );
   var obj2 = cl.find( cond, sel ).sort( sortCond ).hint( hint ).toArray();

   commCompareObject( obj1, obj2 );

   cl.dropIndex( idxName );
   cl.remove();
}