import ifj26.*

def sgn(x: Int)(y: Int): Int =
  if x < y then
    return -1
  else
    return 1

@main
def main(): Unit =
  val a = sgn(1)(2)
  val b = sgn(5)(2)
  print(a)
  print(" ")
  print(b)
