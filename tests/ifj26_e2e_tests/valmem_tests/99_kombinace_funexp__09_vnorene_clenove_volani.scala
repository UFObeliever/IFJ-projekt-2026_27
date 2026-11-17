import ifj26.*

def f(x: Int): Int = x + 1

@main
def main(): Unit =
  print(f(f("ab".length).toString.length))
