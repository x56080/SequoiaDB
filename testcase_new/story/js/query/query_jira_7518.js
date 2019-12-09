/*******************************************************************************
*@Description : check the times of index read when query, if it hasn't save the
*               context, duplicate index read will happen. [jira_201]
*@Modify list :
*               2014-11-6  xiaojun Hu  Changed
*               2019-08-23 wangkexin Modified
*******************************************************************************/
main();
// main function
function main ()
{
   var clName = "cl7518";
   var cl = readyCL( clName );
   // insert data
   var count = 2000;
   var records = [];
   for( var i = 1; i <= count; i++ )
   {
      records.push( { id: i, pid: i, name: "mike" + i + "index" + i, content: "afdsafdsafdsafdsafdsafdsafdsafdsafdsafdsafdsafdsa", uid: i } );
   }
   cl.insert( records );
   println( "insert data success" );

   // create index
   cl.createIndex( "idIndex", { id: 1 }, false );
   cl.createIndex( "idpidIndex", { id: -1, pid: 1 }, false );
   cl.createIndex( "idpidnameIndex", { id: 1, pid: 1, name: 1 }, true );
   println( "create index successful" );

   var _clName = COMMCSNAME + "." + clName;
   var snapInfo1 = getCurrentRead( db, _clName );
   var dataRead1 = snapInfo1[0];
   var indexRead1 = snapInfo1[1];
   println( db.snapshot( SDB_SNAP_SESSIONS_CURRENT, {},
      {
         "SessionID": 1, "TotalIndexRead": 1,
         "TotalDataRead": 1
      } ) );
   // query1. {Index Name : "idIndex"}
   println( "==>query1. {Index Name : 'idIndex'}" );
   println( cl.find( { id: 1000 } ).next() );
   var snapInfo2 = getCurrentRead( db, _clName );
   var dataRead2 = snapInfo2[0];
   var indexRead2 = snapInfo2[1];
   var count = cl.count();
   var dataVal = dataRead2 - dataRead1;
   println( "dataRead2 =" + dataRead2 + ",dataRead1 =" + dataRead1 + ",value =" + dataVal );
   //compare the times with count*10
   var value = indexRead2 - indexRead1;
   println( "indexRead2 =" + indexRead2 + ",indexRead1 =" + indexRead1 + ",value = " + value );
   if( value == 0 )
   {
      throw "error,this operation must use the index read";
   }
   if( value > count * 5 )
   {
      throw "read index too much times";
   }

   // query2. {Index Name : "idpidIndex"}
   println( "==>query2. {Index Name : 'idpidIndex'}" );
   println( cl.find( { id: { $lt: 5000, $gt: 2000 }, pid: 10 } ).next() );
   //sleep(5000);
   var snapInfo3 = getCurrentRead( db, _clName );
   var dataRead3 = snapInfo3[0];
   var indexRead3 = snapInfo3[1];
   var count = cl.count();
   var dataVal = dataRead3 - dataRead2;
   println( "dataRead3 =" + dataRead3 + ",dataRead2 =" + dataRead2 + ",value =" + dataVal );
   //compare the times with the count
   value = indexRead3 - indexRead2;
   println( "indexRead3 =" + indexRead3 + ",indexRead2 =" + indexRead2 + ",value = " + value );
   if( value == 0 )
   {
      println( "error,this operation must use the index read" );
      throw "ErrIndexRead[Idx3-Idx2]";
   }
   if( value > count * 2 )
   {
      throw "read index times greater than the data count"
   }

   // query3. {Index Name : "idpidIndex"}
   println( "==>query3. {Index Name : 'idpidnameIndex'}" );
   println( cl.find( {
      $and: [{ id: 1000 }, { pid: { $lt: 10 } },
      { name: "mike1000index3" }]
   } ).next() );
   var snapInfo4 = getCurrentRead( db, _clName );
   var dataRead4 = snapInfo4[0];
   var indexRead4 = snapInfo4[1];
   var dataVal = dataRead4 - dataRead3;
   println( "dataRead4 =" + dataRead4 + ",dataRead3 =" + dataRead3 + ",value =" + dataVal );
   value = indexRead4 - indexRead3;
   println( "indexRead4 =" + indexRead4 + ",indexRead3 =" + indexRead3 + ",value = " + value );
   if( value == 0 )
   {
      throw "error,this operator must use the index read";
   }

   if( value > 20 )
   {
      throw "read index too much times";
   }

   // query4. {Index Name : null }
   println( "==>query4. {Index Name : null}" );
   println( cl.find( { $and: [{ pid: { $gt: 1 } }, { uid: 50 }] } ).next() );
   var snapInfo5 = getCurrentRead( db, _clName );
   var dataRead5 = snapInfo5[0];
   var indexRead5 = snapInfo5[1];
   var dataVal = dataRead5 - dataRead4;
   println( "dataRead5 =" + dataRead5 + ",dataRead4 =" + dataRead4 + ",value =" + dataVal );
   value = indexRead5 - indexRead4;
   println( "indexRead5 =" + indexRead5 + ",indexRead4 =" + indexRead4 + ",value = " + value );
   if( value > 0 )
   {
      throw "error,this operator can't use the index read";
   }
   println( "query data success" );

   commDropCL( db, COMMCSNAME, clName, false, false, "clear collection in the end, correct way" );
}

function getCurrentRead ( db, clName )
{
   var masNode = new Array;
   var masHost = new Array;
   var clRG = commGetCLGroups( db, clName );
   for( var i = 0; i < clRG.length; ++i )
   {
      var group = commGetGroups( db );
      for( var j = 0; j < group.length; ++j )
      {
         if( clRG[i] == group[j][0].GroupName )
         {
            for( var m = 0; m < group[j][0].Length; ++m )
            {
               masNode[m] = group[j][m + 1].svcname;    // get master node
               masHost[m] = group[j][m + 1].HostName;   // get master host
            }
         }
      }
   }
   var currentSession = db.snapshot( SDB_SNAP_SESSIONS_CURRENT, {},
      {
         "NodeName": 1, "SessionID": 1, "TotalIndexRead": 1,
         "TotalDataRead": 1
      } ).toArray();
   var dataIdx = new Array( 0, 0 );
   println( "current session length : " + currentSession.length );
   // get total data read and total index read in this loop
   for( var i = 0; i < currentSession.length; ++i )
   {
      var sessionObj = eval( "(" + currentSession[i] + ")" );
      var _nodeName = sessionObj.NodeName;
      totalIdxRead = sessionObj.TotalIndexRead;
      var nodeSplit = _nodeName.split( ":" );
      if( false == commIsStandalone( db ) )   // group
      {
         for( var j = 0; j < masNode.length; ++j )
         {
            if( masNode[j] == nodeSplit[1] && masHost[j] == nodeSplit[0] )
            {
               dataIdx[0] += sessionObj.TotalDataRead;
               dataIdx[1] += sessionObj.TotalIndexRead;
               println( "host : " + masHost[j] + " node : " + masNode[j] );
               break;
            }
         }
      }
      else   // standalone
      {
         dataIdx[0] = sessionObj.TotalDataRead;
         dataIdx[1] = sessionObj.TotalIndexRead;
         println( db.snapshot( SDB_SNAP_SESSIONS_CURRENT, {},
            {
               "SessionID": 1, "TotalIndexRead": 1,
               "TotalDataRead": 1
            } ) );
      }
   }
   return dataIdx;
}
