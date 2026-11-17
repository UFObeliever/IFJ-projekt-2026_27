import ifj26.*

def dbl(x: Int): Int = x * 2

@main
def main(): Unit =
  val n = strLength("hello") * dbl(2)
  print(toStr(n))
