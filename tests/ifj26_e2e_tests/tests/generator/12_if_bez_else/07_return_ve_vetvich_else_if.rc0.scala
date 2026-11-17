import ifj26.*

def sign(x: Int): Int =
  if x < 0 then
    return 0 - 1
  else if x == 0 then
    return 0
  else
    return 1

@main def main(): Unit =
  val a = sign(0 - 7)
  val b = sign(0)
  val c = sign(7)
  print(a)
  print("\n")
  print(b)
  print("\n")
  print(c)
  print("\n")
