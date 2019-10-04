
structure Frames =
struct 


 type t =  (int * string array * Pos.t array) option;
  (*val lengths : int array = Array.array(RAWFRAME_MAX_PLANES,0);
  val callsigns = Array.array(RAWFRAME_MAX_SIGNS,"a");
  val positions : Pos.t array =
    Array.array(RAWFRAME_MAX_PLANES,(0.0,0.0,0.0));
  val planecnt : int ref  = ref 0; *)

  val empty :t  = NONE


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




  exception EmptyFrame;

  fun getCallsigns(frame:t) =
  let
    val (n,cs,pos) = Option.valOf(frame)
  in
    if Option.isSome(frame) then cs else raise EmptyFrame
  end

  fun getPosition(frame:t)=
  let 
    val (n,cs,pos) = Option.valOf(frame)
  in
    if Option.isSome(frame) then pos else raise EmptyFrame 
  end

  fun getPlaneCnt(frame:t) = 
  let
    val (n,cs,pos) = Option.valOf(frame)
  in 
    if Option.isSome(frame) then n else raise EmptyFrame 
  end

  fun createFrame(n :int,cs:string array,pos :Pos.t array) :t =
    SOME (n,cs,pos)
 
  open TextIO

 fun arrayToList arr = Array.foldr (op ::) [] arr
 fun printList (xs, sep, f) = print (String.concatWith sep (map f xs))
 
  fun pFText (f :t) = 
     let
         val nplanes = getPlaneCnt(f)
         val posArr  = getPosition(f)
         val csArr = getCallsigns(f)
          
     in

         print("Frames.createFrame( "^Int.toString(nplanes)^", Array.fromList( [ ");
         for (0 to (nplanes-2)) (fn i => print("\""^Array.sub(csArr,i)^ "\"," ));
         print("\""^Array.sub(csArr,nplanes-1)^"\" ] ), Array.fromList( [");
         for (0 to (nplanes-2)) (fn i => print(Pos.getString(Array.sub(posArr,i)) ^",") );
         print(Pos.getString(Array.sub(posArr,nplanes-1))^"] )) \n :: \n")


     end

 fun printFrame (f :t) = 
     let
         val nplanes = getPlaneCnt(f)
         val posArr  = getPosition(f)
         val csArr = getCallsigns(f)
          
     in

         print("\n");
         for (0 to (nplanes-1)) (fn i => print(Array.sub(csArr,i)^" "^Pos.getString(Array.sub(posArr,i)) ^ "\n") )

     end

 

  exception InvalidNumber;


  val frameBuffer :t list ref= ref []
  
  fun readFrame (stream) = 
        let
            fun revLsToArr x = Array.fromList(List.rev x)  

            fun convertTuple(l) = 
              let
                val one :: xs = l
                val two::ys = xs
                val three :: zs = ys 
              in
                (*print(Real.toString(one)^"\n");
                print(Real.toString(two)^"\n");
                print(Real.toString(three)^"\n");*)
                if not (List.null(zs)) then Pos.errorcode else (one,two,three)
              end 


            fun processLine line =
                let val (name::(rest)) = String.tokens (fn c => c = #" ") line
                in
                (name, convertTuple (map (fn n => case Real.fromString n of
                                 SOME number => number
                               | NONE => raise InvalidNumber)
                   rest))
                end

            (*reads one frame and returns rest of stream*)    
            fun readlines (firstLs, secondLs,strm) =
                case StreamIO.inputLine(strm) of
                    SOME (line,str) => if String.isPrefix("Frame")(line) then 
                          readlines (firstLs, secondLs,str)
                        else if String.isPrefix("End")(line) then 
                          (firstLs,secondLs,str)
                        else if String.size(line) > 1 then 
                          let 
                            val (name, pos) = processLine(line) 
                          in
                            readlines ((name::firstLs), (pos::secondLs),str)
                          end
                        else
                          (firstLs, secondLs,str)

                    | NONE      => (firstLs, secondLs,strm)
            val (nameLs, posLs,st) = readlines([], [],stream)



        in
           (revLsToArr nameLs, revLsToArr posLs,st)
        end

  
  (*Reads frames from file and populates framebuffer*)
  fun readFromFile (f) =
  let
    val filestream = TextIO.openIn(f)

    

        fun createFrameBuffer(fstream)=
            let
                val (cs,pos,remainderStr) = readFrame(fstream)

                (*getting num of planes from num of callsigns instead of frames file*)
                val nplanes = Array.length(cs)

                val frame = createFrame(nplanes,cs,pos) 
            in
                frameBuffer := !frameBuffer @ [frame];
                if not(StreamIO.endOfStream(remainderStr)) then createFrameBuffer(remainderStr) else !frameBuffer 
            end  


    
  in
     createFrameBuffer(getInstream filestream)
  end
  

end
