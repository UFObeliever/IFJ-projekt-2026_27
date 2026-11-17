import ifj26.*

def plus(a: Int, b: Int): Int = a + b

def seven: Int = plus(3, 4)

@main
def main(): Unit =
  print(seven)
