import( "../lib/main.js" )
import( "../lib/basic_operation/commlib.js" )

// debug to print change stream records
var _changeStream_debug = false ;

// select a group with secondary nodes
function selectGroupWithSecondary( db )
{
   var groups = commGetGroups( db, false, "" ) ;
   var selectedGroup = "" ;
   for ( index = 0 ; index < groups.length ; ++ index )
   {
      if ( groups[ index ][ 0 ].Length > 1 )
      {
         selectedGroup = groups[ index ][ 0 ].GroupName ;
         break ;
      }
   }
   if ( "" == selectedGroup )
   {
      println( "select no group" ) ;
      return selectedGroup ;
   }
   println( "selected group " + selectedGroup ) ;
   return selectedGroup ;
}

// select primary node with a collection
function selectPrimaryForChangeStream( db, csName, clName )
{
   var groups = commGetCLGroups( db, csName + "." + clName ) ;
   var index = parseInt( Math.random() * groups.length ) ;
   println( "selected group " + groups[ index ] ) ;
   var node = db.getRG( groups[ index ] ).getMaster() ;
   println( "selected node " + node ) ;
   return node.connect() ;
}

// select secondary node with a collection
function selectSecondaryForChangeStream( db, csName, clName )
{
   var groups = commGetCLGroups( db, csName + "." + clName ) ;
   var index = parseInt( Math.random() * groups.length ) ;
   println( "selected group " + groups[ index ] ) ;
   var node = db.getRG( groups[ index ] ).getSlave() ;
   println( "selected node " + node ) ;
   return node.connect() ;
}

function checkChangeStreamCommonResult( actRecord, csName, clName, changeFlags, isTrans, isTransRollback )
{
   if ( undefined != csName )
   {
      if ( csName == "" )
      {
         assert.equal( actRecord.CollectionSpace, undefined,
                       "collection space name should be undefined" ) ;
      }
      else
      {
         assert.equal( actRecord.CollectionSpace,
                       csName,
                       "collection space name is different" ) ;
      }
   }
   if ( undefined != clName )
   {
      if ( clName == "" )
      {
         assert.equal( actRecord.Collection, undefined,
                       "collection name should be undefined" ) ;
      }
      else
      {
         assert.equal( actRecord.Collection,
                       csName + "." + clName,
                       "collection name is different" ) ;
      }
   }
   if ( undefined != changeFlags )
   {
      assert.equal( actRecord.ChangeFlags, changeFlags,
                    "change flags are different" ) ;
   }
   if ( undefined != isTrans )
   {
      assert.equal( actRecord.TransInfo.TransID != undefined, true,
                    "transaction is required" ) ;
   }
   if ( undefined != isTransRollback )
   {
      assert.equal( actRecord.TransInfo.TransAttr.indexOf( "Rollback" ) >= 0, true,
                    "transaction rollback is required" ) ;
   }
}

// check insert change stream result
function checkChangeStreamInsertResult( cursor, csName, clName, document, changeFlags, isTrans, isTransRollback )
{
   var token = "" ;

   try
   {
      while ( cursor.next() )
      {
         var actRecord = cursor.current().toObj() ;
         if ( _changeStream_debug )
         {
            println( JSON.stringify( actRecord ) ) ;
         }
         if ( actRecord.Type == "control" )
         {
            if ( actRecord.ControlType == "empty" )
            {
               actRecord = null ;
               continue ;
            }
            else
            {
               throw new Error( "change stream is stopped" ) ;
            }
         }
         else if ( actRecord.Type == "change" )
         {
            if ( actRecord.ChangeType == "insert" )
            {
               if ( undefined != document )
               {
                  assert.equal( actRecord.DocumentKey._id, document._id,
                                "document key is different" ) ;
                  if ( !commCompareObject( actRecord.Document, document ) )
                  {
                     throw new Error( "document is different" ) ;
                  }
               }
               checkChangeStreamCommonResult( actRecord, csName, clName, changeFlags, isTrans, isTransRollback ) ;
               token = actRecord.Token ;
               break ;
            }
            else
            {
               throw new Error( "unexpected change type: " + actRecord.ChangeType ) ;
            }
         }
         else
         {
            throw new Error( "unexpected record type: " + actRecord.Type ) ;
         }
      }
   }
   catch ( e )
   {
      cursor.close() ;
      commThrowError( e, "checkChangeStreamInsertResult" ) ;
   }

   return token ;
}

