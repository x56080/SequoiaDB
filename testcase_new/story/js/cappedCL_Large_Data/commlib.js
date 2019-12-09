/************************************
*@Description: check count
*@author:      zhaoyu
*@createDate:  2017.7.18
**************************************/
function checkCount ( dbcl, findConf, expectCount )
{
   try
   {
      var actualCount = countRecords( dbcl, findConf );
      if( actualCount !== expectCount )
      {
         println( "actualCount: " + actualCount + ",expectCount: " + expectCount );
         throw "RECORD_COUNT_ERROR";
      }
   } catch( e )
   {
      throw buildException( "checkCount", e, null, null, e );
   }
   println( "check count success!" );
}

/************************************
*@Description: check logical ID
*@author:      zhaoyu
*@createDate:  2017.7.13
**************************************/
function checkLogicalID ( dbcl, findConf, selectConf, sortConf, limitConf, skipConf, expectIDs )
{
   try
   {
      var logicalIDs = getLogicalID( dbcl, findConf, selectConf, sortConf, limitConf, skipConf );
      if( logicalIDs.length !== expectIDs.length )
      {
         println( "actualIDsLength: " + logicalIDs.length + ",expectIDsLength: " + expectIDs.length );
         throw "LOGICAL_ID_COUNT_ERROR";
      }
      for( var i = 0; i < expectIDs.length; i++ )
      {
         if( logicalIDs[i] !== expectIDs[i] )
         {
            println( "prev " + logicalIDs[i - 1] + " prev " + expectIDs[i - 1] );
            println( "error occurs in the " + i + "th record," + "actualID:" + logicalIDs[i] + ",expectID: " + expectIDs[i] );
            throw "LOGICAL_ID_NOT_EQUAL";
         }
      }

   } catch( e )
   {
      throw buildException( "checkLogicalID", e, null, null, e );
   }

   println( "check logical id success!" );
}

/************************************
*@Description: return count record
*@author:      zhaoyu
*@createDate:  2017.7.11
**************************************/
function countRecords ( dbcl, conf )
{
   try
   {
      var count = dbcl.count( conf );
      return parseInt( count );
   } catch( e )
   {
      throw buildException( "count", e, null, null, e );
   }
}

/************************************
*@Description: insert data
*@author:      zhaoyu
*@createDate:  2017.7.12
**************************************/
function insertDatas ( dbcl, datas )
{
   try
   {
      dbcl.insert( datas );
   }
   catch( e )
   {
      throw buildException( "insertDatas()", e, null, null, e );
   }
}

/************************************
*@Description: get logical ID ,return array
*@author:      zhaoyu
*@createDate:  2017.7.11
**************************************/
function getLogicalID ( dbcl, findConf, selectConf, sortConf, limitConf, skipConf )
{
   var logicalIDs = [];
   try
   {
      var cursor = dbcl.find( findConf, selectConf ).sort( sortConf ).limit( limitConf ).skip( skipConf );
      while( cursor.next() )
      {
         var logicalID = cursor.current().toObj()._id;
         logicalIDs.push( logicalID );
      }
      return logicalIDs;
   } catch( e )
   {
      throw buildException( "get logical IDs", e, null, null, e );
   }
}

/************************************
*@Description: pop datas
*@author:      zhaoyu
*@createDate:  2017.7.11
**************************************/
function pop ( dbcl, logicalID, direction )
{
   try
   {
      println( "--pop logicalID: " + logicalID + ", Direction: " + direction )
      dbcl.pop( { LogicalID: logicalID, Direction: direction } );
   } catch( e )
   {
      throw buildException( "pop", e, null, null, e );
   }
}
/************************************
*@Description: insert datas
*@author:      zhaoyu
*@createDate:  2017.7.11
**************************************/
function insertFixedLengthDatas ( dbcl, recordNum, stringLength, string )
{
   try
   {
      var doc = new StringBuffer();
      doc.append( stringLength, string );
      var strings = doc.toString();

      var records = [];
      for( var i = 0; i < recordNum; i++ )
      {
         records.push( { a: strings } );
      }
      dbcl.insert( records );
      doc.clear();
   } catch( e )
   {
      throw buildException( "insertFixedLengthDatas", e, null, null, e );
   }

   return records;

}

