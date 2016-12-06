open MLton.PrimThread

(* test moving threads between queues *)

fun before_after (x, pri) = let
in
  print "Queue before setting priority:\n" ;
  displayThreadQueue(0) ;
  setPriority(x, pri) ;
  print "\nQueue after setting priority:\n";
  displayThreadQueue(0)
end

local
  val () = MLton.PrimThread.PThread.copyCurrent()
  val sp1 = MLton.PrimThread.PThread.savedPre()
  val _ = before_after(sp1, 5)

  val () = MLton.PrimThread.PThread.copyCurrent()
  val sp2 = MLton.PrimThread.PThread.savedPre()
  val _ = before_after(sp2, 5)

  val () = MLton.PrimThread.PThread.copyCurrent()
  val sp3 = MLton.PrimThread.PThread.savedPre()
  val _ = before_after(sp3, 5)

  val () = MLton.PrimThread.PThread.copyCurrent()
  val sp4 = MLton.PrimThread.PThread.savedPre()
  val _ = before_after(sp4, 5)

  val () = MLton.PrimThread.PThread.copyCurrent()
  val sp5 = MLton.PrimThread.PThread.savedPre()
  val _ = before_after(sp5, 5)

  val _ = before_after(sp4, 2)
  val _ = before_after(sp3, 3)
  val _ = before_after(sp1, 2)
  val _ = before_after(sp2, 2)
in
end

val rec delay =
   fn 0 => ()
    | n => delay (n - 1)
    
val rec loop =
   fn 0 => ()
    | n => (delay 50000000; loop (n - 1))

val _ = loop 100

val _ = print "Done\n"
