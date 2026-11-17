import ifj26.*

def dbl(x: Int): Int = x * 2

def add(a: Int, b: Int): Int = a + b

@main
def main(): Unit =
  print(toStr(add(1, 2)) + "-" + toStr(dbl(3)) + "-" + toStr(add(dbl(1), 1)))
