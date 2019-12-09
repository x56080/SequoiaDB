// create cl.
// normal case.

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
   println( "failed to create cs, rc= " + e );
   throw e;
}

for( var i = 0; i < ( 128 - CHANGEDPREFIX.length ); ++i )
{
   TESTCLNAME = TESTCLNAME + "a";
}
var j = true;
try
{
   var varCL = varCS.createCL( TESTCLNAME );
}
catch( e )
{
   j = false;
}
if( j )
{
   throw -1;
}

