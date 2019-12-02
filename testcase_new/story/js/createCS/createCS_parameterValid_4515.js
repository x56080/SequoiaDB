/****************************************************
@description: seqDB-4515:createCS，name长度超过边界
@author:
2019-6-4 wuyan init
****************************************************/
main(); 

function main()
{
   var csNameLen = 0; 
   println( "---Begin to test csName len is 0 " ); 
   createCSAndCheckResult( csNameLen ); 
   
   println( "---Begin to test csName len is 128B" ); 
   var csNameLen = 128; 
   createCSAndCheckResult( csNameLen ); 
}

function createCSAndCheckResult( csNameLen )
{
   println( "\n---Begin to createCS." ); 
   var csName = getRandomString( csNameLen ); 
   
   //create cs; 
   try
   {
      db.createCS( csName ); 
      throw "create cs should be fail!"; 
   }
   catch( e )
   {
      if( e !== -6 )
      {
         throw buildException( "create cs", e ); 
      }
      
   }
   
   //check cs is not exist; 
   try
   {
      db.getCS( csName ); 
      throw "get cs should be fail!"
   }
   catch( e )
   {
      if( e !== -6 )
      {
         throw buildException( "check cs", e ); 
      }
   }
   
}

function getRandomString( len )
{
   var str = ""; 
   if( len = 0 )
   {
      return str; 
   }
   else
   {
      var chars = "1234567890abcdefghijklmnABCDEFGHIJKLMNOPQRSTUVWXYZ-中文。~!@#%^&()_ + ~_"; 
      var strLen = chars.length; 
      for( var i = 0; i < len; i++ )
      {
         str += chars.charAt( Math.floor( Math.random()* strLen ) ); 
      }
   }
   
   return str; 
}
