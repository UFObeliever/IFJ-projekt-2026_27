import ifj26.*

def describe(a: Int, b: Double, c: String): String =
  val sa = toStr(a)
  val sb = toStr(b)
  return sa + ":" + sb + ":" + c

@main def main(): Unit =
  val r = describe(1, 2.5, "x")
  print(r)
  print("\n")
