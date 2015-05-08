open MLton.Thread

val prio = 1

val _ =
   if 13 = 1 + switch(fn t => prepare (t, 12, prio))
      then print "1 succeeded\n"
   else ()

val _ =
   if 13 = 1 + switch(fn t =>
                      prepare (new(fn () => switch(fn _ => prepare (t, 12, prio))), 
                      (), prio))
      then print "2 succeeded\n"
   else ()
      
val _ =
   if 13 = switch(fn t => prepare (prepend(t, fn n => n + 1, prio), 12, prio))
      then print "3 succeeded\n"
   else ()

val _ =
   if 13 = switch(fn t =>
                  prepare (new(fn () =>
                               let val t = prepend(t, fn n => n + 1, prio)
                               in switch(fn _ => prepare (t, 12, prio))
                               end),
                           (), prio))
      then print "4 succeeded\n"
   else ()
