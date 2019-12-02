// create cs.
// unnormal_3 case
var res = false; 
try
{
   db.createCS( "" ); 
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



