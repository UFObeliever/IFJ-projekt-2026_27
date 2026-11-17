import ifj26.*

def f(n: Int)(d: Double)(s: String): String =
  val a = toStr(n)
  val b = toStr(d)
  s + a + " " + b

@main def main(): Unit =
  val r = f(7)(1.5)("x=")
  print(r)
  print("\n")
