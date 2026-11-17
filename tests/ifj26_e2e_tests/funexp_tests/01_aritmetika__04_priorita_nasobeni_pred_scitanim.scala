import ifj26.*

def dbl(x: Int): Int = x * 2

@main
def main(): Unit =
  print(toStr(2 * dbl(3) + 1) + " " + toStr(1 + 2 * dbl(3)))
