val cmd = CommandLine.name ()
val args = CommandLine.arguments ()

fun intFromString(s) = case Int.fromString(s) of SOME x => x | NONE => 0;

fun fib n = 
    let
    fun fib' (0,a,b) = a
      | fib' (n,a,b) = fib' (n-1,a+b,a)
    in
    fib' (n,0,1)
    end


val v =
  if List.length args = 0
  then let
       in
          print cmd ; print " [depth]\n" ;
          0
       end
  else fib (intFromString(hd(args)))

val _ = print (Int.toString(v))
val _ = print "\n"
