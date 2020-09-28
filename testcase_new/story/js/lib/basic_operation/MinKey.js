var tmpMinKey = {
   help: MinKey.prototype.help,
   toString: MinKey.prototype.toString
};
var funcMinKey = MinKey;
var funchelp = MinKey.help;
MinKey=function(){try{return funcMinKey.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
MinKey.help = function(){try{ return funchelp.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
MinKey.prototype.help=function(){try{return tmpMinKey.help.apply(this,arguments);}catch(e){commThrowError(e);}};
MinKey.prototype.toString=function(){try{return tmpMinKey.toString.apply(this,arguments);}catch(e){commThrowError(e);}};
