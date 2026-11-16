import ifj26.*

@main def main(): Unit =
  val a = 1
  var b = 0
  if a < 2 then
    while b < 3 do
      b = b + a
  else
    b = a
