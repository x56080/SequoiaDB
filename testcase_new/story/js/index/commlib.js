/*******************************************************************************
@Description : Create Index common functions
@Modify list :
               2014-5-20  xiaojun Hu  Init
*******************************************************************************/
import( "../lib/main.js" );

var csName = COMMCSNAME;
var clName = COMMCLNAME;

// common functions

//inspect the index is created success or not.
function inspecIndex ( cl, indexName, indexKey, keyValue, idxUnique, idxEnforced )
{
   var timeout = 10;
   var time = 0;
   println( "1" )
   try
   {
      var getIndex = new Boolean( true );
      getIndex = cl.getIndex( indexName );
      while( undefined == getIndex && time < timeout )
      {
         getIndex = cl.getIndex( indexName );
         ++time;
         sleep( 1000 );
      }
      //println(cl.getIndex( indexName )) ;
      var indexDef = getIndex.toString();
      indexDef = eval( '(' + indexDef + ')' );
      var index = indexDef["IndexDef"];
      if( keyValue != index["key"][indexKey] )
      {
         println( "Wrong index name or key value : " + index["key"][indexKey] );
         throw "ErrIdxValue";
      }
      if( idxUnique != index["unique"] )
      {
         println( "Wrong index unique : " + index["unique"] );
         throw "ErrIdxUnique";
      }
      if( idxEnforced != index["enforced"] )
      {
         println( "Wrong index enforced : " + index["enforced"] );
         throw "ErrIdxEnforced";
      }
      println( "Success to inspect index : " + indexName );

   }
   catch( e )
   {
      println( "argument value:'" + indexName + "','" + indexKey + "','" + keyValue + "','" + idxUnique + "','" + idxEnforced );
      println( "Failed to inspect index : " + indexName + " rc=: " + e );
      throw e;
   }
}


function createIndex ( cl, idxName, idxKeygen, unique, enforced, errno )
{
   if( undefined == unique ) { unique = false; }
   if( undefined == enforced ) { enforced = false; }
   if( undefined == errno ) { errno = ""; }
   try
   {
      if( undefined == cl || undefined == idxName || undefined == idxKeygen )
      {
         println( "please check the argument of createIndex" );
         throw "ErrArg";
      }
      cl.createIndex( idxName, idxKeygen, unique, enforced );
      // inspect the index we created
   }
   catch( e )
   {
      if( errno != e )
      {
         println( "failed to create index, rc = " + e );
         throw e;
      }
   }
}

//inspect the index is created success or not.
function inspecIndex ( cl, indexName, indexKey, keyValue, idxUnique, idxEnforced )
{
   if( undefined == idxUnique ) { idxUnique = false; }
   if( undefined == idxEnforced ) { idxEnforced = false; }
   try
   {
      if( undefined == cl || undefined == indexName || undefined == indexKey || undefined == keyValue )
      {
         println( " wrong argument when inspect index " );
         throw "ErrArg";
      }
      var getIndex = new Boolean( true );
      try
      {
         getIndex = cl.getIndex( indexName );
      }
      catch( e )
      {
         getIndex = undefined;
      }
      println( "2" )
      var cnt = 0;
      while( cnt < 20 )
      {
         try
         {
            getIndex = cl.getIndex( indexName );
         }
         catch( e )
         {
            getIndex = undefined;
         }
         if( undefined != getIndex )
         {
            break;
         }
         ++cnt;
      }
      if( undefined == getIndex )
      {
         println( "Don't have the index, name = " + indexName );
         throw "ErrIdxName";
      }
      //println(cl.getIndex( indexName )) ;
      var indexDef = getIndex.toString();
      indexDef = eval( '(' + indexDef + ')' );
      var index = indexDef["IndexDef"];

      if( keyValue != index["key"][indexKey] )
      {
         println( "Wrong index name or key value : " + index["key"][indexKey] );
         throw "ErrIdxValue";
      }
      if( idxUnique != index["unique"] )
      {
         println( "Wrong index unique : " + index["unique"] );
         throw "ErrIdxUnique";
      }
      if( idxEnforced != index["enforced"] )
      {
         println( "Wrong index enforced : " + index["enforced"] );
         throw "ErrIdxEnforced";
      }
      println( "Success to inspect index : " + indexName );
   }
   catch( e )
   {
      println( "argument value:'" + indexName + "','" + indexKey + "','" + keyValue + "','" + idxUnique + "','" + idxEnforced );
      println( "Failed to inspect index : " + indexName + " rc=: " + e );
      throw e;
   }
}

/****************************************************
@description: check the scanType of the explain
@modify list:
              2016-3-3 yan WU init
****************************************************/
function checkExplain ( CL, keyValue )
{
   listIndex = CL.find( keyValue ).explain()
   var scanType = listIndex.current().toObj()["ScanType"];
   println( "test = " + scanType );
   var expectType = "ixscan";
   if( expectType != scanType )
   {
      throw buildException( "testFindByIndex", "check scanType", "check scanType is wrong", expectType, scanType );
   }
}

/****************************************************
@description: check the result of query
@modify list:
              2016-3-3 yan WU init
****************************************************/
function checkResult ( idxCL, keyValue )
{
   var rc = idxCL.find( keyValue );
   var expRecs = [];
   expRecs.push( keyValue );
   checkRec( rc, expRecs );
}

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

   //check every records every fields
   for( var i in expRecs )
   {
      var actRec = actRecs[i];
      var expRec = expRecs[i];
      for( var f in expRec )
      {
         if( JSON.stringify( actRec[f] ) !== JSON.stringify( expRec[f] ) )
         {
            println( "\nerror occurs in " + ( parseInt( i ) + 1 ) + "th record, in field '" + f + "'" );
            println( "\nactual recs in cl= " + JSON.stringify( actRecs ) + "\n\nexpect recs= " + JSON.stringify( expRecs ) );
            throw buildException( "checkRec()", "rec ERROR" );
         }
      }
   }
}

/* ****************************************************
@description: turn to local time
@parameter:
   time: Timestamp with time zone to millisecond,eg:'1901-12-31T15:54:03.000Z'
   format: eg:%Y-%m-%d-%H.%M.%S.000000
@return: 
   localtime, eg: '1901-12-31-15.54.03.000000'
**************************************************** */
function turnLocaltime ( time, format )
{
   if( typeof ( format ) == "undefined" ) { format = "%Y-%m-%d"; };
   try
   {
      var msecond = new Date( time ).getTime();
      var second = parseInt( msecond / 1000 );  //millisecond to second
      var localtime = cmdRun( 'date -d@"' + second + '" "+' + format + '"' );

      return localtime;
   }
   catch( e )
   {
      println( "Timestamp with time zone to local time failed, time[" + time + "], second[" + second + "]" );
      throw e;
   }
}

function cmdRun ( str )
{
   try
   {
      var cmd = new Cmd();
      var rc = cmd.run( str ).split( "\n" )[0];
      return rc;
   }
   catch( e )
   {
      println( "Failed to exec cmd run." );
      throw e;
   }
}
