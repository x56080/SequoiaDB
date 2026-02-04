/******************************************************************************
 * @Description   : seqDB-33462:超过半数节点故障，启动运维模式后存活节点不超过半数
 * @Author        : liuli
 * @CreateTime    : 2023.09.20
 * @LastEditTime  : 2023.09.20
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipGroupLessThanThree = true;

main( test );
function test ()
{
   var groupName = commGetDataGroupNames( db )[0];
   var group = db.getRG( groupName );
   var slaveNodeNames = getGroupSlaveNodeName( db, groupName );
   
   if ( slaveNodeNames.length < 2 )
   {
       return ;
   }

   try
   {
      // 停止所有备节点
      stopNodes( db, group, slaveNodeNames ) ;

      // group循环获取主节点，最终报错
      var timeout = 30;
      var doTime = 0;
      while( doTime < timeout )
      {
         try
         {
            group.getMaster();
         } catch( e )
         {
            if( e == SDB_RTN_NO_PRIMARY_FOUND )
            {
               break;
            } else
            {
               throw e;
            }
         }
         sleep( 1000 );
         doTime++;
      }

      if( doTime >= timeout )
      {
         throw new Error( "waitNodeStart timeout" );
      }

      // 一个节点启动运维模式
      var options = { NodeName: slaveNodeNames[0], MinKeepTime: 10, MaxKeepTime: 20 };
      group.startMaintenanceMode( options );

      assert.tryThrow( SDB_RTN_NO_PRIMARY_FOUND, function()
      {
         group.getMaster();
      } )

      // 其它备节点也启动运维模式
      for ( var i = 1 ; i < slaveNodeNames.length ; ++i )
      {
          options = { NodeName: slaveNodeNames[i], MinKeepTime: 10, MaxKeepTime: 20 };
          group.startMaintenanceMode( options );
      }

      // 等待选主
      var timeout = 30;
      var doTime = 0;
      while( doTime < timeout )
      {
         try
         {
            group.getMaster();
            break;
         } catch( e )
         {
            if( e != SDB_RTN_NO_PRIMARY_FOUND )
            {
               throw e;
            }
         }
         sleep( 1000 );
         doTime++;
      }

      if( doTime >= timeout )
      {
         throw new Error( "waitNodeStart timeout" );
      }
   } finally
   {
      // 启动节点，恢复集群
      group.start();
      group.stopMaintenanceMode();
   }

   commCheckBusinessStatus( db );
}