// insert record.
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
   var varCL = varCS.createCL( COMMCLNAME, { ReplSize: 0, Compressed: true } );
} catch( e )
{
   throw e;
}
var veryBigJsonString = "{\"a0\":0";
var i = 0;
for( i = 0; i < 1000; i++ )
{
   veryBigJsonString += ",\"a" + i + "\":" + i;
}
veryBigJsonString += "}";

var veryBigJson = eval( '(' + veryBigJsonString + ')' );

try
{
   varCL.insert( veryBigJson );
}
catch( e )
{
   println( "failed to insert record, rc= " + e );
   throw e;
}

try
{
   if( varCL.find().count() != 1 )
      throw "number error"
   if( !compareObj( veryBigJson, varCL.find().next().toObj() ) )
      throw "compare verify failed"
}
catch( e )
{
   throw e;
}

try
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
      "drop colleciton in the end" );
}
catch( e )
{
   println( "failed to drop cs, rc= " + e );
   throw e;
}
