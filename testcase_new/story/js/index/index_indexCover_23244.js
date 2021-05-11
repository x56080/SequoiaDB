/******************************************************************************
 * @Description   : seqDB-23244:索引支持数组，测试覆盖索引
 * @Author        : Yi Pan
 * @CreateTime    : 2021.01.19
 * @LastEditTime  : 2021.05.08
 * @LastEditors   : XiaoNi Huang
 ******************************************************************************/

testConf.clName = COMMCLNAME + "_23244";

main( test );

function test ()
{
   var rgName = commGetCLGroups( db, COMMCSNAME + "." + testConf.clName )[0];
   var cl = testPara.testCL;

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
   //修改配置文件
   db.updateConf( { indexcoveron: true }, { Global: true } );

   //创建索引 NotArray指定为false
   var idxname = "idx23244";
   cl.createIndex( idxname, { 'age': 1 }, { NotArray: false } );

   //插入数据
   cl.insert( { 'age': 1 } );
   cl.insert( { 'age': 2 } );

   //查询数据
   var actResult = cl.find( { 'age': 1 }, { 'age': '' } ).hint( { "": idxname } );
   expResult = [{ 'age': 1 }];
   commCompareResults( actResult, expResult );
   //查询分析
   var explain = cl.find( { 'age': 1 }, { 'age': '' } ).hint( { "": idxname } ).explain();
   checkIndexCover( explain, false );

   //条件查询
   var actResult = cl.find( { 'age': { $et: 2 } }, { 'age': "" } ).hint( { "": idxname } );
   expResult = [{ 'age': 2 }];
   commCompareResults( actResult, expResult );
   //条件分析
   var explain = cl.find( { 'age': { $et: 2 } }, { 'age': "" } ).hint( { "": idxname } ).explain();
   checkIndexCover( explain, false );

   cl.dropIndex( idxname );
   cl.remove();
}