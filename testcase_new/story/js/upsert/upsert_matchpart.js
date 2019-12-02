// upsert record.
// normal case.

// clear
commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop cl in the beginning" ); 

// create cs, cl
var varCL = commCreateCL( db, COMMCSNAME, COMMCLNAME, -1, true, true, false, 
"create cs and cl in begin" ); 
var count = 100; 

try
{
   for( i = 0; i < count - 10; i++ )
   {
      varCL.insert( {id:i, mineName:"上海矿场", mineTime:"2013-06-14", localtion:{resId:0, resourceName:null, country:"中国", state:"上海", city:"上海市"}} ); 
   }
   for(; i < count; i++ )
   {
      varCL.insert( {id:i, mineName:"北京矿场", mineTime:"2013-06-14", localtion:{resId:0, resourceName:null, country:"中国", state:"北京", city:"北京市"}} ); 
   }
}
catch( e )
{
   println( "failed to insert record, rc= " + e ); 
   throw e; 
}

try
{
   varCL.upsert( {$set:{"localtion.street":"人民路12号"}}, {mineName:"北京矿场"} ); 
}
catch( e )
{
   println( "failed to update( {localtion:{$set:{street:人民路12号}}, rc= " + e ); 
   throw e; 
}

var rc; 
try
{
   rc = varCL.find(); 
}
catch( e )
{
   println( "failed to read record, rc= " + e ); 
   throw e; 
}

var size = 0; 
while( rc.next() )
{
   recordObj = rc.current().toObj(); 
   recordStr = rc.current().toJson(); 
   
   if( recordObj["mineName"] == "北京矿场" )
   {
      if( recordObj["localtion"]["street"] != "人民路12号" )
      {
         println( "The record is not be upsert:record" + recordStr ); 
         throw -1; 
      }
   }
   else
   {
      if( recordObj["localtion"]["street"] != null )
      {
         println( "The record should not be upsert:record" + recordStr ); 
         throw -1; 
      }
   }
   
   size++; 
}

if( size != count )
{
   println( "The record size of not equal " + count ); 
   println( varCL.find() ); 
   throw -1; 
}

// clear
commDropCL( db, COMMCSNAME, COMMCLNAME, false, false, "drop cl in the end" ); 
