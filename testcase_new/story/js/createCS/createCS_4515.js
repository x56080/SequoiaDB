/****************************************************
@description: seqDB-4515:createCS��name���ȳ����߽�
@author:
2019-6-4 wuyan init
****************************************************/
main( test );
function test ()
{
   var csNameLen = 0;
   createCSAndCheckResult( csNameLen );

   var csNameLen = 128;
   createCSAndCheckResult( csNameLen );
}

function createCSAndCheckResult ( csNameLen )
{
   var csName = getRandomString( csNameLen );

   //create cs; 
   assert.tryThrow( -6, function()
   {
      db.createCS( csName );
   } );

   //check cs is not exist; 
   assert.tryThrow( -6, function()
   {
      db.getCS( csName );
   } );
}

function getRandomString ( len )
{
   var str = "";
   if( len = 0 )
   {
      return str;
   }
   else
   {
      var chars = "1234567890abcdefghijklmnABCDEFGHIJKLMNOPQRSTUVWXYZ-���ġ�~!@#%^&()_ + ~_";
      var strLen = chars.length;
      for( var i = 0; i < len; i++ )
      {
         str += chars.charAt( Math.floor( Math.random() * strLen ) );
      }
   }

   return str;
}
