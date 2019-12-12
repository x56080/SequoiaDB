/******************************************************************************
@Description :   seqDB-12011:在_id字段上创建全文索引
@Modify list :   2018-10-10  xiaoni Zhao  Init
******************************************************************************/
function main ()
{

   if( commIsStandalone( db ) )
   {
      println( "Run mode is standalone" );
      return;
   }

   var clName = COMMCLNAME + "_ES_12011";
   var textIndexName = "a_12011";

   commDropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   createIndexOnId( dbcl, textIndexName );

   createIndexContainId( dbcl, textIndexName );

   commDropCL( db, COMMCSNAME, clName, true, true );
}

function createIndexOnId ( dbcl, textIndexName )
{
   try
   {
      dbcl.createIndex( textIndexName, { _id: "text" } );
      throw new Error( 'create text index on _id should fail!' );
   } catch( e )
   {
      if( e.message != -6 )
      {
         throw e;
      }
   }
}

function createIndexContainId ( dbcl, textIndexName )
{
   try
   {
      dbcl.createIndex( textIndexName, { _id: "text", a: "text" } );
      throw new Error( 'create text index include _id should fail!' );
   } catch( e )
   {
      if( e.message != -6 )
      {
         throw e;
      }
   }
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
;
