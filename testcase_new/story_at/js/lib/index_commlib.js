import( "../lib/basic_operation/commlib.js" );
import( "../lib/main.js" );
import( "../lib/func.js" );


function checkIndexCover ( explain, expResult )
{
   while( explain.next() )
   {
      var result = explain.current().toObj();
      var actResult = result.IndexCover;
      assert.equal( expResult, actResult );
   }
}

