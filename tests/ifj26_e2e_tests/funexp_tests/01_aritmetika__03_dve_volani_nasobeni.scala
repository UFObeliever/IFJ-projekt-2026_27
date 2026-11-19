import ifj26.*

def dbl(x: Int): Int = x * 2

@main
def main(): Unit =
  print(toStr(dbl(2) * dbl(3)))
