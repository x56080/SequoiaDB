/*****************************************************************************
@Description : Creating index by use chinese.
@Modify list :
               2014-5-18  xiaojun Hu  Modify
               2016-2-27  yan wu Modify:删除函数inspecIndex( indexName , indexKey , keyValue , idxUnique , idxEnforced )，
                                        更新为调用公共函数inspecIndex；
                                        增加结果检测（查看访问计划是否走索引、走索引查询数据是否正确）
*****************************************************************************/
try
{
   commDropCL( db, csName, clName, true, true, "drop cl in the beginning" );
}
catch( e )
{
   println( "unexpected err happened when clear cs:" + e );
   throw e;
}

   var varCL = commCreateCL( db, csName, clName );

try
{
   varCL.createIndex( "chen", { "中文": 1 }, true );
   inspecIndex( varCL, "chen", "中文", 1, true, false );
}
catch( e )
{
   println( "Failed to create index, rc=" + e );
   throw e;
}

try
{
   varCL.insert( { "中文": 12 } );
}
catch( e )
{
   println( "Failed to insert data after create index, rc=" + e );
   throw e;
}

//test find by index 
checkExplain( varCL, { "中文": 12 } );

//check the result of find  
checkResult( varCL, { "中文": 12 } );

try
{
   varCL.insert( { "中文": 12 } );
}
catch( e )
{
   if( -38 != e )
   {
      println( "Failed to insert same record to database, rc=" + e );
      throw e;
   }
}

try
{
   varCL.createIndex( "testindex", { "use.id": 1 }, true );
}
catch( e )
{
   println( "failed to create index, rc=" + e );
   throw e;
}

try
{
   commDropCL( db, csName, clName, false, false,
      "drop colleciton in the end" );
}
catch( e )
{
   println( "unexpected err happened when clear cs end:" + e );
   throw e;
}
