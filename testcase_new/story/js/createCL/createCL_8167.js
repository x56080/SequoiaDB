// create cl.
// normal case.

TESTCLNAME = CHANGEDPREFIX + "foo";

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
   println( "failed to create cs, rc= " + e );
   throw e;
}

for( var i = 0; i < ( 127 - TESTCLNAME.length ); ++i )
{
   TESTCLNAME = TESTCLNAME + "a";
}

try
{
   var varCL = varCS.createCL( TESTCLNAME );
}
catch( e )
{
   println( "failed to create cl, rc= " + e );
   throw e;
}

try
{
   var rc = varCS.getCL( TESTCLNAME );
}
catch( e )
{
   println( "failed to get cl, rc= " + e );
   throw e;
}

try
{
   varCL.insert( { a: 1 } );
}
catch( e )
{
   println( "failed to insert record to cl, rc= " + e );
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
