// create cs.
// unnormal_2 case.
TESTCSNAMGE = CHANGEDPREFIX + "foo";

TESTCLNAMGE = CHANGEDPREFIX + "bar";
var res = false;
try
{
   db.createCS( TESTCSNAMGE + ".cs" );
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



