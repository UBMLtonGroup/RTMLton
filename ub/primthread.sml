open MLton.PrimThread

val x = primthreadtest("hihi\n")
val _ = print (Int.toString(x))
val _ = print "\n"

val sp = MLton.PrimThread.Thread.savedPre() 
