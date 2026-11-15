import ifj26.*

@main def main(): Unit =
  val x = twice(4)
  val y = once(x)
  print(x)
  print("\n")
  print(y)
  print("\n")

def twice(a: Int): Int =
  val b = once(a)
  return b + b

def once(a: Int): Int = a + 1
