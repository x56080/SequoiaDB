/******************************************************************************
@Description seqDB-21885:内置SQL语句查询$SNAPSHOT_CL
@author liyuanyue
@date 2020-3-19
******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   var groupName = commGetDataGroupNames( db )[0];

   commDropCL( db, COMMCSNAME, CHANGEDPREFIX + "_21885_0" );
   commDropCL( db, COMMCSNAME, CHANGEDPREFIX + "_21885_1" );
   commDropCL( db, COMMCSNAME, CHANGEDPREFIX + "_21885_2" );
   commDropCL( db, COMMCSNAME, CHANGEDPREFIX + "_21885_3" );

   commCreateCL( db, COMMCSNAME, CHANGEDPREFIX + "_21885_0", { Group: groupName } );
   commCreateCL( db, COMMCSNAME, CHANGEDPREFIX + "_21885_1", { Group: groupName } );
   commCreateCL( db, COMMCSNAME, CHANGEDPREFIX + "_21885_2", { Group: groupName } );
   commCreateCL( db, COMMCSNAME, CHANGEDPREFIX + "_21885_3", { Group: groupName } );

   // 使用内置SQL语句查询快照信息
   var cur = db.exec( "select * from $SNAPSHOT_CL" );
   while( cur.next() )
   {
      var tmpObj = cur.current().toObj();
      for( var i = 0; i < 4; i++ )
      {
         // 找到自己创建的集合
         if( tmpObj["Name"] === ( COMMCSNAME + "." + CHANGEDPREFIX + "_21885_" + i ) )
         {
            var tmpDetails = tmpObj["Details"];
            var expObj = {
               GroupName: tmpDetails[0]["GroupName"], Indexes: tmpDetails[0]["Indexes"], TotalRecords: tmpDetails[0]["TotalRecords"],
               TotalDataRead: tmpDetails[0]["TotalDataRead"], TotalDataWrite: tmpDetails[0]["TotalDataWrite"], TotalUpdate: tmpDetails[0]["TotalUpdate"],
               TotalDelete: tmpDetails[0]["TotalDelete"], TotalInsert: tmpDetails[0]["TotalInsert"], TotalInsert: tmpDetails[0]["TotalInsert"],
               TotalSelect: tmpDetails[0]["TotalSelect"], TotalRead: tmpDetails[0]["TotalRead"], TotalWrite: tmpDetails[0]["TotalWrite"]
            };
            var actObj = {
               GroupName: groupName, Indexes: 1, TotalRecords: 0, TotalDataRead: 0,
               TotalDataWrite: 0, TotalUpdate: 0, TotalDelete: 0, TotalInsert: 0,
               TotalInsert: 0, TotalSelect: 0, TotalRead: 0, TotalWrite: 0
            };
            if( !( commCompareObject( expObj, actObj ) ) )
            {
               throw new Error( "$SNAPSHOT_CL result error\n" + "expObj :" + JSON.stringify( expObj ) + "\nactObj :" + JSON.stringify( actObj ) );
            }
         }
      }
   }

   commDropCL( db, COMMCSNAME, CHANGEDPREFIX + "_21885_0" );
   commDropCL( db, COMMCSNAME, CHANGEDPREFIX + "_21885_1" );
   commDropCL( db, COMMCSNAME, CHANGEDPREFIX + "_21885_2" );
   commDropCL( db, COMMCSNAME, CHANGEDPREFIX + "_21885_3" );
}