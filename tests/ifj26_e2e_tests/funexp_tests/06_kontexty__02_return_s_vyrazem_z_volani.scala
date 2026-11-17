import ifj26.*

def dbl(x: Int): Int = x * 2

def f(x: Int): Int =
  return dbl(x) + dbl(x + 1)

@main
def main(): Unit =
  print(toStr(f(2)))
