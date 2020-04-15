/* *****************************************************************************
@description: seqDB-22072:设置会话访问属性preferedinstance/PreferedInstanceMode/PreferedStrict/PreferedPeriod的值，检查设置成功 
@author: 2020-4-9 zhaoxiaoni  Init
***************************************************************************** */
main();

function main()
{
   if( commIsStandalone( db ) )
   {
      println( "run mode is standalone" );
      return;
   }

   //v2.8版本不支持设置PreferedStrict的值
   //PreferedInstance为字母、PreferedInstanceMode为ordered、PreferedPeriod为默认值
   var options = { PreferedInstance: "M", PreferedInstanceMode: "ordered" };
   db.setSessionAttr( options );
   var expResult = { PreferedInstance: "M", PreferedInstanceMode: "ordered", PreferedPeriod: 60 };
   checkSessionAttr( expResult );

   //PreferedInstance为数字、PreferedInstanceMode为random、PreferedPeriod为-1
   options = { PreferedInstance: 12, PreferedInstanceMode: "random", PreferedPeriod: -1 };
   db.setSessionAttr( options );
   expResult = { PreferedInstance: 12, PreferedInstanceMode: "random", PreferedPeriod: -1 };
   checkSessionAttr( expResult );

   //PreferedInstance为数组、PreferedInstanceMode默认值、PreferedPeriod为0
   options = { PreferedInstance: [ 12, 25, 30 ], PreferedPeriod: 0 };
   db.setSessionAttr( options );
   expResult = { PreferedInstance: [ 12, 25, 30 ], PreferedInstanceMode: "random", PreferedPeriod: 0 };
   checkSessionAttr( expResult ); 

   //PreferedPeriod为400000000000000000000000000
   var options = { PreferedPeriod: 400000000000000000000000000 };
   db.setSessionAttr( options );
   var expResult = { PreferedPeriod: -1 };
   checkSessionAttr( expResult );
}

function checkSessionAttr( expResult )
{
   var actResult = {};
   var object = db.getSessionAttr();
   actResult.PreferedInstance = object.PreferedInstance;
   actResult.PreferedInstanceMode = object.PreferedInstanceMode;
   actResult.PreferedPeriod = object.PreferedPeriod;
   commCompareObject ( expResult, actResult );  
}
