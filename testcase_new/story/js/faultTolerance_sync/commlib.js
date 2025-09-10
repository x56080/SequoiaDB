import( "../lib/basic_operation/commlib.js" );
import( "../lib/main.js" );

function getRandomInt ( min, max ) // [min, max)
{
   var range = max - min;
   var value = min + parseInt( Math.random() * range );
   return value;
}

function getRandomString ( strLen )
{
   var str = "";
   for( var i = 0; i < strLen; i++ )
   {
      var ascii = getRandomInt( 48, 127 );
      var c = String.fromCharCode( ascii );
      str += c;
   }
   return str;
}

function getSlaveReplSessionTID( slave )
{
   var cursor = slave.snapshot( 2, { "Type": "ReplAgent" }, { "TID": '' } );
   var tid = cursor.current().toObj().TID ;
   return tid;
}