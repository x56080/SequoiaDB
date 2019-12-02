//drop not exist collection
CSPREFIX_CS = CHANGEDPREFIX + "ONWfoo"; 

CSPREFIX_CL = CHANGEDPREFIX + "bar"; 
try
{
   db.dropCS( CSPREFIX_CS ); 
}
catch( e )
{
   
}
var res = false; 
try
{
   db.dropCS( CSPREFIX_CS ); 
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
