import ifj26.*

def f(x: Int): Int = x * 2

@main
def main(): Unit =
  print(toStr(f("abc".length)))
