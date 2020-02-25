val n = valOf (Int.fromString (hd (CommandLine.arguments ())))


exception A
fun y z = raise A
fun x z = y z*3 
fun w z = x z*10 
fun v z = w z*5 handle A => 1
val _ = v n
