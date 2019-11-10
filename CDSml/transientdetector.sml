






structure StateTable  = 
struct 

  type t = string * Pos.t ref;

  val table : t list ref = ref [];



  fun search (cs,[]) = ref Pos.errorcode
    | search (cs,(x,Posval) ::tl) = 
        if String.compare(x,cs) = EQUAL  then Posval
        else search (cs,tl)   
  
 (* puts a Position in the table. if new, creates a new entry to list, if
 * callsign already exists, updates the Position*)
  fun put (cs , position :Pos.t) = 
    let
      val p = search(cs,!table)
    in
      if Pos.eq(!p,Pos.errorcode) then ( table := (cs,ref position) :: !table; ())
      else p:= position
    end
  
(*gets the corresponding Position from table, given the callsign*)
  fun get (cs) = 
    let
      val x = search (cs,!table)
    in
      !x
    end;

end




structure TransientDetector =
struct
  
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

  
  (* returns list of list of motions*)
  (*graph_colors need not be cleared in perform voxelHashing as its called with a
  * ref [] each time in this function*)
  fun reduceCollisionSet(motions :Motion.t list) = 
  let 
    val voxel_map = ref (VoxelMap.makemap())
    val graph_colors = ref []

    fun pvh (m) = 
      let 
          val (a,b) = Reducer.performVoxelHashing(m,voxel_map,graph_colors)
      in
          ( (*print ("graph colors length
          "^Int.toString(List.length(!graph_colors))^"\n");*)
          graph_colors := []) (*graph_colors := b @ !graph_colors*)
      end

    fun traverseListofMotions ([]) = ()
      | traverseListofMotions (h :: t) =  (pvh (h); traverseListofMotions(t))



  in
     traverseListofMotions(motions);
  (*   print("Voxel map length " ^
  *   Int.toString(VoxelMap.getLength(!voxel_map))^"\n");*)
     VoxelMap.getvalues(voxel_map)
  end 


  (*returns list of triples Aircraft1 * aircraft2 * coordinates of collision 
  * Each list of motions corresponds to motions in a voxel that can possibly have
  * collisions. there can be more than one collision in a voxel hence it returns
  * list of collisions
  * *)
  fun determineCollisions(motions : Motion.t list) = 
  let
    
    val ret = ref (Collision.empty())


    fun det (one,[]) = !ret
      | det (one,two :: tl) =
      let
        val vec = Motion.findIntersection(one,two)
      in
        (*print("collision at "^Pos.getString(vec));*)
        if not(Pos.eq(vec,Pos.errorcode)) then
          (ret:= Collision.create(Motion.getAircraft(one),Motion.getAircraft(two),vec) :: !ret;
           det(one,tl) )
        else
          det(one,tl)
      end
      

    fun determine ([]) = !ret 
      | determine (one :: tl) = (det(one,tl) ; determine(tl))


  in
      (*print("determine motions length = "^
      * Int.toString(List.length(motions))^"\n");*)
      determine(motions)
  end 



  fun lookforCollisions(motions : Motion.t list) = 
  let
    val check = reduceCollisionSet(motions) (*List of list of motions*)

    val c = Collision.empty()

    fun map f [] = []
      | map f (x :: xs) = (f x @ map f xs )

   fun printcheck  [] = ()
      | printcheck (x :: xs) = print("Check list i length "^Int.toString(List.length(x))^"\n")

    val ls = map determineCollisions check 
  in 
    (*print("check length " ^ Int.toString(List.length(check))^"\n");*)
    (*printcheck check;
    print ("Collision length "^ Int.toString(List.length(ls)) ^"\n");*)
   c @ ls
  end





  (*Call signs are indexed per plane in the array. i.e. callsign at index i is the
  * callsign of plane i
  * If the callsign is not present in statetable, then the motion is same as
  * current Position. If callsign is present motion is vector from old Position to
  * new Position*)
  fun TCM (i,currentFrame) = 
          let 
            val p :Pos.t = Array.sub(Frames.getPosition(currentFrame), i)
            val c  = Array.sub(Frames.getCallsigns(currentFrame), i)

          fun createMotions (cs, position :Pos.t) = 
              let 
                  val old_Pos = StateTable.get(cs)
              in
                (StateTable.put(cs,position);
                if Pos.eq(old_Pos,Pos.errorcode) then 
                 (cs,position,position) else
                  (cs,old_Pos,position) )
              end


          in
           createMotions(c,p)      
          end;


  (*returns list of motions*)
  fun TRANSIENTDETECTOR_createMotions(currentFrame) = 
  let
     val mo : Motion.t list ref= ref []

   in
     for (0 to (Frames.getPlaneCnt(currentFrame)-1)) (fn i => mo :=  !mo @ [TCM(i,currentFrame)]);
     !mo
   end;




  fun TRANSIENTDETECTOR_run(currentFrame) = 
   let
     val motions : Motion.t list = TRANSIENTDETECTOR_createMotions(currentFrame);

      val collisions = lookforCollisions(motions)

     fun printResults ([],index) = ()
       | printResults (c :: tl,index) = (print("CD Collision" ^ Int.toString(index) ^ "occured at location " ^ Pos.getString(Collision.getLocation(c)) ^
                                          " with aircraft " ^ Collision.getAircraftOne(c) ^ " and aircraft " ^
                                          Collision.getAircraftTwo(c) ^ "\n" );
                                         printResults(tl,index+1))
   in
     (*(*Motion.printListOfMotions(motions);*)print ("CD detected " ^ Int.toString(List.length(collisions)) ^ " collisions \n");
     printResults(collisions,0)
     *)()
   end;

end








