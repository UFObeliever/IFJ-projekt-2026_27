import ifj26.*

def choose(b: Boolean, x: Int, y: Int): Int =
  if b then
    return x
  else
    return y

@main def main(): Unit =
  val a = choose(3 > 2, 10, 20)
  val b = choose(3 < 2, 10, 20)
  print(a)
  print("\n")
  print(b)
  print("\n")
