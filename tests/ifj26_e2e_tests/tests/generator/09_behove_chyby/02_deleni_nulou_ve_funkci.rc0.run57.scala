import ifj26.*

def divide(a: Int, b: Int): Int =
  val q = a / b
  return q

@main def main(): Unit =
  print("start\n")
  val x = divide(6, 3)
  print(x)
  print("\n")
  val y = divide(1, 0)
  print(y)
  print("\n")
