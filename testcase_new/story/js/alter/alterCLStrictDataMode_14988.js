/* *****************************************************************************
@discretion: cl alter strictDataMode
@author£º2018-4-26 wuyan  Init
***************************************************************************** */
var clName = CHANGEDPREFIX + "_alterclstrictDataMode_14988"; 

main( db ); 
function main( db )
{
   try
   {
      if( true == commIsStandalone( db ) )
      {
         println( "run mode is standalone" ); 
         return; 
      }
      //clean environment before test
      commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" ); 
      
      //create cl
      var dbcl = commCreateCL( db, COMMCSNAME, clName ); 
      
      //insert data
      dbcl.insert( {no:{ '$numberLong' : '-9223372036854775808'}, a:1} ); 
      
      //alter cl strictDataMode is true, numoverflow error
      println( "---alter strictDataMode to true" ); 
      var strictDataMode = true; 
      dbcl.setAttributes( {StrictDataMode:strictDataMode} ); 
      checkResult( clName, "AttributeDesc", "Compressed | StrictDataMode" ); 
      numoverflowError( dbcl ); 
      
      //alter cl strictDataMode is false, numoverflow error
      println( "---alter strictDataMode to false" ); 
      var strictDataMode1 = false; 
      dbcl.setAttributes( {StrictDataMode:strictDataMode1} ); 
      checkResult( clName, "AttributeDesc", "Compressed" ); 
      numoverflowConversion( dbcl )
      
      //clean
      //commDropCL( db, COMMCSNAME, clName, true, true, "clear collection in the beginning" ); 
   }
   catch( e )
   {
      throw buildException( "alter  fail:", e ); 
   }
   finally
   {
      if( db != null )
      {
         db.close()
      }
   }
}

function numoverflowError( dbcl )
{
   try
   {
      var rc = dbcl.find( {}, { "no": { "$abs": 1 }} ).next(); 
      throw "need throw error"; 
   }
   catch( e )
   {
      if( e != -318 )
      {
         throw buildException( "numoverflow should be fail:", e ); 
      }
   }
}

function numoverflowConversion( dbcl )
{
   var rc = dbcl.find( {}, { "no": { "$abs": 1 }, "_id":{"$include":0}} ); 
   var actRecs = []; 
   while( rc.next() )
   {
      actRecs.push( rc.current().toObj() ); 
   }
   
   var expRecs = []; 
   expRecs.push( {"no":{"$decimal":"9223372036854775808"}, "a":1} ); 
   if( JSON.stringify( actRecs )!== JSON.stringify( expRecs ) )
   {
      throw buildException( "checkRec()", "rec ERROR", "", JSON.stringify( expRecs ), JSON.stringify( actRecs ) ); 
   }
}

function checkResult( clName, fieldName, expFieldValue )
{
   var clFullName = COMMCSNAME + "." + clName; 
   var cur = db.snapshot( 8, {"Name":clFullName} ); 
   var actualFieldValue = cur.current().toObj()[fieldName]; 
   
   if( expFieldValue !== actualFieldValue )
   {
      throw buildException( "test fieldvalue", "check field", "value is wrong", expFieldValue, actualFieldValue ); 
   }
}
