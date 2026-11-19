import ifj26.*

def isOdd(n: Int): Boolean = n / 2 * 2 != n

@main
def main(): Unit =
  print("odd=" + toStr(isOdd(7)) + "," + toStr(isOdd(8)))