// check update change stream result
function checkChangeStreamUpdateResult( cursor, csName, clName, documentKey, updateAction, changeFlags, isTrans, isTransRollback )
{
   var token = "" ;

   try
   {
      while ( cursor.next() )
      {
         var actRecord = cursor.current().toObj() ;
         if ( _changeStream_debug )
         {
            println( JSON.stringify( actRecord ) ) ;
         }
         if ( actRecord.Type == "control" )
         {
            if ( actRecord.ControlType == "empty" )
            {
               actRecord = null ;
               continue ;
            }
            else
            {
               throw new Error( "change stream is stopped" ) ;
            }
         }
         else if ( actRecord.Type == "change" )
         {
            if ( actRecord.ChangeType == "update" )
            {
               if ( undefined != documentKey )
               {
                  assert.equal( actRecord.DocumentKey._id, documentKey._id,
                                "document key is different" ) ;
               }
               if ( undefined != updateAction &&
                    !commCompareObject( actRecord.UpdateAction, updateAction ) )
               {
                  throw new Error( "update action is different" ) ;
               }
               checkChangeStreamCommonResult( actRecord, csName, clName, changeFlags, isTrans, isTransRollback ) ;
               token = actRecord.Token ;
               break ;
            }
            else
            {
               throw new Error( "unexpected change type: " + actRecord.ChangeType ) ;
            }
         }
         else
         {
            throw new Error( "unexpected record type: " + actRecord.Type ) ;
         }
      }
   }
   catch ( e )
   {
      cursor.close() ;
      commThrowError( e, "checkChangeStreamUpdateResult" ) ;
   }

   return token ;
}

// check delete change stream result
function checkChangeStreamDeleteResult( cursor, csName, clName, documentKey, changeFlags, isTrans, isTransRollback )
{
   var token = "" ;

   try
   {
      while ( cursor.next() )
      {
         var actRecord = cursor.current().toObj() ;
         if ( _changeStream_debug )
         {
            println( JSON.stringify( actRecord ) ) ;
         }
         if ( actRecord.Type == "control" )
         {
            if ( actRecord.ControlType == "empty" )
            {
               actRecord = null ;
               continue ;
            }
            else
            {
               throw new Error( "change stream is stopped" ) ;
            }
         }
         else if ( actRecord.Type == "change" )
         {
            if ( actRecord.ChangeType == "delete" )
            {
               if ( undefined != documentKey )
               {
                  assert.equal( actRecord.DocumentKey._id, documentKey._id,
                                "document key is different" ) ;
               }
               checkChangeStreamCommonResult( actRecord, csName, clName, changeFlags, isTrans, isTransRollback ) ;
               token = actRecord.Token ;
               break ;
            }
            else
            {
               throw new Error( "unexpected change type: " + actRecord.ChangeType ) ;
            }
         }
         else
         {
            throw new Error( "unexpected record type: " + actRecord.Type ) ;
         }
      }
   }
   catch ( e )
   {
      cursor.close() ;
      commThrowError( e, "checkChangeStreamDeleteResult" ) ;
   }

   return token ;
}

// check lob change stream result
function checkChangeStreamLobResult( cursor, csName, clName, documentKey, changeType, changeFlags )
{
   var token = "" ;

   try
   {
      while ( cursor.next() )
      {
         var actRecord = cursor.current().toObj() ;
         if ( _changeStream_debug )
         {
            println( JSON.stringify( actRecord ) ) ;
         }
         if ( actRecord.Type == "control" )
         {
            if ( actRecord.ControlType == "empty" )
            {
               actRecord = null ;
               continue ;
            }
            else
            {
               throw new Error( "change stream is stopped" ) ;
            }
         }
         else if ( actRecord.Type == "change" )
         {
            if ( actRecord.ChangeType == changeType )
            {
               if ( undefined != documentKey )
               {
                  assert.equal( actRecord.DocumentKey._id, documentKey._id,
                                "document key is different" ) ;
               }
               checkChangeStreamCommonResult( actRecord, csName, clName, changeFlags ) ;
               token = actRecord.Token ;
               break ;
            }
            else
            {
               throw new Error( "unexpected change type: " + actRecord.ChangeType ) ;
            }
         }
         else
         {
            throw new Error( "unexpected record type: " + actRecord.Type ) ;
         }
      }
   }
   catch ( e )
   {
      cursor.close() ;
      commThrowError( e, "checkChangeStreamDeleteResult" ) ;
   }

   return token ;
}

