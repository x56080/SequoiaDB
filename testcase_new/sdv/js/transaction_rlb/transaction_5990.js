/*******************************************************************************
*@Description: seqDB-5990:事务功能关闭，执行事务操作
*@Author:      2020-4-25 Zhao Xiaoni
********************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test()
{
   try
   {
      db.updateConf( { "transactionon": false } );
      throw "Excute updateConf should be failed!";
   }
   catch( e )
   {
      if( e !== -264 )
      {
         throw new Error( e );
      }
   }
   var groupName = commGetGroups( db )[0][0].GroupName;
   db.getRG( groupName ).stop();
   db.getRG( groupName ).start();
   
   try
   {
      db.transBegin();
      throw "Excute transBegin should be failed!";
   }
   catch( e )
   {
      if( e !== -253 )
      {
         throw new Error( e );
      }
   }

   try
   {
      db.deleteConf( { "transactionon": 1 } );
      throw "Excute deleteConf should be failed!";
   }
   catch( e )
   {
      if( e !== -264 )
      {
         throw new Error( e );
      }
   }
  
   db.getRG( groupName ).stop();
   db.getRG( groupName ).start();
}

