/***************************************************************************
@Description :seqDB-11985 :创建重复的全文索引 
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

   var clName = COMMCLNAME + "_ES_11985";
   dropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   //创建索引名已存在的全文索引
   var indexName = "a_11985";
   dbcl.createIndex( indexName, { about: 1 } );
   commCheckIndexConsistency( dbcl, indexName, true );
   try
   {
      dbcl.createIndex( indexName, { content: "text" } );
      throw new Error( "CREATEINDEXERR" );
   }
   catch( e )
   {
      if( e.message != -46 )
      {
         throw e;
      }
   }

   //在已存在全文索引定义的集合中，再次创建全文索引
   dbcl.createIndex( "b_11985", { content: "text" } );
   commCheckIndexConsistency( dbcl, "b_11985", true );
   try
   {
      dbcl.createIndex( "c_11985", { content: "text" } );
      throw new Error( "CREATEINDEXERR" );
   }
   catch( e )
   {
      if( e.message != -42 )
      {
         throw e;
      }
   }

   var dbOperator = new DBOperator();
   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, "b_11985" );
   dropCL( db, COMMCSNAME, clName, true, true );
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
