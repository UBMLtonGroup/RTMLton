fun doStuff () = 
let open Array
  val arr= array(1000,1)

  fun loop i = 
    if i < length(arr) then
      (update(arr,i,2);loop (i+1))
    else
      ()
in
  loop 0
end

fun timeIt f x = let
  val t0 = Time.now ()
  val result = f x
  val t1 = Time.now ()
in
  print(concat["It took ",Time.toString(Time.-(t1,t0))," seconds\n"]);
  result
end


val _ = timeIt doStuff ()
val _ = print ("Done\n")
