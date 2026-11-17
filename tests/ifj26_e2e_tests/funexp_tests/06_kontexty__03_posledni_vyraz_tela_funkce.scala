import ifj26.*

def dbl(x: Int): Int = x * 2

def g(x: Int): Int =
  val t = dbl(x)
  t + dbl(t)

@main
def main(): Unit =
  print(toStr(g(1)))
