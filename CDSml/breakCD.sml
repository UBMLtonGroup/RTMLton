let open Array
        val kArraySize =70000000 (*Close to 1.12 gigs with the array filled with
        real option objects*)
        val arr = array (kArraySize, NONE)
        fun loop1 i =
          if i < (kArraySize)
            then (update (arr, i, SOME(Real.fromInt(i)));
                  loop1 (i + 1))
            else ()

        fun loop2 i =
          if i < (kArraySize div 2)
            then (update (arr, i, NONE);
                  loop1 (i + 1))
            else ()


        fun timeme f x = 
          let
            val start = Time.toMicroseconds(Time.now())
          in
            (f x); IntInf.toString(Time.toMicroseconds(Time.now())-start)
          end
in
    (
        print ("Time to fill array = "^ (timeme loop1 0) ^"\n");

        print ("Time to empty half array = "^ (timeme loop2 0) ^"\n");
        
        if (let val n = Int.min (1000, (length(arr) div 2) - 1)
           in Real.!= ( let val v= sub (arr, n) in if Option.isSome v then
             Option.valOf v else 0.0 end, (1.0 / Real.fromInt(n)))
           end)
        then print "Failed\n"
        else print "Success\n"
        (*  fake reference to array to keep them from being optimized away
        *)
    )
end;

val _ = print "Done\n"
