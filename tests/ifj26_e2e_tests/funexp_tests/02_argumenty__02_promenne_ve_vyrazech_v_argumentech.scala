import ifj26.*

def add(a: Int, b: Int): Int = a + b

@main
def main(): Unit =
  val a = 5
  val b = 3
  print(toStr(add(a * 2, b - 1)))
