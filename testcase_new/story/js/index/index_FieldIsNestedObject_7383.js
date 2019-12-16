/*************************************************************************
@Description : Creating index by use nest object as the key .
@Modify list :
               2014-5-18  xiaojun Hu  Modify
               2016-2-27  yan wu Modify:删除函数inspecIndex( indexName , indexKey , keyValue , idxUnique , idxEnforced )，
                                        更新为调用公共函数inspecIndex；
                                        增加结果检测（查看访问计划是否走索引、走索引查询数据是否正确）
*************************************************************************/
try
{
   commDropCL( db, csName, clName, true, true, "drop cl in the beginning" );
}
catch( e )
{
   println( "unexpected err happened when clear cs:" + e );
   throw e;
}

try
{
   var optionObj = { ReplSize: 0, Compressed: true };
   var varCL = commCreateCL( db, csName, clName, optionObj, true,
      false, "create collecton 1 failed" );
}
catch( e )
{
   println( "Failed to create CS and CL, rc=" + e );
   throw e;
}

try
{
   varCL.createIndex( "testindex", { "use.id": 1 }, true );
   inspecIndex( varCL, "testindex", "use.id", 1, true, false );
}
catch( e )
{
   println( "failed to create index, rc=" + e );
   throw e;
}

try
{
   varCL.insert( { use: { id: 1, name: "chen" } } );
   varCL.insert( { use: { id: 2, name: "chen" } } );
}
catch( e )
{
   println( "failed to insert record, rc=" + e );
   throw e;
}

//test find by index
checkExplain( varCL, { "use.id": 1 } );

//check the result of find  
var rc = varCL.find( { "use.id": 1 } );
var expRecs = [];
expRecs.push( { use: { id: 1, name: "chen" } } );
checkRec( rc, expRecs );

try
{
   varCL.insert( { use: { id: 1, name: "chensdf" } } );
}
catch( e )
{
   if( -38 != e )
   {
      println( "Failed to insert data to database, rc=" + e );
      throw e;
   }
}

try
{
   commDropCL( db, csName, clName, false, false,
      "drop colleciton in the end" );
}
catch( e )
{
   println( "unexpected err happened when clear cs:" + e );
   throw e;
}

