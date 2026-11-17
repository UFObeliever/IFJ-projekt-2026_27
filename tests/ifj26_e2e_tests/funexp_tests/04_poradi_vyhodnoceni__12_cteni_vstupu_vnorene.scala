import ifj26.*

def sub(a: Int, b: Int): Int = a - b

def dbl(x: Int): Int = x * 2

@main
def main(): Unit =
  print(toStr(sub(dbl(readInt()), readInt())))
