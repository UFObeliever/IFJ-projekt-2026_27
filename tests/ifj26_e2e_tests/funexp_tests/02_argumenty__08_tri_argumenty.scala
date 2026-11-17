import ifj26.*

def add3(a: Int, b: Int, c: Int): Int = a + b + c

def dbl(x: Int): Int = x * 2

@main
def main(): Unit =
  print(toStr(add3(1 + 1, dbl(2), 10 - 4)))
