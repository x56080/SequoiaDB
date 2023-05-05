

var dropCS = function(db, csName) {
    try {
        db.dropCS(csName);
    } catch (e) {
    }
    sleep(1000);
} ;
