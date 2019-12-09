//drop collection
//innomal case3
try
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop cl in the beginning" );
}
catch( e )
{
   println( "unexpected err happened when clear cs:" + e );
   throw e;
}
try
{
   var varCS = commCreateCS( db, COMMCSNAME, true, "create CS in the beginning" );
}
catch( e )
{
   println( "failed to create cs, rc=" + e );
   throw e;
}

var res = false;
try
{
   varCS.dropCL( "" );
}
catch( e )
{
   if( e == -6 )
   {
      res = true;
   }
}
if( !res )
{
   throw -1;
}

try
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
      "drop colleciton in the end" );
}
catch( e )
{
   println( "unexpected err happened when clear cs:" + e );
   throw e;
}
