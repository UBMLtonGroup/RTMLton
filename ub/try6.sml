
exception A
fun y z = if z > 0 then raise A else 2; print "hi\n";
fun x z = y z handle A => 1
val _ = x 1
