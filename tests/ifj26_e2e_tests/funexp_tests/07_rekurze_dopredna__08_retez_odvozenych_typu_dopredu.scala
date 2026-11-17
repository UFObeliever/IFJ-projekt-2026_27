import ifj26.*

def fa(x: Int) = fb(x) + 1

@main
def main(): Unit =
  print(toStr(fa(3)))

def fb(x: Int) = x * 2
