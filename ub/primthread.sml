open MLton.PrimThread

fun loop 0 = 0
  | loop (n:int) = 
  let
	in
	    MLton.PrimThread.PThread.copyCurrent() ;
	    loop(n - 1) 
	end


(* val _ = loop 10 ignore *)

val () = MLton.PrimThread.PThread.copyCurrent() 

val sp = MLton.PrimThread.PThread.savedPre() 

val p = setPriority(sp, 5)