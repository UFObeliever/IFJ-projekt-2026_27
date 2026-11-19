import ifj26.*

def half(x: Int): Int = x / 2

@main
def main(): Unit =
  print(toStr(half(9) + 100 / half(10)))
