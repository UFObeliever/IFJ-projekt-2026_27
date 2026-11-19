import ifj26.*

def dbl(x: Int): Int = x * 2

@main
def main(): Unit =
  val d: Double = dbl(2) + 1
  print(toStr(toInt(d * 2.0)))
