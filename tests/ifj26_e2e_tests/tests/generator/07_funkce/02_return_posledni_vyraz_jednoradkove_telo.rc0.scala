import ifj26.*

def a(x: Int): Int =
  return x + 1

def b(x: Int): Int =
  val y = x * 2
  y

def c(x: Int): Int = x - 1

@main def main(): Unit =
  val r1 = a(10)
  val r2 = b(10)
  val r3 = c(10)
  print(r1)
  print("\n")
  print(r2)
  print("\n")
  print(r3)
  print("\n")
