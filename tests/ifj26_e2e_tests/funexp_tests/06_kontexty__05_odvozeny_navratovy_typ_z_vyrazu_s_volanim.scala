import ifj26.*

def dbl(x: Int): Int = x * 2

def m(x: Int) = dbl(x) * 2

@main
def main(): Unit =
  print(toStr(m(3)))
