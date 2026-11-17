import ifj26.*

def clamp(x: Int): Int =
  if x < 0 then
    return 0
  if x > 10 then
    return 10
  x

@main def main(): Unit =
  val a = clamp(0 - 3)
  val b = clamp(4)
  val c = clamp(30)
  print(a)
  print("\n")
  print(b)
  print("\n")
  print(c)
  print("\n")
