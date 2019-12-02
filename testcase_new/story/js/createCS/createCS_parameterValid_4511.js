/****************************************************
@description: seqDB-4511:createCS，覆盖options:Domain长度边界:1B/127B
@author:
2019-6-4 wuyan init
****************************************************/
main(); 

function main()
{
   if( true === commIsStandalone( db ) )
   {
      println( "Standalone environment!" ); 
      return; 
   }
   
   var groups = commGetGroups( db ); 
   var groupName = groups[0][0]["GroupName"]; 
   
   var domainNameLen = 1; 
   println( "---Begin to test domainName len is 1. " ); 
   createCSAndCheckResult( domainNameLen, groupName ); 
   
   println( "---Begin to test domainName len is 127B." ); 
   var domainNameLen = 127; 
   createCSAndCheckResult( domainNameLen, groupName ); 
}

function createCSAndCheckResult( domainNameLen, groupName )
{
   println( "\n---Begin to createCS.the domainName length is " + domainNameLen ); 
   var domainName = getRandomString( domainNameLen ); 
   var csName = "cs4511"; 
   var clName = "cl4511"
   commDropCS( db, csName, true, "clear cs in the beginning." )
   commDropDomain( db, domainName ); 
   commCreateDomain( db, domainName, [ groupName ] ); 
   var cs = db.createCS( csName, { Domain: domainName} ); 
   cs.createCL( clName ); 
   checkCSInDomain( domainName, csName ); 
   
   commDropCS( db, csName, false, "clear cs in the ending." ); 
   commDropDomain( db, domainName ); 
}

function getRandomString( len )
{
   var chars = "1234567890abcdefghijklmnABCDEFGHIJKLMNOPQRSTUVWXYZ-中文。~!@#%^&()_ + ~_"; 
   var str = ""; 
   var strLen = chars.length; 
   
   var clPrefix = "4511cs_"; 
   if( len > clPrefix.length )
   {
      len = len - clPrefix.length; 
   }
   
   for( var i = 0; i < len; i++ )
   {
      str += chars.charAt( Math.floor( Math.random()* strLen ) ); 
   }
   
   if( len > clPrefix.length )
   {
      str = clPrefix + str; 
   }
   
   return str; 
}

function checkCSInDomain( domainName, csName )
{
   var domainCur = db.getDomain( domainName ); 
   while( domainCur.next )
   {
      var domainInfo = domainCur.current(); 
      var csNameInDomain = domainInfo.toObj().Name; 
      if( csName !== csNameInDomain )
      {
         throw buildException( "checkCSInDomain", "", "csName:" + csName, "the Name of Domain is :" + csNameInDomain ); 
      }
   }
}
