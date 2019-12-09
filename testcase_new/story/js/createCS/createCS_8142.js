// create cs
// unnormal case
TESTCSNAMGE = CHANGEDPREFIX + "foo";

TESTCLNAMGE = CHANGEDPREFIX + "bar";
var res = false;
var name = "";
for( var i = 0; i < 10000; i++ )
{
   name = name + "a";
}
try
{
   db.createCS( name );
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