/************************************
*@Description: generate strings
*@author:      zhaoyu
*@createDate:  2017.7.11
**************************************/
function StringBuffer ()
{
   this._strings = new Array();
}
StringBuffer.prototype.append = function( stringLength, string )
{
   for( var i = 0; i < stringLength; i++ )
   {
      this._strings.push( string );
   }
};
StringBuffer.prototype.toString = function()
{
   return this._strings.join( "" );
};
StringBuffer.prototype.clear = function()
{
   this._strings = [];
}
StringBuffer.prototype.size = function()
{
   return this._strings.length;
}


/************************************
*@Description: get actual result and check it 
*@author:      zhaoyu
*@createDate:  2017.7.11
**************************************/
function checkRecords ( dbcl, findConf, selectConf, sortConf, limitConf, skipConf, expRecs )
{
   if( typeof ( findConf ) == "undefined" ) { findConf = null; }
   if( typeof ( selectConf ) == "undefined" ) { selectConf = null; }
   if( typeof ( sortConf ) == "undefined" ) { sortConf = null; }
   if( typeof ( limitConf ) == "undefined" ) { limitConf = null; }
   if( typeof ( skipConf ) == "undefined" ) { skipConf = null; }
   var rc = dbcl.find( findConf, selectConf ).sort( sortConf ).limit( limitConf ).skip( skipConf );
   //println("--begin to check the data");
   checkRec( rc, expRecs );
   //println("--end check the data");
}

/************************************
*@Description: compare actual and expect result,
               they is not the same ,return error ,
               else return ok
*@author:      zhaoyu
*@createDate:  2015.5.20
**************************************/
function checkRec ( rc, expRecs )
{
   //get actual records to array
   var actRecs = [];
   while( rc.next() )
   {
      actRecs.push( rc.current().toObj() );
   }
   //check count
   if( actRecs.length !== expRecs.length )
   {
      println( "\nactual recs in cl= " + JSON.stringify( actRecs ) + "\n\nexpect recs= " + JSON.stringify( expRecs ) );
      throw buildException( "check count", null, "",
         expRecs.length, actRecs.length );
   }

   //check every records every fields,expRecs as compare source
   for( var i in expRecs )
   {
      var actRec = actRecs[i];
      var expRec = expRecs[i];

      for( var f in expRec )
      {
         if( JSON.stringify( actRec[f] ) !== JSON.stringify( expRec[f] ) )
         {
            println( "\nerror occurs in " + ( parseInt( i ) + 1 ) + "th record, in field '" + f + "'" );
            println( "\nactual record= " + JSON.stringify( actRec ) + "\n\nexpect record= " + JSON.stringify( expRec ) );
            //println("\nactual recs in cl= "+JSON.stringify(actRecs)+"\n\nexpect recs= "+JSON.stringify(expRecs));   		
            throw buildException( "checkRec()", "rec ERROR" );
         }
      }
   }
   //check every records every fields,actRecs as compare source
   for( var j in actRecs )
   {
      var actRec = actRecs[j];
      var expRec = expRecs[j];

      for( var f in actRec )
      {
         if( f == "_id" )
         {
            continue;
         }
         if( JSON.stringify( actRec[f] ) !== JSON.stringify( expRec[f] ) )
         {
            println( "\nerror occurs in " + ( parseInt( j ) + 1 ) + "th record, in field '" + f + "'" );
            println( "\nactual record= " + JSON.stringify( actRec ) + "\n\nexpect record= " + JSON.stringify( expRec ) );
            //println("\nactual recs in cl= "+JSON.stringify(actRecs)+"\n\nexpect recs= "+JSON.stringify(expRecs));   		
            throw buildException( "checkRec()", "rec ERROR" );
         }
      }
   }
}


