var tmpNumberLong = {
   help: NumberLong.prototype.help,
   toString: NumberLong.prototype.toString,
   valueOf: NumberLong.prototype.valueOf
};
var funcNumberLong = NumberLong;
var funcNumberLonghelp = NumberLong.help;
NumberLong=function(){try{return funcNumberLong.apply( this, arguments ); } catch( e ) { var msg = e.message || e; throw new Error(msg) } };
NumberLong.help = function(){try{ return funcNumberLonghelp.apply( this, arguments ); } catch( e ) { var msg = e.message || e; throw new Error(msg) } };
NumberLong.prototype.help=function(){try{return tmpNumberLong.help.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
NumberLong.prototype.toString=function(){try{return tmpNumberLong.toString.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
NumberLong.prototype.valueOf=function(){try{return tmpNumberLong.valueOf.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
