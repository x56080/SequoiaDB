// create cl.
// normal case.

TESTCSNAME = CHANGEDPREFIX + "foo";

try
{
   commDropCS( db, TESTCSNAME, true, "drop collecion space in the beginning" );
}
catch( e )
{
   println( "unexpected err happened when clear cs:" + e );
   throw e;
}

try
{
   var varCS = commCreateCS( db, TESTCSNAME, true, "failed to create CS" );
}
catch( e )
{
   println( "failed to create cs, rc= " + e );
   throw e;
}

var j = 0;
var aa = Array( ";", ":", "\'", "\"", "{", "}", "[", "]", ",", "<", ">", "?", "/", "|", "\\", "+", "=", "-", "_", "~", "`", "!", "@", "#", "$", "%", "^", "&", "*", "(", ")" );
for( var i = 0; i < aa.length; ++i )
{
   try
   {
      var CLname = CHANGEDPREFIX + aa[i];
      var varCL = varCS.createCL( CLname );
   }
   catch( e )
   {
      println( "collection name: " + CLname );
      println( "failed to create cl, rc = " + e );
      throw e;
   }
}


try
{
   var rc = varCS.getCL( CHANGEDPREFIX + aa[3] );
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
   commDropCS( db, TESTCSNAME, true, "drop collecion space in the beginning" );
}
catch( e )
{
   println( "unexpected err happened when clear cs:" + e );
   throw e;
}

