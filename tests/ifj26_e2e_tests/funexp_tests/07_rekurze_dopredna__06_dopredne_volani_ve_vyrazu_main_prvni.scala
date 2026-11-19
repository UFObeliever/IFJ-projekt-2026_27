import ifj26.*

@main
def main(): Unit =
  print(toStr(later(3) * 2 + later(1)))

def later(x: Int): Int = x + 100
