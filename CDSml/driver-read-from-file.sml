
structure Driver = 
struct


    

  (*To do read properly from file*)
    datatype for = to of int * int
                    | downto of int * int

    infix to downto

    val for =
        fn lo to up =>
            (fn f => let fun loop lo = if lo > up then ()
                                  else (f lo; loop (lo+1))
                     in loop lo end)
        | up downto lo =>
            (fn f => let fun loop up = if up < lo then ()
                                  else (f up; loop (up-1))
                     in loop up end)


    fun sleepForPeriod(t) =
     OS.Process.sleep t 


    val periodint = Int.toLarge(50)
    val period = Time.fromMilliseconds periodint;  



        


    fun printBenchmarkResults([] : IntInf.int list,i: int) = print("Detector completed successfully\n")
      | printBenchmarkResults(x ::xs,i ) =
      let
        val cnt = i+1
      in 
        if (x >= Time.toMicroseconds(period)) then
          (print("Task "^ Int.toString(i)^" missed the deadline\n");
            printBenchmarkResults(xs,cnt ))
        else
          printBenchmarkResults(xs,cnt)
      end


    

    fun benchmarkCD ts tc tr  =
    let
      
     fun min_arr arr = Array.foldl IntInf.min (Array.sub(arr,0)) arr 
     fun max_arr arr = Array.foldl IntInf.max (Array.sub(arr,0)) arr

     fun avg_arr arr = IntInf.div( (Array.foldl (op +) (Int.toLarge(0)) arr),
       Int.toLarge(Array.length(arr)) )

      fun std_dev arr = 
      let
        val mean = avg_arr arr
        val newarr = (Array.modify (fn i => (i-mean)*(i-mean)) arr;arr);
        fun meanofsqdiff newarr = avg_arr newarr
      in
        Math.sqrt(Real.fromLargeInt(meanofsqdiff arr))
      end 



      val responseList =  Array.array(Array.length(tr),Time.toMicroseconds(Time.zeroTime))
      val compList =  Array.array(Array.length(tr),Time.toMicroseconds(Time.zeroTime))
      val jitList =  Array.array(Array.length(tr),Time.toMicroseconds(Time.zeroTime))

      fun calculateTimes (i) = 
        let 
            val cTime = (Array.sub(tc,i) - Array.sub(ts,i))
            val jTime = (Array.sub(ts,i) - Array.sub(tr,i))
            val rTime = (cTime + jTime)
        in
          print(IntInf.toString(cTime)^",");
          (*
          print(IntInf.toString(jTime)^": jitter time \n");
          print(IntInf.toString(rTime)^" : response time\n"); 

          print(IntInf.toString(Array.sub(ts,i))^": start time \n");
          print(IntInf.toString(Array.sub(tc,i))^": Completeion time \n");

          print(IntInf.toString(Array.sub(tr,i))^": ideal release time \n");

          *)

          Array.update(responseList,i,rTime);
          Array.update(compList,i,cTime); 
          Array.update(jitList,i,jTime) 
        end 
        

    in
        print("@");
        for(0 to (Array.length(tr) -1)) (fn i => calculateTimes(i));
        print("@");
        (*for(0 to (Array.length(tr) -1)) (fn i =>
        * print(IntInf.toString(Array.sub(responseList,i))^"\n"))*)
        (*printBenchmarkResults (Array.foldr (op ::) [] responseList,0)*)
        

        print ("\n")

     (*   print("Detector Completed\n");
        print("=============================================================\n");
        print("Max response time = "^IntInf.toString(IntInf.div((max_arr responseList),1000))^" ms \n");
        print("Min response time = "^IntInf.toString(IntInf.div(min_arr responseList,1000))^" ms\n");
        print("Avg response time = "^IntInf.toString(IntInf.div(avg_arr responseList,1000))^" ms\n");
        print("Std Dev response time = "^Real.toString((std_dev responseList)/1000.00)^" ms\n");
        print("=============================================================\n");
        print("Max Computation time = "^IntInf.toString(IntInf.div(max_arr compList,1000))^" ms\n");
        print("Min Computation time = "^IntInf.toString(IntInf.div(min_arr compList,1000))^" ms\n");
        print("Avg computation time = "^IntInf.toString(IntInf.div(avg_arr compList,1000))^" ms\n");
        print("Std Dev Computation time = "^Real.toString((std_dev compList)/1000.00)^" ms\n");
        print("=============================================================\n");
        print("Max Jitter time = "^IntInf.toString(IntInf.div(max_arr jitList,1000))^" ms\n");
        print("Min Jitter time = "^IntInf.toString(IntInf.div(min_arr jitList,1000))^" ms\n");
        print("Avg Jitter time = "^IntInf.toString(IntInf.div(avg_arr jitList,1000))^" ms\n");
        print("Std Dev Jitter time = "^Real.toString((std_dev
        jitList)/1000.00)^" ms\n")*)
        
    end;








    fun main() = 
    let 
      val frameBuffer = Frames.readFromFile("./input/frames_col.txt")

      val maxFrames = 200
           
      val ts = Array.array(maxFrames,Time.toMicroseconds (Time.zeroTime)) 
      val tc = Array.array(maxFrames,Time.toMicroseconds(Time.zeroTime) )
      
      val tr = Array.array(maxFrames,Time.toMicroseconds(Time.zeroTime) )

    fun populateIdealTime(t0) = 
        for(0 to (Array.length(tr)-1) ) (fn i => Array.update(tr,i, (t0 + (Time.toMicroseconds(period)* Int.toLarge(i))) ) ) 

          
      fun maybeSleep start =
        let 
          val elapsed = Time.toMicroseconds(Time.now()) - start
          val sleeptime = Time.toMicroseconds(period) - elapsed
        in
          if sleeptime > 0 then sleepForPeriod (Time.fromMicroseconds sleeptime)
          else ()
        end
     
      val responseTimeList : Time.time list ref  = ref [];
     




        

      fun loop ([],i,frameBuf) = if not (i=maxFrames) then loop(frameBuf,i,frameBuf) else ()
        | loop(x::xs,i,frameBuf) = if not(i = maxFrames) then (Array.update(ts,i,Time.toMicroseconds (Time.now()));
                                                        (*print(IntInf.toString(Array.sub(ts,i))^"\n"); *)
                                                        TransientDetector.TRANSIENTDETECTOR_run(x) ;
                                                        


                                                        Array.update(tc,i,Time.toMicroseconds(Time.now ()) );
                                                        (*print(IntInf.toString(Time.toMicroseconds(Time.now()))^"\n");*)
                                                        maybeSleep (Array.sub(ts,i));
                                                        loop(xs,(i+1),frameBuf) ) 
                          else
                            ()
    in
      (*print (Int.toString(List.length(maxFrames)))*)
      loop(frameBuffer, 0,frameBuffer);

      (*printArray(ts);*)
        
     populateIdealTime(Array.sub(ts,0));


      (benchmarkCD ts tc tr)

    end;




end


val rec delay =
   fn 0 => ()
    | n => delay (n - 1)
    
val rec loop =
   fn 0 => ()
    | n => ((*print ("loop" ^ Int.toString(n) ^"\n");*) delay 50000000; loop (n - 1))


(*val _ = (PThread.spawn(fn () => Driver.main());PThread.spawn(fn () => loop
* 10); PThread.run())*)

val _ = Driver.main();

(*Run with rtobj 5 for RTMLton*)

val _ = print ("done\n")
