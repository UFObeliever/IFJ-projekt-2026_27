import ifj26.*

def f(a: Int, b: Int)(c: Int, d: Int): Int = a * 1000 + b * 100 + c * 10 + d

@main def main(): Unit =
  val x = f(1, 2)(3, 4)
  print(x)
  print("\n")
