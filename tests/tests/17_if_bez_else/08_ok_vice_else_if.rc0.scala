import ifj26.*

@main def main(): Unit =
  val x = 5
  if x < 0 then
    print("a")
  else if x == 0 then
    print("b")
  else if x < 10 then
    print("c")
  else if x < 100 then
    print("d")
  else
    print("e")
