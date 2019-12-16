/******************************************************************************
@Description :   seqDB-15991: 创建集合时，创建1个自增字段 
@Modify list :   2018-10-17    xiaoni Zhao  Init
******************************************************************************/
function main ()
{
   if( commIsStandalone( db ) )
   {
      println( "Deploy is standalone" );
      return;
   }

   var clName = COMMCLNAME + "_15991";
   var field = "id1";

   commDropCL( db, COMMCSNAME, clName );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: "id1" } } );

   var clID = getCLID( COMMCSNAME, clName );
   var sequenceName = "SYS_" + clID + "_" + field + "_SEQ";
   var expArr = [{ Field: field, SequenceName: sequenceName }];
   checkAutoIncrementonCL( COMMCSNAME, clName, expArr );
   checkSequence( sequenceName, {} );

   dbcl.insert( { a: 1 } );

   var rc = dbcl.find();
   var expRecs = [{ "id1": 1, "a": 1 }];
   checkRec( rc, expRecs );

   commDropCL( db, COMMCSNAME, clName );
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
