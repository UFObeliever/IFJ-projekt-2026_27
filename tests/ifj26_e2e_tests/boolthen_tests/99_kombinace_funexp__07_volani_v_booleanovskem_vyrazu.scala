import ifj26.*

def isEven(n: Int): Boolean = n / 2 * 2 == n

@main
def main(): Unit =
  print(toStr(isEven(4) && !isEven(3)))
