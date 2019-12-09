////create exist collection case2

TESTCLNAME = CHANGEDPREFIX + "bar";

try
{
   commDropCL( db, COMMCSNAME, TESTCLNAME, true, true, "drop CL in the beginning" );
}
catch( e )
{
   if( e != -34 )
   {
      println( "unexpected err happened when clear cs:" + e );
      throw e;
   }
}

try
{
   var varCS = commCreateCS( db, COMMCSNAME, true, "failed to create CS" );
}
catch( e )
{
   println( "failed to create cs,rc=" + e );
   throw e;
}

try
{
   var varCL = varCS.createCL( TESTCLNAME, { Compressed: true } );
}
catch( e )
{
   println( "failed to create cl, rc= " + e );
   throw e;
}

try
{
   varCS.dropCL( TESTCLNAME );
}
catch( e )
{
   println( "failed to drop cl,rc=" + e );
   throw e;
}

try
{
   varCS.createCL( TESTCLNAME, { Compressed: true } );
}
catch( e )
{
   println( "failed to create cl, rc= " + e );
   throw e;
}

try
{
   commDropCL( db, COMMCSNAME, TESTCLNAME, false, false, "drop CL in the end" );
}
catch( e )
{
   println( "unexpected err happened when clear cs:" + e );
   throw e;
}
