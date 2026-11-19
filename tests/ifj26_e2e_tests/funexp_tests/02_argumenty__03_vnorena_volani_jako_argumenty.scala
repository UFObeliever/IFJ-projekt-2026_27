import ifj26.*

def add(a: Int, b: Int): Int = a + b

def dbl(x: Int): Int = x * 2

@main
def main(): Unit =
  print(toStr(add(dbl(2), dbl(3))))
