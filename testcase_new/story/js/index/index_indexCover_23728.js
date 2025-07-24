/******************************************************************************
 * @Description   : seqDB-23728 : 记录dmsRecord头中有Ovf标记时会assert
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
testConf.clName = CHANGEDPREFIX + "_cl23728";

main( test );
function test ( testPara )
{
<<<<<<< HEAD
=======
   var rgName = commGetCLGroups( db, COMMCSNAME + "." + testConf.clName )[0];
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
   var cl = testPara.testCL;

   //插入数据
   cl.insert( { "_id": { "$oid": "6048721debc199adadc80583" } } );

   //更新数据
   cl.update( { "$set": { "id": 0 } }, { "id": { "$isnull": 1 } } );

   //创建索引
<<<<<<< HEAD
   cl.createIndex( "PRIMARY", { "id": 1 }, { "Unique": true, "NotNull": true, "NotArray": true } );

   //查询结果
   cl.find( {}, { "id": null } ).sort( { "id": 1 } ).hint( { "": "PRIMARY" } );

=======
   var idxName = "PRIMARY";
   cl.createIndex( idxName, { "id": 1 }, { "Unique": true, "NotNull": true, "NotArray": true } );

   try
   {
      // 普通部署模式
      testIndexCover( cl, idxName );

      // SEQUOIADBMAINSTREAM-6926，RR隔离级别覆盖索引走的内部流程不同，需要跑此场景用例
      if( getConfig( rgName, "mvccon" ) === "TRUE" )
      {
         db.updateConf( { "transisolation": 3 } );
         testIndexCover( cl, idxName );
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

function testIndexCover ( cl, idxName )
{
   //查询结果
   var act = cl.find( {}, { "id": null } ).sort( { "id": 1 } ).hint( { "": idxName } );
   //比较结果
   var exp = [{ "id": 0 }];
   commCompareResults( act, exp );
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
}