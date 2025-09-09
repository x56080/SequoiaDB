/******************************************************************************
 * @Description   : 磁盘卡死功能测试
 * @Author        : fangjiabin
 * @CreateTime    : 2025.08.29
 * @LastEditTime  : 2025.08.29
 * @LastEditors   : fangjiabin
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   var csName = "cs_disk_10333";
   var clName = "cl_disk_10333";

   db.deleteConf( {"ftmask":'',"detectdisk":'',"ftdiskslowthreshold":'',"ftdiskslowincrement":''} );

   commDropCS( db, csName );

   var groupsArray = commGetGroups( db, false, "", true, true, true );
   var groupName = null;
   for (var i = 0; i < groupsArray.length; i++)
   {
      if (groupsArray[i][0].Length >= 2)
      {
         groupName = groupsArray[i][0].GroupName;
         break;
      }
   }
   if (!groupName)
   {
      println("skip for no slave nodes");
      return;
   }
   println("GroupName: "+groupName);

   commCreateCS( db, csName );
   commCreateCL( db, csName, clName, { Group: groupName, ReplSize: -1 } );
   var cl = db.getCS( csName ).getCL( clName );
   var recArray = [];
   for (var i = 0; i < 10; i++) {
      recArray.push( { a: i, b: i, c: i } );
   }
   cl.insert( recArray );
   commCheckLSN( db, groupName );

   var master = null;
   var masterConn = null;
   var time = 50 ;
   var hasDiskFault = false ;

   try
   {
      master = db.getRG( groupName ).getMaster();
      masterConn = master.connect();
      masterConn.traceOff("");
      var option = new SdbTraceOption().breakPoints("clsTarceEmptyFunction");
      masterConn.traceOn( 1000, option );

      while( time > 0 )
      {
         var cursor = db.snapshot(6,{RawData:true,"NodeName": master._nodename},{NodeName:'',IsPrimary:'',FTStatus:''});
         var obj = cursor.current().toObj();
         println(JSON.stringify(obj));
         if ( obj.IsPrimary == false && obj.FTStatus == "DISKFAULT" )
         {
            hasDiskFault = true;
            cl.insert( recArray );
            println("Disk fault, insert done");
            break;
         }
         sleep(5000);
         time--;
      }

      if ( !hasDiskFault )
      {
         throw new Error("Disk detect unexpect error");
      }

      masterConn.traceOff("");
      masterConn.close();
      masterConn = null;

      time = 50 ;
      while( time > 0 )
      {
         var cursor = db.snapshot(6,{RawData:true,"NodeName": master._nodename},{NodeName:'',IsPrimary:'',FTStatus:''});
         var obj = cursor.current().toObj();
         println("obj: "+JSON.stringify(obj));

         if ( obj.FTStatus == "" )
         {
            cl.insert( recArray );
            println("Disk has recover, insert done");
            break;
         }

         sleep(5000);
         time--;
      }

      commDropCS( db, csName );
   }
   finally
   {
      if (masterConn)
      {
         masterConn.traceOff("");
         masterConn.close();
      }
   }
}