// check update change stream result
function checkChangeStreamResult( cursor, csName, clName, changeType, changeFlags )
{
   var token = "" ;

   try
   {
      while ( cursor.next() )
      {
         var actRecord = cursor.current().toObj() ;
         if ( _changeStream_debug )
         {
            println( JSON.stringify( actRecord ) ) ;
         }
         if ( actRecord.Type == "control" )
         {
            if ( actRecord.ControlType == "empty" )
            {
               actRecord = null ;
               continue ;
            }
            else
            {
               throw new Error( "change stream is stopped" ) ;
            }
         }
         else if ( actRecord.Type == "change" )
         {
            if ( actRecord.ChangeType == changeType )
            {
               checkChangeStreamCommonResult( actRecord, csName, clName, changeFlags ) ;
               token = actRecord.Token ;
               break ;
            }
            else
            {
               throw new Error( "unexpected change type: " + actRecord.ChangeType ) ;
            }
         }
         else
         {
            throw new Error( "unexpected record type: " + actRecord.Type ) ;
         }
      }
   }
   catch ( e )
   {
      cursor.close() ;
      commThrowError( e, "checkChangeStreamDeleteResult" ) ;
   }

   return token ;
}

// check error change stream result
function checkChangeStreamErrorResult( cursor, skipChange, errorCode, errorReason )
{
   var token = "" ;

   try
   {
      while ( cursor.next() )
      {
         var actRecord = cursor.current().toObj() ;
         if ( _changeStream_debug )
         {
            println( JSON.stringify( actRecord ) ) ;
         }
         if ( actRecord.Type == "control" )
         {
            if ( actRecord.ControlType == "empty" )
            {
               actRecord = null ;
               continue ;
            }
            else if ( actRecord.ControlType == "error" )
            {
               assert.equal( actRecord.ControlRC, errorCode, "error code is different" ) ;
               if ( undefined != errorReason )
               {
                  assert.equal( actRecord.ControlReason, errorReason, "error reason is different" ) ;
               }
               token = actRecord.Token ;
               break ;
            }
            else
            {
               throw new Error( "change stream is stopped" ) ;
            }
         }
         else if ( skipChange && actRecord.Type == "change" )
         {
            actRecord = null ;
            continue ;
         }
         else
         {
            throw new Error( "unexpected record type: " + actRecord.Type ) ;
         }
      }
   }
   catch ( e )
   {
      cursor.close() ;
      commThrowError( e, "checkChangeStreamErrorResult" ) ;
   }

   return token ;
}

// check change stream is closed
function checkChangeStreamClosed( cur )
{
   assert.equal( cur.next(), null, "change stream is not closed" ) ;
}

// get node list in a group
function getNodesInGroups( db, group )
{
   var datas = new Array();

   //standalone
   if( true === commIsStandalone( db ) )
   {
      datas[0] = db;
   }
   else
   {
      var rg = db.getRG( group );
      var rgDetail = eval( "( " + rg.getDetail().toArray()[0] + " )" );
      var nodesInGroup = rgDetail.Group;
      for( var j = 0; j < nodesInGroup.length; ++j )
      {
         var hostName = nodesInGroup[j].HostName;
         var serviceName = nodesInGroup[j].Service[0].Name;
         datas[j] = new Sdb( hostName, serviceName );
      }
   }
   return datas;
}

