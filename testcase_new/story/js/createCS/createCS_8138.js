//creat same collection space
TESTCSNAMGE = CHANGEDPREFIX + "foo";

TESTCLNAMGE = CHANGEDPREFIX + "bar";
try
{
   db.dropCS( TESTCSNAMGE );
}
catch( e )
{

}
try
{
   db.createCS( TESTCSNAMGE );
}
catch( e )
{
   println( "failed to create cs, rc=" + e );
   throw e;
}

var res = false;
try
{
   db.createCS( TESTCSNAMGE );
}
catch( e )
{
   if( e == -33 ) res = true;
}

if( !res ) { throw -1; }

try
{
   db.dropCS( TESTCSNAMGE );
}
catch( e )
{
   println( "failed to drop cs, rc= " + e );
   throw e;
}
