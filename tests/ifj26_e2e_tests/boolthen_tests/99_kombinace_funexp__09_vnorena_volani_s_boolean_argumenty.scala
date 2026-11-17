import ifj26.*

def isEven(n: Int): Boolean = n / 2 * 2 == n

def both(a: Boolean, b: Boolean): Boolean = a && b

@main
def main(): Unit =
  print(toStr(both(isEven(2), isEven(3) || true)))
