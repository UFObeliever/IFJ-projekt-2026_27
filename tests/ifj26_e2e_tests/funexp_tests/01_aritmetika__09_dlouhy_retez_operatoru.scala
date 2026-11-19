import ifj26.*

def dbl(x: Int): Int = x * 2

@main
def main(): Unit =
  print(toStr(dbl(1) + dbl(2) * dbl(3) - dbl(4) / dbl(2)))
