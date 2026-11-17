import ifj26.*

def dbl(x: Int): Int = x * 2

def h(x: Int): Int = dbl(x) + dbl(x + 1)

@main
def main(): Unit =
  print(toStr(h(2)))