// get primary node lsn in a group
function getPrimaryNodeLSNs( db, group )
{
   var datas = getNodesInGroups( db, group );
   var lsn = 0 ;
   for( var j = 0; j < datas.length; ++j )
   {
      var getSnapshot6 = eval( "( " + datas[j].snapshot( 6 ).toArray()[0] + " )" );

      var completeLSN = getSnapshot6.CompleteLSN;
      var isPrimary = getSnapshot6.IsPrimary;
      if( isPrimary )
      {
         lsn = completeLSN;
         break;
      }
   }
   return lsn;
}

// get secondary node lsn in a group
function getSecondaryNodeLSNs( db, group )
{
   var datas = getNodesInGroups( db, group );

   var LSNs = new Array();
   var f = 0;
   for( var j = 0; j < datas.length; ++j )
   {
      var getSnapshot6 = eval( "( " + datas[j].snapshot( 6 ).toArray()[0] + " )" );

      var completeLSN = getSnapshot6.CompleteLSN;
      var isPrimary = getSnapshot6.IsPrimary;
      if( !isPrimary )
      {
         LSNs[f++] = completeLSN;
      }
   }

   return LSNs;
}

// check LSN consistency of a group
function checkLSN( db, group, primaryLSN )
{
   var slaveNodeLSNs = getSecondaryNodeLSNs( db, group );
   for( var j = 0; j < slaveNodeLSNs.length; ++j )
   {
      if( primaryLSN > slaveNodeLSNs[j] )
      {
         return false;
      }
   }
   return true;
}

// loop check LSN consistency of a group
function checkLSNConsistency( db, group )
{
   //the longest waiting time is 600S
   var lsnFlag = false;
   var timeout = 600;
   var doTimes = 0;

   //get primary nodes
   var primaryLSN = getPrimaryNodeLSNs( db, group );
   while( true )
   {
      lsnFlag = checkLSN( db, group, primaryLSN );
      if( !lsnFlag )
      {
         if( doTimes < timeout )
         {
            ++doTimes;
            sleep( 1000 );
         }
         else
         {
            throw new Error( "check lsn time out" );
         }
      }
      else
      {
         break;
      }
   }
}

// get recycle names of a given origin name
function getRecycleName( sdb, originName, opType )
{
   var option;
   if( opType == undefined )
   {
      option = { "OriginName": originName };
   }
   else
   {
      option = { "OriginName": originName, "OpType": opType };
   }
   var recycleNames = [];
   var rc = sdb.getRecycleBin().list( option );
   while( rc.next() )
   {
      var recycleName = rc.current().toObj().RecycleName;
      recycleNames.push( recycleName );
   }
   rc.close();
   recycleNames.sort();
   return recycleNames;
}

// get first recycle name of a given origin name
function getOneRecycleName( sdb, originName, opType )
{
   return getRecycleName( sdb, originName, opType )[0];
}

// sub test tear down
function subTearDown( db, testConf )
{
   if( db !== undefined )
   {
      dropTestCL( db, testConf );
      dropTestCS( db, testConf );
   }
}

// sub test
function subTest( func, testPara )
{
   var isExecSuccess = false;
   try
   {
      db.getRecycleBin().dropAll();
      commonSetUp( db, testConf );
      func( testPara );
      isExecSuccess = true;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         if( e.message === "standalone" ||
             e.message === "one data group" ||
             e.message === "one duplicate per group" ||
             e.message === "group less than three" ||
             e.message === "exist one node group" ||
             e.message === "skip test" )
         {
            return;
         }
         println( e.stack );
      }
      throw e;
   }
   finally
   {
      if( isExecSuccess )
      {
         subTearDown( db, testConf );
      }
   }
}

// loop with test on primary node and secondary node
function mainLoop( func, testPara )
{
   var isExecSuccess = false;
   try
   {
      testPara.usePrimary = true ;
      println( "test with primary node" ) ;
      subTest( func, testPara ) ;
      println( "test with secondary node" ) ;
      testPara.usePrimary = false ;
      subTest( func, testPara ) ;
      isExecSuccess = true;
   }
   finally
   {
      if ( isExecSuccess )
      {
         commonTearDown( db, testConf );
      }
      finiTestGroups( testConf.testGroups );
   }
}
