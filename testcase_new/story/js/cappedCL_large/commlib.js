import( "../lib/basic_operation/commlib.js" );
import( "../lib/main.js" );

/************************************
*@Description: check count
*@author:      zhaoyu
*@createDate:  2017.7.18
**************************************/
function checkCount ( dbcl, findConf, expectCount )
{
   var actualCount = dbcl.count( findConf );
   assert.equal( actualCount, expectCount );
}

/************************************
*@Description: check logical ID
*@author:      zhaoyu
*@createDate:  2017.7.13
**************************************/
function checkLogicalID ( dbcl, findConf, selectConf, sortConf, limitConf, skipConf, expectIDs )
{
   var logicalIDs = getLogicalID( dbcl, findConf, selectConf, sortConf, limitConf, skipConf );
   assert.equal( logicalIDs.length, expectIDs.length );
   for( var i = 0; i < expectIDs.length; i++ )
   {
      assert.equal( logicalIDs[i], expectIDs[i] );
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
   var cursor = dbcl.find( findConf, selectConf ).sort( sortConf ).limit( limitConf ).skip( skipConf );
   while( cursor.next() )
   {
      var logicalID = cursor.current().toObj()._id;
      logicalIDs.push( logicalID );
   }
   return logicalIDs;
}

/************************************
*@Description: insert datas
*@author:      zhaoyu
*@createDate:  2017.7.11
**************************************/
function insertFixedLengthDatas ( dbcl, recordNum, stringLength, string )
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
   assert.equal( actRecs.length, expRecs.length );

   //check every records every fields,expRecs as compare source
   for( var i in expRecs )
   {
      var actRec = actRecs[i];
      var expRec = expRecs[i];

      for( var f in expRec )
      {
         assert.equal( actRec[f], expRec[f] );
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
         assert.equal( actRec[f], expRec[f] );
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
            throw new Error( "check lsn time out" );
         }
      }
      else 
      {
         break;
      }
   }

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
