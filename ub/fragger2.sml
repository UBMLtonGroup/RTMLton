(* this benchmark allocates single element array options till heap is full and
* deallocates half of them *)
fun timeme f x = 
          let
            val start = Time.toMicroseconds(Time.now())

            val arr2 = f x
          in
            (print
            ("\n"^IntInf.toString(Time.toMicroseconds(Time.now())-start) ^"\n") ; arr2 )
          end



fun heapstats () =
  let
    open MLton.GC
  in
    IntInf.toString(Statistics.numMarkCompactGCs ())
  end   


fun doit kArraySize =
  let open Array

            val arr = array(kArraySize,NONE)
            
            fun allocateNewArray size = 
                let
                    val arr2 = timeme array (size,SOME(Real.fromInt(10)))
                    fun traverseArray j = 
                            if j < (size)
                            then ((if (j mod 1000) =0 
                                    then update(arr2, j,
                                    SOME(Real.fromInt(j+1)))
                                    else ());
                                   traverseArray (j+1))
                            else ()
                in
                    traverseArray 0;
                    arr2
                end

            
            fun fillHeap i =
                if i < (kArraySize)
                    then (update (arr, i, SOME(Real.fromInt(i)));
                          fillHeap (i + 1))
                    else ()
              

            fun fragmentHeap i =
                if i < kArraySize 
                  then (update (arr, i, NONE);
                        fragmentHeap (i + 2))
                  else ()

            fun traverseArray ls i = 
              if (i < (length ls))
                then (sub (ls,i);
                     traverseArray ls (i+1))
                else ()
                              
  in
    let open MLton.GC
    in
        

        fillHeap 0;
       (* print(Int.toString(length ls)^ "\n");*)
        (*allocateNewArray (990000);*)
        
       (*print(heapstats ());*)
       fragmentHeap 0 ;
       (*print(heapstats ());*)


       allocateNewArray(kArraySize*2);
       (*traverseArray (allocateNewArray(kArraySize*2)) 0 ;*)
       (*print(heapstats ()^"\n");*)

        (* Keep large array alive with fake reference*)
        if (let val n = Int.min (1000, (length(arr) div 2) - 1)
           in Real.!= ( let val v= sub (arr, n) in if Option.isSome v then
             Option.valOf v else 0.0 end, (1.0 / Real.fromInt(n)))
           end)
        then print "Failed\n"
        else print "Success\n"
    end
  end

(*8660000  filled array allocates 207,853,836 bytes*)
(*8669770 : ub/fragger @MLton max-heap 204793k  gc-summary --  for MLton *)
val _ = doit 1000000 (*(Option.valOf(Int.fromString(hd (CommandLine.arguments()))))*)

