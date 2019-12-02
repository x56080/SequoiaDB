/* *****************************************************************************
@Description: Insert common functions
@Modify list:
2014-3-1 Jianhui Xu  Init
***************************************************************************** */

//inspect the index is created success or not.
function inspecIndex( cl, indexName, indexKey, keyValue, idxUnique, idxEnforced )
{
   var timeout = 10; 
   var time = 0; 
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
      //println( cl.getIndex( indexName ) ); 
      var indexDef = getIndex.toString(); 
      indexDef = eval( '( ' + indexDef + ' )' ); 
      var index = indexDef[ "IndexDef" ]; 
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
      println( "argument value:'" + indexName + "', '" + indexKey + "', '" + keyValue + "', '" + idxUnique + "', '" + idxEnforced ); 
      println( "Failed to inspect index : " + indexName + " rc=: " + e ); 
      throw e; 
   }
}
