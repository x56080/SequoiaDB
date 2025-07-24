/******************************************************************************
 * @Description   : seqDB-23727 : 如果索引中的字段名存在包含关系,查询数据
 * @Author        : Yi Pan
 * @CreateTime    : 2021.03.23
<<<<<<< HEAD
 * @LastEditTime  : 2021.03.29
 * @LastEditors   : Yi Pan
=======
 * @LastEditTime  : 2021.05.11
 * @LastEditors   : XiaoNi Huang
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
 ******************************************************************************/
testConf.clName = CHANGEDPREFIX + "cl_23727";

main( test );
function test ( testPara )
{
<<<<<<< HEAD
=======
   var rgName = commGetCLGroups( db, COMMCSNAME + "." + testConf.clName )[0];
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
   var cl = testPara.testCL;

   //插入数据
   cl.insert( { "a": "x", "ab": "x" } );
   cl.insert( { "a": "f", "ab": "f" } );

   //创建索引
<<<<<<< HEAD
   cl.createIndex( "idx", { "a": 1, "ab": 1, "ac": 1 }, { "NotArray": true } );

   //查询结果
   var act = cl.find( { "ac": { "$isnull": 1 } }, { "a": null, "ab": null, "ac": null } ).sort( { "a": 1, "ab": 1, "ac": 1 } ).hint( { "": "idx" } );

=======
   var idxName = "idx";
   cl.createIndex( idxName, { "a": 1, "ab": 1, "ac": 1 }, { "NotArray": true } );

   try
   {
      // 普通部署模式
      testIndexCover( cl );

      // SEQUOIADBMAINSTREAM-6926，RR隔离级别覆盖索引走的内部流程不同，需要跑此场景用例
      if( getConfig( rgName, "mvccon" ) === "TRUE" )
      {
         db.updateConf( { "transisolation": 3 } );
         testIndexCover( cl );
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

function testIndexCover ( cl )
{
   //查询结果
   var act = cl.find( { "ac": { "$isnull": 1 } }, { "a": null, "ab": null, "ac": null } ).sort( { "a": 1, "ab": 1, "ac": 1 } ).hint( { "": "idx" } );
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
   //比较结果
   var exp = [{ a: 'f', ab: 'f', ac: null }, { a: 'x', ab: 'x', ac: null }];
   commCompareResults( act, exp );
}