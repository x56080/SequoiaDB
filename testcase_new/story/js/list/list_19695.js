/******************************************************************************
*@Description : seqDB-19695:检查检查list( SDB_LIST_USERS )列表信息
*               TestLink : seqDB-19695
*@author      : wangkexin
*@Date        : 2019-09-27
******************************************************************************/
function main()
{
   if( commIsStandalone( db ) )
   {
      println( "------Deploy is standalone" ); 
      return; 
   }
   
   var userName1 = "user19695a"; 
   var userName2 = "user19695b"; 
   var expOptions1 = {}; 
   var expOptions2 = {AuditMask:"DDL|DML|!DQL"}; 
   
   db.createUsr( userName1, userName1 ); 
   db.createUsr( userName2, userName2, expOptions2 ); 
   
   checkListUsersResult( userName1, expOptions1 ); 
   checkListUsersResult( userName2, expOptions2 ); 
   
   db.dropUsr( userName1, userName1 ); 
   db.dropUsr( userName2, userName2 ); 
}

function checkListUsersResult( expUser, expOptions )
{
   var cur = db.list( SDB_LIST_USERS, {"User": expUser} ); 
   var returnedCount = 0; 
   while( cur.next() )
   {
      var obj = cur.current().toObj(); 
      if( obj.User != expUser )
      {
         throw new Error( "compare user name failed, expUser is " + expUser + ", actUser " + obj.User ); 
      }
      
      if( JSON.stringify( obj.Options )!= JSON.stringify( expOptions ) )
      {
         throw new Error( "compare options failed, exp options is " + JSON.stringify( expOptions )+ ", act options is " + JSON.stringify( obj.Options ) ); 
      }
      returnedCount++; 
   }
   
   if( returnedCount != 1 )
   {
      throw new Error( "check list returned number failed, returned number is 1, act returned number is " + returnedCount ); 
   }
}

try
{
   main(); 
}
catch( e )
{
   if( e.constructor === Error )
   {
      println( e.stack ); 
   }
   throw e; 
}
