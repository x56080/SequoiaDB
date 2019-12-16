// drop collection.
// normal case.
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
try
{
   var optionObj = { ReplSize: 0, Compressed: true };
   var varCL = commCreateCL( db, COMMCSNAME, COMMCLNAME, optionObj, true,
      false, "create collecton 1 failed" );
}
catch( e )
{
   println( "failed to create cl, rc= " + e );
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
   varCS.dropCL( COMMCLNAME );
}
catch( e )
{
   println( "failed to drop cl, rc= " + e );
   throw e;
}


var found = true;
try
{
   varCS.getCL( COMMCLNAME );
}
catch( e )
{
   if( -23 != e )
   {
      println( "unexpected err, rc= " + e );
      throw e;
   }
   else
   {
      found = false;
   }
}

if( found )
{
   println( "still can get cl after drop" );
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


