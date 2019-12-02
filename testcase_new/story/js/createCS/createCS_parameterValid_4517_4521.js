/****************************************************
@description: seqDB-4517:createCS，createCS，options:PageSize无效取值
seqDB-4521:createCS，createCS，options:LobPageSize无效取值
@author:
2019-6-4 wuyan init
****************************************************/
main(); 
function main()
{
   if( true == commIsStandalone( db ) )
   {
      println( "run mode is standalone" ); 
      return; 
   }
   var csName = "cs4517"; 
   commDropCS( db, csName, true, "clear cs in the beginning." ); 
   
   println( "---Begin to test testcase-4517. " ); 
   var pageSizes = [ "", 4097, -4096 ]; 
   var lobPageSize = 4096; 
   for( var i = 0; i < pageSizes.length; i++ )
   {
      var pageSize = pageSizes[i]; 
      createCSWithLobPageSize( csName, lobPageSize, pageSize ); 
   }
   
   println( "---Begin to test testcase-4521. " ); 
   var lobPageSizes = [ "", 1, -4096]; 
   var pageSize = 4096; 
   for( var i = 0; i < lobPageSizes.length; i++ )
   {
      var lobPageSize = lobPageSizes[i]; 
      createCSWithLobPageSize( csName, lobPageSize, pageSize ); 
   }
   
}

function createCSWithLobPageSize( csName, lobPageSize, pageSize )
{
   println( "\n---Begin to createCS with lobPageSize:" + lobPageSize + " pageSize:" + pageSize ); 
   //create cs; 
   try
   {
      var options = { LobPageSize : lobPageSize, PageSize : pageSize }; 
      db.createCS( csName, options ); 
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
      if( e !== -34 )
      {
         throw buildException( "check cs", e ); 
      }
   }
   
}

