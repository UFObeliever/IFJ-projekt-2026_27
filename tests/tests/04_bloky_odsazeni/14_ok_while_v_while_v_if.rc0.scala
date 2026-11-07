import ifj26.*

@main def main(): Unit =
  var a = 0
  var b = 0
  if a < 1 then
    while a < 3 do
      while b < 3 do
        b = b + 1
      a = a + 1
  else
    a = 5
