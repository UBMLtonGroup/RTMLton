
structure Reducer =
struct 


    
    val voxel_size = 10.0;
    val proximity_radius = Constants.proximity_radius;


    val horizontal = (voxel_size,0.0);
    val vertical = (0.0,voxel_size);

    (*create a 2d point representing a voxel*)
    fun voxelHash(position :Pos.t) = 
    let
      val (x,y,_) = position

      val voxelx = voxel_size * (x/voxel_size)
      val voxely = voxel_size * (y/voxel_size)

    in
      (if x < 0.0 then voxelx-voxel_size else voxelx, if y < 0.0 then
        voxely-voxel_size else voxely)
    end


  (*given voxel and motion, determine if motion is in voxel*)
  fun isInVoxel(voxel,motion :Motion.t) =
  let
    open Pos
    val(voxelx,voxely) = voxel
    val init = Motion.getfirstPosition(motion)
    val fin = Motion.getsecondPosition(motion)
    val v_s = voxel_size
    val r = proximity_radius /2.0
    
    val v_x =  voxelx
    val x0 = getx(init)
    val xv = getx(fin) - getx(init)

    val v_y = voxely
    val y0 =  gety(init)
    val yv = gety(fin)- gety(init)

    val low_x : real ref = ref ((v_x - r - x0)/xv)
    val high_x :real ref = ref ((v_y + v_s + r - y0)/yv)
     
    val low_y : real ref = ref((v_y-r-y0)/yv)
    val high_y : real ref = ref((v_y+v_s+r-y0)/yv)

    fun swap (a,b) = 
    let 
      val tmp : real ref = ref 0.0
    in
      tmp := !a;
      a := !b;
      b:= !tmp
    end;

    fun getResult () = 
      if (
        (
            ( Pos.nearlyEqual(xv,0.0,0.01) andalso v_x <= x0 + r andalso x0 - r <= v_x + v_s) (* no motion in x *) orelse 
                    ((!low_x <= 1.0 andalso 1.0 <= !high_x) orelse (!low_x <= 0.0 andalso 0.0 <= !high_x) orelse (0.0 <= !low_x andalso !high_x <= 1.0))
        )
        
        andalso 
       
        (
            ( (Pos.nearlyEqual(yv,0.0,0.01)) andalso (v_y <= y0 + r) andalso (y0 - r <= v_y + v_s ) ) (* no motion in y *) orelse 
                    ( (!low_y <= 1.0)  andalso (1.0 <= !high_y) )  orelse ( (!low_y
                    <= 0.0) andalso (0.0 <= !high_y) ) orelse ( (0.0 <= !low_y)
                    andalso (!high_y <= 1.0))
        )
        
        andalso

        ( Pos.nearlyEqual(xv,0.0,0.01) orelse (Pos.nearlyEqual(yv,0.0,0.01)) orelse (* no motion in x or y or both *)
                    (!low_y <= !high_x andalso !high_x <= !high_y) orelse
                    (!low_y <= !low_x andalso !low_x <= !high_y) orelse (!low_x <= !low_y andalso !high_y <= !high_x))
        ) then true else false

  in
    ((*Voxel.printVoxel(voxel);
    Motion.printMotion(motion);  *)
    
       if xv<0.0 then swap(low_x,high_x) else ();
    if yv< 0.0 then swap(low_y,high_y) else ();

 (*print(Bool.toString((
            ( Pos.nearlyEqual(xv,0.0,0.01) andalso v_x <= x0 + r andalso x0 - r <= v_x + v_s) (* no motion in x *) orelse 
                    ((!low_x <= 1.0 andalso 1.0 <= !high_x) orelse (!low_x <= 0.0 andalso 0.0 <= !high_x) orelse (0.0 <= !low_x andalso !high_x <= 1.0))
        )
) );
    
    print(Bool.toString((
            ( (Pos.nearlyEqual(yv,0.0,0.01)) andalso (v_y <= y0 + r) andalso (y0 - r <= v_y + v_s ) ) (* no motion in y *) orelse 
                    ( (!low_y <= 1.0)  andalso (1.0 <= !high_y) )  orelse ( (!low_y
                    <= 0.0) andalso (0.0 <= !high_y) ) orelse ( (0.0 <= !low_y)
                    andalso (!high_y <= 1.0))
        )));
    
    print(Bool.toString(( Pos.nearlyEqual(xv,0.0,0.01) orelse (Pos.nearlyEqual(yv,0.0,0.01)) orelse (* no motion in x or y or both *)
                    (!low_y <= !high_x andalso !high_x <= !high_y) orelse
                    (!low_y <= !low_x andalso !low_x <= !high_y) orelse (!low_x
                    <= !low_y andalso !high_y <= !high_x))));     *)   

    getResult ()
        
    )
  end; 


  fun containsKey(key,[]) = false
    |containsKey(key, h:: tl) = if Voxel.eq(key,h) then true else containsKey(key,tl)
    


  (*Returns a list of motions which have possible collisions. Unlike in CDj
  * which returns a Hashmap and then extract values from it in reduce collision.*)
  fun dfsVoxelHashRecurse(motion :Motion.t,next_voxel,voxel_map,graph_colors) = 
  let
    val tmp :(real*real) ref  = ref (0.0,0.0)

    val (nvx,nvy) = next_voxel
    val (hx,hy)= horizontal
    val (vx,vy) = vertical
    
    (*Left Boundary*)
    val lb = (nvx-hx,nvy-hy)

    (*Right Boundary*)
    val rb = (nvx+hx,nvy+hy)

    (*Upper Boundary*)
    val ub = (nvx+vx,nvy+vy)

    (*Lower Boundary*)
    val lob = (nvx-vx,nvy-vy)

    (*Upper-Left*)
    val (ulx,uly)= (nvx-hx,nvy-hy)
    val ul = (ulx+vx,uly+vy)

    (*Upper-Right*)
    val (urx,ury) = (nvx+hx,nvy+hy)
    val ur = (urx+vx,ury+vy)

    (*Lower-Left*)
    val (llx,lly) = (nvx-hx,nvy-hy)
    val ll = (llx-vx,lly-vy)

    (*Lower-Right*)
    val (lrx,lry) = (nvx+hx,nvy+hy)
    val lr = (lrx-vx,lry-vy)


  in
    if isInVoxel(next_voxel,motion) andalso not (containsKey(next_voxel,!graph_colors))  then 
      (
        (*print("IN ADDING VOXEL\n");*)
        graph_colors :=  next_voxel :: !graph_colors;
        voxel_map := VoxelMap.put(next_voxel,motion,!voxel_map);

        dfsVoxelHashRecurse(motion,lb,voxel_map,graph_colors);
        dfsVoxelHashRecurse(motion,rb,voxel_map,graph_colors);
        dfsVoxelHashRecurse(motion,ub,voxel_map,graph_colors);
        dfsVoxelHashRecurse(motion,lob,voxel_map,graph_colors);
        dfsVoxelHashRecurse(motion,ul,voxel_map,graph_colors);
        dfsVoxelHashRecurse(motion,ur,voxel_map,graph_colors);
        dfsVoxelHashRecurse(motion,ll,voxel_map,graph_colors);
        dfsVoxelHashRecurse(motion,lr,voxel_map,graph_colors)

       ) else (voxel_map,graph_colors)
  end;



    fun performVoxelHashing(motion,voxel_map,graph_colors) = 
    let
      val voxel = voxelHash(Motion.getfirstPosition(motion))
    in
      dfsVoxelHashRecurse(motion,voxel,voxel_map,graph_colors)
    end;


    


end
