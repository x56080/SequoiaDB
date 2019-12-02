//drop innormal collection space

var res = false; 
try
{
   db.dropCS( "" ); 
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
