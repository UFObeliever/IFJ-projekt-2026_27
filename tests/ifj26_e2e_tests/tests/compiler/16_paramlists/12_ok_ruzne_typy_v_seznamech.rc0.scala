import ifj26.*

def f(a: Int)(b: Double)(c: String): String = c

@main def main(): Unit =
  val s = f(1)(2.5)("x")
  print(s)
