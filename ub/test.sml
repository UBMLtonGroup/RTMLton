val _ = print "ubtest start\n"


val rec delay =
   fn 0 => ()
    | n => delay (n - 1)
    
val rec loop =
   fn 0 => ()
    | n => (print ("loop" ^ Int.toString(n) ^"\n"); delay 50000000; loop (n - 1))

val _ = loop 100

val _ = print "ubtest finish\n"
