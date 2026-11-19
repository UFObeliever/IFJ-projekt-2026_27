import ifj26.*

def add(a: Int)(b: Int): Int = a + b

@main def main(): Unit =
  val x = add(add(1)(2))(add(3)(4))
  print(x)
  print("\n")
