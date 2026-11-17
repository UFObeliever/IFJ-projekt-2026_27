import ifj26.*

def add(a: Int, b: Int): Int = a + b

@main
def main(): Unit =
  print(toStr(add(1 + 2, 3 * 4)))
