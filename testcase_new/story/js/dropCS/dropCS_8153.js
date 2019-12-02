//drop innormal collection space

var res = false; 
try
{
   db.dropCS( "dropUCS.cs" ); 
}
catch( e )
{
   if( e == -34 )
   {
      res = true; 
   }
}
if( !res )
{
   throw -1; 
}
