/***************************************************************************
@Description :seqDB-11993 :创建全文索引接口参数校验 
@Modify list :
              2018-10-25  YinZhen  Create
****************************************************************************/
function main ()
{
   if( commIsStandalone( db ) )
   {
      println( "Deploy is standalone" );
      return;
   }

   var clName = COMMCLNAME + "_ES_11993";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   //创建索引类型非法的全文索引
   var indexName = "a_11993";
   try
   {
      dbcl.createIndex( indexName, { content: "int" } );
      throw new Error( "CREATEINDEXERR" );
   }
   catch( e )
   {
      if( e.message != -6 )
      {
         throw e;
      }
   }
   commCheckIndex( dbcl, indexName, false );

   //创建非法的复合索引
   try
   {
      dbcl.createIndex( indexName, { content: "text", about: 1 } );
      throw new Error( "CREATEINDEXERR" );
   }
   catch( e )
   {
      if( e.message != -6 )
      {
         throw e;
      }
   }
   commCheckIndex( dbcl, indexName, false );

   //指定isUnique、enforced、sortBufferSize创建全文索引
   dbcl.createIndex( indexName, { content: "text" }, true, true, 128 );
   commCheckIndex( dbcl, indexName, true );

   dbcl.insert( [{ content: "a" }, { content: "a" }] );
   var dbOperator = new DBOperator();
   var actResult = dbOperator.findFromCL( dbcl, null, { content: "" } );
   var expResult = [{ content: "a" }, { content: "a" }];

   checkResult( expResult, actResult );

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, indexName );
   commDropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}
try
{
   main();
}
catch( e )
{
   if( e.constructor === Error )
   {
      println( e.stack );
   }
   throw e;
}