/************************************
*@Description: 获取所有组的数据节点
*@author:      zhaoyu 
*@createDate:  2017.11.8
**************************************/
function getNodesInGroups ( db, groups )
{
   var datas = new Array();

   //standalone
   if( true === commIsStandalone( db ) )
   {
      datas[0] = Array();
      datas[0][0] = db;
   } else
   {
      for( var i = 0; i < groups.length; ++i )
      {
         datas[i] = Array();

         var rg = db.getRG( groups[i] );
         var rgDetail = eval( "(" + rg.getDetail().toArray()[0] + ")" );
         var nodesInGroup = rgDetail.Group;
         for( var j = 0; j < nodesInGroup.length; ++j )
         {
            var hostName = nodesInGroup[j].HostName;
            var serviceName = nodesInGroup[j].Service[0].Name;
            datas[i][j] = new Sdb( hostName, serviceName );
         }

      }
   }
   return datas;
}

/************************************
*@Description: 获取主节点的lsn
*@author:      liuxiaoxuan
*@createDate:  2017.01.29
**************************************/
function getPrimaryNodeLSNs ( db, groups )
{

   var datas = getNodesInGroups( db, groups );

   var LSNs = new Array();
   for( var i = 0; i < datas.length; ++i )
   {
      var nodesInGroup = datas[i];
      LSNs[i] = Array();
      for( var j = 0; j < nodesInGroup.length; ++j )
      {
         var getSnapshot6 = eval( "(" + nodesInGroup[j].snapshot( 6 ).toArray()[0] + ")" );

         var completeLSN = getSnapshot6.CompleteLSN;
         var isPrimary = getSnapshot6.IsPrimary;
         if( isPrimary )
         {
            LSNs[i][0] = completeLSN;
            break;
         }
      }
   }

   return LSNs;
}

/************************************
*@Description: 获取备节点的lsn
*@author:      liuxiaoxuan
*@createDate:  2017.01.29
**************************************/
function getSlaveNodeLSNs ( db, groups )
{
   var datas = getNodesInGroups( db, groups );

   var LSNs = new Array();
   for( var i = 0; i < datas.length; ++i )
   {
      var nodesInGroup = datas[i];
      LSNs[i] = Array();
      var f = 0;
      for( var j = 0; j < nodesInGroup.length; ++j )
      {
         var getSnapshot6 = eval( "(" + nodesInGroup[j].snapshot( 6 ).toArray()[0] + ")" );

         var completeLSN = getSnapshot6.CompleteLSN;
         var isPrimary = getSnapshot6.IsPrimary;
         if( !isPrimary )
         {
            LSNs[i][f++] = completeLSN;
         }
      }
   }

   return LSNs;
}

/************************************
*@Description: 检查cl所在组的主备节点lsn是否一致(超时600s)
*@author:      liuxiaoxuan
*@createDate:  2017.01.29
**************************************/
function checkConsistency ( db, csName, clName, groups )
{
   if( csName == null ) { var csName = "UNDEFINED"; }
   if( clName == null ) { var clName = "UNDEFINED"; }
   if( groups == undefined ) { var groups = commGetCLGroups( db, csName + "." + clName ); }

   //the longest waiting time is 600S
   var lsnFlag = false;
   var timeout = 600;
   var doTimes = 0;

   //get primary nodes
   var primaryNodeLSNs = getPrimaryNodeLSNs( db, groups );
   while( true )
   {
      lsnFlag = checkLSN( db, groups, primaryNodeLSNs );
      if( !lsnFlag )
      {
         if( doTimes < timeout )
         {
            ++doTimes;
            sleep( 1000 );
         }
         else
         {
            throw "check lsn time out";
         }
      }
      else 
      {
         break;
      }
   }
   println( "check lsn ok!" );

}

/************************************
*@Description: 获取当前主备节点lsn是否一致的状态
*@author:      zhaoyu
*@createDate:  2017.11.8
**************************************/
function checkLSN ( db, groups, primaryNodeLSNs )
{
   var slaveNodeLSNs = getSlaveNodeLSNs( db, groups );

   var checkLSN = true;
   //比较主备节点lsn
   for( var i = 0; i < slaveNodeLSNs.length; ++i )
   {
      for( var j = 0; j < slaveNodeLSNs[i].length; ++j )
      {
         if( primaryNodeLSNs[i][0] > slaveNodeLSNs[i][j] )
         {
            checkLSN = false;
            return checkLSN;
         }
      }
   }

   return checkLSN;
}
