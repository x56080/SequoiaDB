// list cs_2.

var csArray = new Array(); 

for( i = 0; i < 100; i++ )
{
   csArray[i] = COMMCSNAME + i; 
}

var i = 0; 

for( i = 0; i < csArray.length; i++ )
{
   try
   {
      db.dropCS( csArray[i] ); 
   }
   catch( e )
   {
   }
}


try
{
   for( i = 0; i < csArray.length; i++ )
   {
      db.createCS( csArray[i] ); 
   }
}
catch( e )
{
   println( "failed to create cs:" + csArray[i] + ", rc= " + e ); 
   throw e; 
}

try
{
   db.dropCS( csArray[0] ); 
   db.dropCS( csArray[1] ); 
}
catch( e )
{
   if( e != -34 )
   {
      println( "failed to clear cs 0 and 1, rc1=" + e ); 
      throw e; 
   }
}

try
{
   var cur = db.listCollectionSpaces(); 
}
catch( e )
{
   println( "failed to list CS, e=" + e ); 
   throw e; 
}

var res = false; 
var reserdCSNum = 0; 

while( cur.next() )
{
   for( var j = 0; j < csArray.length; j++ )
   {
      if( cur.current().toObj()["Name"] == csArray[j] )
      {
         reserdCSNum++; 
         break; 
      }
   }
   
   if( cur.current().toObj()["Name"] == csArray[0] || cur.current().toObj()["Name"] == csArray[1] )
   {
      println( "uncorrect name in list: " + cur.current().toObj()["Name"] )
      throw -1; 
   }
}

if( reserdCSNum != csArray.length - 2 )
{
   println( "The cs count:" + reserdCSNum + " is incorret, after drop 2 cs." ); 
   throw -1; 
}


for( i = 2; i < csArray.length; i++ )
{
   try
   {
      db.dropCS( csArray[i] ); 
   }
   catch( e )
   {
      println( "Failed to drop cs:" + csArray[i] ); 
      throw e; 
   }
}
