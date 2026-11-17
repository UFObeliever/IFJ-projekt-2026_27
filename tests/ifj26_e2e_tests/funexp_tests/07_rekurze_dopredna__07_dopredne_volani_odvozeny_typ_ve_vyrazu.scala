import ifj26.*

@main
def main(): Unit =
  print(toStr(twice(4) + 1))

def twice(x: Int) = x * 2
