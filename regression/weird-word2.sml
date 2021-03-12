fun fib (w: Word5.word) : Word5.word =
   if w <= 0wx1
      then 0wx1
   else fib (w - 0wx1) + fib (w - 0wx2)

val s =
   case (fib 0wx5) of
      0wx0 => "0wx0"
    | 0wx1 => "0wx1"
    | 0wx2 => "0wx2"
    | 0wx3 => "0wx3"
    | 0wx4 => "0wx4"
    | 0wx5 => "0wx5"
    | 0wx6 => "0wx6"
    | 0wx7 => "0wx7"
    | 0wx8 => "0wx8"
    | 0wx9 => "0wx9"
    | 0wxA => "0wxA"
    | 0wxB => "0wxB"
    | 0wxC => "0wxC"
    | 0wxD => "0wxD"
    | 0wxE => "0wxE"
    | 0wxF => "0wxF"
    | 0wx10 => "0wx10"
    | 0wx11 => "0wx11"
    | 0wx12 => "0wx12"
    | 0wx13 => "0wx13"
    | 0wx14 => "0wx14"
    | 0wx15 => "0wx15"
    | 0wx16 => "0wx16"
    | 0wx17 => "0wx17"
    | 0wx18 => "0wx18"
    | 0wx19 => "0wx19"
    | 0wx1A => "0wx1A"
    | 0wx1B => "0wx1B"
    | 0wx1C => "0wx1C"
    | 0wx1D => "0wx1D"
    | 0wx1E => "0wx1E"
    | 0wx1F => "0wx1F"

val _ = print (concat [s, "\n"])
