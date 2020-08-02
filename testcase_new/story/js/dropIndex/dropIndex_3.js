/************************************************************************
@Description : drop the index "$id".Cannot drop .
@Modify list :
2014-5-16  xiaojun Hu  modify
************************************************************************/
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
   println( "Failed to create CS and CL, rc= " + e );
   throw e;
}

try
{
   varCL.dropIndex( "$id" );
}
catch( e )
{
   if( e != -56 )
   {
      println( "Failed to drop index, rc= " + e );
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
   println( "failed to clear cs:" + e );
   throw e;
}
