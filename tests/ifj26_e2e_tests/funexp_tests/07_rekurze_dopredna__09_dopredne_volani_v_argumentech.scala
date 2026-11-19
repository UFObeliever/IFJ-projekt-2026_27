import ifj26.*

@main
def main(): Unit =
  print(toStr(plus(inc(1), inc(2) * 2)))

def plus(a: Int, b: Int): Int = a + b

def inc(x: Int): Int = x + 1
