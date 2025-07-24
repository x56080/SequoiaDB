/******************************************************************************
 * @Description   : seqDB-23247 :: 索引不支持数组，查询条件/选择条件为{} 
 * @Author        : Yu Fan
 * @CreateTime    : 2021.01.09
<<<<<<< HEAD
 * @LastEditTime  : 2021.10.14
=======
 * @LastEditTime  : 2021.10.15
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
 * @LastEditors   : XiaoNi Huang
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_23247";
var indexName = "Index_23247";

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
   cl.createIndex( indexName, { a: 1 }, { NotArray: true } )

   // 插入数据
   var records = new Array();
   for( var i = 0; i < 10; i++ )
   {
      records.push( { a: i } );
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
   // 查询条件为{}
   var cursor = cl.find( {}, { a: "" } ).sort( { a: 1 } ).hint( { "": indexName } );
   commCompareResults( cursor, records );
   var explainInfo = cl.find( {}, { a: "" } ).sort( { a: 1 } ).hint( { "": indexName } ).explain().toArray();
   assert.equal( JSON.parse( explainInfo[0] ).IndexCover, true, "explainInfo = " + explainInfo );

   // 选择条件为{}
   var cursor = cl.find( { a: 1 }, {} );
   commCompareResults( cursor, [{ a: 1 }] );
   var explainInfo = cl.find( { a: 1 }, {} ).explain().toArray();
   assert.equal( JSON.parse( explainInfo[0] ).IndexCover, false, "explainInfo = " + explainInfo );
}

