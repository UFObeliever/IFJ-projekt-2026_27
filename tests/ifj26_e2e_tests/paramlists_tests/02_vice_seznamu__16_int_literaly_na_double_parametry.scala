import ifj26.*

def avg(a: Double)(b: Double): Double = (a + b) / 2.0

@main
def main(): Unit =
  val r = avg(1)(2)
  val s = toStr(r)
  print(s)
