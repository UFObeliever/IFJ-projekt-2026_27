import ifj26.*

@main def main(): Unit =
  var a = 1
  var b = 1
  var i = 0
  while i < 3 do
    if a == 1 then
      if b == 1 then
        print("x")
    else
      print("y")
    b = b + 1
    a = a + 1
    i = i + 1
