import ifj26.*

def twice(x: Int): Int = x * 2

def add(a: Int, b: Int): Int = a + b

@main
def main(): Unit =
  print(s"${add(twice(1), 3)}")
