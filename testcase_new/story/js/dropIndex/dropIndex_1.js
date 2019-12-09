/*********************************************************************************
@Description : create the index first, and then drop the index normal.
@Modify list :
2014-5-16  xiaojun Hu Modify
*********************************************************************************/
try
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop cl in the beginning" );
}
catch( e )
{
   println( "Failed to clear the collectionspace first :" + e );
   throw e;
}

try
{
   var optionObj = { ReplSize: 0, Compressed: true };
   var varCL = commCreateCLByOption( db, COMMCSNAME, COMMCLNAME, optionObj, true,
      false, "create collecton 1 failed" );
}
catch( e )
{
   println( "Failed to create CS and CL :" + e );
   throw e;
}

//create index
try
{
   varCL.createIndex( "testindex", { a: 1 }, false );
   inspecIndex( varCL, "testindex", "a", 1, false, false );
}
catch( e )
{
   println( "failed to create index, rc= " + e );
   throw e;
}

try
{
   varCL.dropIndex( "testindex" );
}
catch( e )
{
   println( "failed to drop index, rc= " + e );
   throw e;
}

commCheckIndex( varCL, "testindex", false );

try
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, false, false,
      "drop colleciton in the end" );
}
catch( e )
{
   println( "Failed to drop CS in the end :" + e );
   throw e;
}

