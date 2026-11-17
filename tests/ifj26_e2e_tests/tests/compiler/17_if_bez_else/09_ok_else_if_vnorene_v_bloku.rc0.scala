import ifj26.*

@main def main(): Unit =
  val x = 5
  if x > 0 then
    if x < 3 then
      print("a")
    else if x < 7 then
      print("b")
  else
    print("c")
