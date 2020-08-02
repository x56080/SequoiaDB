/***************************************************************************
@Description : drop the index, but index don't exist.
@Modify list :
2014-5-16  xiaojun Hu  modify
***************************************************************************/

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
   var varCL = commCreateCL( db, COMMCSNAME, COMMCLNAME );
}
catch( e )
{
   println( "failed to create CS and CL rc= " + e );
   throw e;
}


var res = false;
try
{
   varCL.dropIndex( "testindex" );
}
catch( e )
{
   if( e != -47 )
   {
      println( "Failed to drop index :" + e );
      throw e;
   }
}

try
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, false, false,
      "drop colleciton in the end" );
}
catch( e )
{
   println( "failed to clear cs end: " + e );
   throw e;
}
