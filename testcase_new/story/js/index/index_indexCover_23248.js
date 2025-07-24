/******************************************************************************
 * @Description   : seqDB-23248 :: 索引不支持数组，复合索引，查询条/选择/排序字段均为单索引键字段 
 * @Author        : Yu Fan
 * @CreateTime    : 2021.01.09
<<<<<<< HEAD
 * @LastEditTime  : 2021.01.11
 * @LastEditors   : Yu Fan
=======
 * @LastEditTime  : 2021.05.11
 * @LastEditors   : XiaoNi Huang
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_23248";
var indexName = "Index_23248";

main( test );
function test ( testPara )
{
   db.updateConf( { indexcoveron: true } );
<<<<<<< HEAD
=======
   var rgName = commGetCLGroups( db, COMMCSNAME + "." + testConf.clName )[0];
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
   var cl = testPara.testCL;
   // 创建索引
   cl.createIndex( indexName, { a: 1, b: 1, c: 1 }, { NotArray: true } )

   // 插入数据
   var records = new Array();
   for( var i = 0; i < 10; i++ )
   {
      records.push( { a: i, b: i, c: i, d: i } );
   }
   cl.insert( records )

<<<<<<< HEAD
=======
   try
   {
      // 普通部署模式
      testIndexCover( cl, records );

      // SEQUOIADBMAINSTREAM-6926，RR隔离级别覆盖索引走的内部流程不同，需要跑此场景用例
      if( getConfig( rgName, "mvccon" ) === "TRUE" )
      {
         db.updateConf( { "transisolation": 3 } );
         testIndexCover( cl, records );
      }
   }
   finally
   {
      db.updateConf( { "transisolation": 0 } );
      var transisolation = getConfig( rgName, "transisolation" );
      if( transisolation !== 0 )
      {
         throw new Error( "Expect transisolation: " + "0, actual transisolation: " + transisolation );
      }
   }
}

function testIndexCover ( cl, records )
{
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
   // 查询条件、选择条件、排序字段均为同一单索引字段（首/中间/末尾索引字段），选择条件非object格式
   var cursor = cl.find( { a: { $gt: 7 } }, { a: "" } ).sort( { a: 1 } ).hint( { "": indexName } );
   commCompareResults( cursor, [{ a: 8 }, { a: 9 }] );
   var explainInfo = cl.find( { a: { $gt: 7 } }, { a: "" } ).sort( { a: 1 } ).hint( { "": indexName } ).explain().toArray();
   assert.equal( JSON.parse( explainInfo[0] ).IndexCover, true, "explainInfo = " + explainInfo );

   // 查询条件、选择条件为不同的单索引字段，且选择条件索引字段包含/等于排序索引字段，选择条件非object格式； 其他条件均满足覆盖索引条件；
   var cursor = cl.find( { c: { $gt: 7 } }, { a: "", b: "" } ).sort( { a: 1 } ).hint( { "": indexName } );
   commCompareResults( cursor, [{ a: 8, b: 8 }, { a: 9, b: 9 }] );
   var explainInfo = cl.find( { c: { $gt: 7 } }, { a: "", b: "" } ).sort( { a: 1 } ).hint( { "": indexName } ).explain().toArray();
   assert.equal( JSON.parse( explainInfo[0] ).IndexCover, true, "explainInfo = " + explainInfo );
}

