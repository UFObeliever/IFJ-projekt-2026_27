import ifj26.*

def f(a: Int, b: Int)(c: String): String = c

@main def main(): Unit =
  val s = f(1, 2)("abc")
  print(s)
