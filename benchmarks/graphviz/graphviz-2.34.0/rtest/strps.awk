BEGIN{doit = 0}
 { if (doit) print $0 }
 $0 = /%%End.*Setup/ { doit = 1 }
