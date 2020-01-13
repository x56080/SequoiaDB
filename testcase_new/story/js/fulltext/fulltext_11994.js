/***************************************************************************
@Description :seqDB-11994 :删除不存在的全文索引 
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

   var clName = COMMCLNAME + "_ES_11994";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   //删除存在的全文索引，删除成功
   var indexName = "a_11994";
   commCreateIndex( dbcl, indexName, { content: "text" } );
   commCheckIndexConsistency( dbcl, indexName, true );
   var dbOperator = new DBOperator();
   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, indexName );
   dbcl.dropIndex( indexName );

   //删除不存在的全文索引，删除失败
   commCheckIndexConsistency( dbcl, indexName, false );
   try
   {
      dbcl.dropIndex( indexName );
      throw new Error( "DROPINDEXERR" );
   }
   catch( e )
   {
      if( e.message != -47 )
      {
         throw e;
      }
   }
   commCheckIndexConsistency( dbcl, indexName, false );
   checkIndexNotExistInES( esIndexNames );

   commDropCL( db, COMMCSNAME, clName, true, true );
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
