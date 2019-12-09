// create cs.
// CSname's large is 127.
TESTCSNAMGE = CHANGEDPREFIX + "foo";
var tmpLen = TESTCSNAMGE.length;
TESTCLNAMGE = CHANGEDPREFIX + "bar";
for( var i = 0; i < ( 127 - tmpLen ); ++i )
{
   TESTCSNAMGE = TESTCSNAMGE + "a";
}

var res = false;
try
{
   db.createCS( TESTCSNAMGE );
}
catch( e )
{
   throw e;
}
try
{
   db.dropCS( TESTCSNAMGE );
}
catch( e )
{
   println( "failed to clear the 127B CS, rc =" + e );
   throw e;
}



