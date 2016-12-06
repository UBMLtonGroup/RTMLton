(* at this point, basis is booting, once callFromCHandler
   is set, the following SML code will execute via
   the trampoline in posix thread 0. If you do not
   see the print output, then the trampoline isnt working *)



datatype Tree = Dummy
              | Node of { left: Tree ref, right: Tree ref, i: int, j: int }

fun make_empty_node () =
  Node { left= ref Dummy, right= ref Dummy, i= 0, j= 0 }

fun make_node (l, r) =
  Node { left= ref l, right= ref r, i= 0, j= 0 }

fun PrintDiagnostics () = ()

fun gcbench kStretchTreeDepth =

  let open Int

      fun expt (m:int, n:int) =
        if n = 0 then 1 else m * expt (m, n - 1)
  
      (*  Nodes used by a tree of a given size  *)
      fun TreeSize i =
        expt (2, i + 1) - 1
  
      (*  Number of iterations to use for a given tree depth  *)
      fun NumIters i =
        (2 * (TreeSize kStretchTreeDepth)) div (TreeSize i)

      (*
      ;  Parameters are determined by kStretchTreeDepth.
      ;  In Boehm's version the parameters were fixed as follows:
      ;    public static final int kStretchTreeDepth    = 18;  // about 16Mb
      ;    public static final int kLongLivedTreeDepth  = 16;  // about 4Mb
      ;    public static final int kArraySize  = 500000;       // about 4Mb
      ;    public static final int kMinTreeDepth = 4;
      ;    public static final int kMaxTreeDepth = 16;
      ;  In Larceny the storage numbers above would be 12 Mby, 3 Mby, 6 Mby.
      *)

      val kLongLivedTreeDepth = kStretchTreeDepth - 2
      val kArraySize          = 4 * (TreeSize kLongLivedTreeDepth)
      val kMinTreeDepth       = 4
      val kMaxTreeDepth       = kLongLivedTreeDepth
    
      (*  Build tree top down, assigning to older objects.  *)
      fun Populate (iDepth, Node { left=lr, right=rr, i, j }) =
        if iDepth <= 0
            then false
            else let val iDepth = iDepth - 1
                 in
                 (
                     lr := make_empty_node();
                     rr := make_empty_node();
                     Populate (iDepth, !lr);
                     Populate (iDepth, !rr)
                 )
                 end
      
      (*  Build tree bottom-up  *)
      fun MakeTree iDepth =
        if iDepth <= 0
            then make_empty_node()
            else make_node (MakeTree (iDepth - 1),
                            MakeTree (iDepth - 1))
      
      fun TimeConstruction depth =
        let val iNumIters = NumIters depth
        in
        (
          print (concat ["Creating ",
                         toString iNumIters,
                         " trees of depth ",
                         toString depth,
                         "\n"]);
          let fun loop i =
                if i < iNumIters
                  then (Populate (depth, make_empty_node());
                        loop (i + 1))
                  else ()
          in loop 0
          end;
          let fun loop i =
                if i < iNumIters
                  then (MakeTree depth;
                        loop (i + 1))
                  else ()
          in loop 0
          end
        )
        end
      
      fun main () =
        (
        print "Garbage Collector Test\n";
        print (concat [" Stretching memory with a binary tree of depth ",
                       toString kStretchTreeDepth,
                       "\n"]);
        PrintDiagnostics();
        (*  Stretch the memory space quickly  *)
        MakeTree kStretchTreeDepth;
                         
        (*  Create a long lived object  *)
        print (concat[" Creating a long-lived binary tree of depth ",
                      toString kLongLivedTreeDepth,
                      "\n"]);
        let val longLivedTree = make_empty_node()
        in
        (
          Populate (kLongLivedTreeDepth, longLivedTree);
          
          (*  Create long-lived array, filling half of it  *)
          print (concat [" Creating a long-lived array of ",
                         toString kArraySize,
                         " inexact reals\n"]);
          let open Array
              val arr = array (kArraySize, 0.0)
              fun loop1 i =
                if i < (kArraySize div 2)
                  then (update (arr, i, 1.0/(Real.fromInt(i)));
                        loop1 (i + 1))
                  else ()
              fun loop2 d =
                if d <= kMaxTreeDepth
                  then (TimeConstruction d;
                        loop2 (d + 2))
                  else ()
          in
          (
            loop1 0;
            PrintDiagnostics();

            loop2 kMinTreeDepth;

            if (longLivedTree = Dummy)
               orelse
               let val n = min (1000, (length(arr) div 2) - 1)
               in Real.!= (sub (arr, n), (1.0 / Real.fromInt(n)))
               end
              then print "Failed\n"
              else ()
            (*  fake reference to LongLivedTree
                and array to keep them from being optimized away
            *)
          )
          end)
        end;
        PrintDiagnostics())
  in main()
  end

(*
fun main () =
  run_benchmark ("gcbench",
                 1,
                 fn () => gcbench 18,
                 fn (result) => true)
*)
structure Main =
struct
  fun testit out = TextIO.output (out, "OK\n")
  fun doit () = gcbench 18
end


fun printTime () = TextIO.output (TextIO.stdOut, "Time: " ^ Time.toString (Time.now())  ^  "\n")
open MLton 
open PrimThread
open MLton.Thread

val a = 9320


fun fib n =
  if n < 3 then 
    1
  else
    fib (n-1) + fib (n-2)

fun printfib n = print ( Int.toString (fib (n)) ^ "\n ************** \n" )


(*val _ = MLton.Thread.spawn (fn () => printfib (10))*)

(*val _ = MLton.Thread.spawn(fn () => gcbench 18)*)


open OS.Process
fun inf_sleep t= OS.Process.sleep t



fun dogcbench () = (printTime(); gcbench(18); printTime())

val rec loop =
   fn 0 => ()
    | n => (print (Int.toString n ^ "\n"); dogcbench(); loop (n - 1))

val _ = loop 1


val _ = PrimThread.setBooted()

val _ = print "test2 was running and is going to sleep\n"
fun pause () = (print "sleep\n"; inf_sleep (Time.fromSeconds 1)
                ; PrimThread.gcSafePoint(); pause ())

val _ = pause ()


