fun gcbench kStretchTreeDepth =

  let open Int

      fun expt (m:int, n:int) =
        if n = 0 then 1 else m * expt (m, n - 1)
  
      (*  Nodes used by a tree of a given size  *)
      fun TreeSize2 i =
        expt (2, i + 1) - 1
  
      fun TreeSize i =
         (* (1 << (i + 1)) - 1 *)
         IntInf.toInt(IntInf.<< (IntInf.fromInt 1, Word.fromInt (i+1))) - 1

      (*  Number of iterations to use for a given tree depth  *)
      fun NumIters i =
        (2 * (TreeSize kStretchTreeDepth)) div (TreeSize i)

      val kLongLivedTreeDepth = kStretchTreeDepth - 2
      val kArraySize          = 4 * (TreeSize kLongLivedTreeDepth)
      
      fun main () =
        (
        print "Garbage Collector Test\n";
        print (concat [" Stretching memory with a binary tree of depth ",
                       toString kStretchTreeDepth,
                       "\n"]);
        print (Int.toString(TreeSize2 kStretchTreeDepth))
        )
  in main()
  end



structure Main =
struct
  fun testit out = TextIO.output (out, "OK\n")
  fun doit () = gcbench 18
end


val _ = Main.doit ()

val _ = print "Done\n"
