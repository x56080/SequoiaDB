/*****************************************************************************
@Description : Creating index by use chinese.
@Modify list :
               2014-5-18  xiaojun Hu  Modify
               2016-2-27  yan wu Modify:删除函数inspecIndex( indexName , indexKey , keyValue , idxUnique , idxEnforced )，
                                        更新为调用公共函数inspecIndex；
                                        增加结果检测（查看访问计划是否走索引、走索引查询数据是否正确）
*****************************************************************************/

main( test );

function test ()
{
   commDropCL( db, csName, clName, true, true, "drop cl in the beginning" );

   var varCL = commCreateCL( db, csName, clName );

   varCL.createIndex( "chen", { "中文": 1 }, true );
   inspecIndex( varCL, "chen", "中文", 1, true, false );
   varCL.insert( { "中文": 12 } );

   //test find by index 
   checkExplain( varCL, { "中文": 12 } );

   //check the result of find  
   checkResult( varCL, { "中文": 12 } );

   assert.tryThrow( -38, function()
   {
      varCL.insert( { "中文": 12 } );
   } );

   varCL.createIndex( "testindex", { "use.id": 1 }, true );
   commDropCL( db, csName, clName, false, false );
}
