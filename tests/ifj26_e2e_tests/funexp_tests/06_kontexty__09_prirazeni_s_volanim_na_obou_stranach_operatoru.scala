import ifj26.*

def add(a: Int, b: Int): Int = a + b

def dbl(x: Int): Int = x * 2

@main
def main(): Unit =
  var v = 1
  v = add(v, dbl(v)) * add(1, 1)
  print(toStr(v))
