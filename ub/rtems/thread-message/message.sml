
open MLton.PrimThread
open MLton.Thread
open Posix.Signal

fun gettime () = get_ticks_since_boot ()

fun printit s = print (Int.toString(getMyPriority ())^"] "^s^"\n")

val _ = printit "Hello World\n";

val buffer = ref 0

fun sleep () = 
let
    val _ = Posix.Process.sleep (Time.fromSeconds 1)
in
    ()
end

fun sender () = 
let
in
    while true do (
        sleep ()
        ; printit ("sender: "^Int.toString(!buffer)^"\n")
        ; rtlock 0 ; instrument 0
        ; buffer := !buffer + 1
        ; rtunlock 0 ; instrument 1
    )
end

fun receiver () = 
let
in
    while true do (
        sleep ()
        ; rtlock 0 ; instrument 2
        ; printit ("receiver: "^Int.toString(!buffer)^"\n")
        ; rtunlock 0 ; instrument 3
    )
end

fun instrument_dumper () = 
let
in
    while true do (
        sleep ()
        ; rtlock 0 
        ; dump_instrument_stderr 3
        ; dump_instrument_stderr 2
        ; rtunlock 0
    )
end

val _ = pspawn (fn () => receiver (), 3)
val _ = pspawn (fn () => sender (), 2)
val _ = pspawn (fn () => instrument_dumper (), 4);


val _ = printit "Main end\n"
