/*******************************************************************************
@Description : Create idIndex common functions
@Modify list :
               2016-8-10  wuyan  Init
*******************************************************************************/
function createIdIndex ( cl, sortBufferSize, errno )
{
   if( undefined == sortBufferSize ) { sortBufferSize = null; }
   if( undefined == errno ) { errno = ""; }
   try
   {
      if( undefined == cl )
      {
         println( "please check the argument of createIdIndex" );
         throw "ErrArg";
      }
      cl.createIdIndex( sortBufferSize );
      // inspect the index we created
   }
   catch( e )
   {
      if( errno != e )
      {
         println( "failed to create idIndex, rc = " + e );
         throw e;
      }
   }
}

//inspect the index is created success or not.
function inspecIndex ( cl, indexName, indexKey, keyValue )
{
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
      if( true != index["unique"] )
      {
         println( "Wrong index unique : " + index["unique"] );
         throw "ErrIdxUnique";
      }
      if( true != index["enforced"] )
      {
         println( "Wrong index enforced : " + index["enforced"] );
         throw "ErrIdxEnforced";
      }
      println( "Success to inspect index : " + indexName );
   }
   catch( e )
   {
      println( "argument value:'" + indexName + "','" + indexKey + "','" + keyValue );
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
   //println("listIndex="+listIndex)
   var scanType = listIndex.current().toObj()["ScanType"];
   println( "test = " + scanType );
   var expectType = "ixscan";
   if( expectType != scanType )
   {
      throw buildException( "checkExplain()", "check scanType", "check scanType is wrong", expectType, scanType );
   }
}

/****************************************************
@description: check the result of query
@modify list:
              2016-8-10 yan WU init
****************************************************/
function checkCLData ( expRecs, rc )
{
   println( "\n---Begin to check cl data." );

   var recsArray = [];
   while( rc.next() )
   {
      recsArray.push( rc.current().toObj() );
   }
   //var expRecs = '[{"a":1},{"a":2}]';
   var actRecs = JSON.stringify( recsArray );
   if( actRecs !== expRecs )
   {
      throw buildException( "checkCLdata", null, "[find]",
         "[recs:" + expRecs + "]",
         "[recs:" + actRecs + "]" );
   }
   println( "cl records: " + actRecs );
}
