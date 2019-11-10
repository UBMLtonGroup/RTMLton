val n = valOf (Int.fromString (hd (CommandLine.arguments ())))
val m = valOf (Int.fromString (hd (tl (CommandLine.arguments ()))))

exception E
fun loop i =
   if i = n
      then raise E
   else if i = 0
      then n
   else 1 + loop(i - 1)

val r = loop m handle E => 13

val _ = print (concat ["r = ", Int.toString r, "\n"])

