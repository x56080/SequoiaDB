// create cs.
// unnormal_1 case.
TESTCSNAMGE = CHANGEDPREFIX + "foo";

TESTCLNAMGE = CHANGEDPREFIX + "bar";
var res = false;
try
{
   db.createCS( "SYS" + TESTCSNAMGE );
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
   db.dropCS( "SYS" + TESTCSNAMGE );
}
catch( e )
{
}

