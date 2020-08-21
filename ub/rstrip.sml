open List

val s = "This is a\n string"

fun isnl x =
let
    val _ = print (Char.toString(x))
in
    if x = chr(10) then false else true
end


fun stripnl s' = List.filter
    (fn x => if x = chr(10) then false else true) s'

fun stripnlfromstr s = implode (stripnl (explode s))


val _ = print (implode (stripnl (explode s)))

val _ = print (stripnlfromstr s)

