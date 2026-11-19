import ifj26.*

def add(a: Int)(b: Int): Int = a + b

@main def main(): Unit =
  val y = 3
  val x = add(y * 2)(y - 1)
  print(x)
