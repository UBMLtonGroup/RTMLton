exception E

fun H n = if n > 0 then raise E else 0

fun G n = H n handle _ => (print "exn\n"; 0)

fun F n = G n

val _ = F(1)
