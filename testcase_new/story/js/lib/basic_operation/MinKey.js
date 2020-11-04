var tmpMinKey = {
   help: MinKey.prototype.help,
   toString: MinKey.prototype.toString
};
var funcMinKey = MinKey;
var funcMinKeyhelp = MinKey.help;
MinKey=function(){try{return funcMinKey.apply( this, arguments ); } catch( e ) { var msg = e.message || e; throw new Error(msg) } };
MinKey.help = function(){try{ return funcMinKeyhelp.apply( this, arguments ); } catch( e ) { var msg = e.message || e; throw new Error(msg) } };
MinKey.prototype.help=function(){try{return tmpMinKey.help.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
MinKey.prototype.toString=function(){try{return tmpMinKey.toString.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
