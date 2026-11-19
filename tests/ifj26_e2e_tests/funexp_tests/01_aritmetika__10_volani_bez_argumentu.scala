import ifj26.*

def one(): Int = 1

def zero(): Int = 0

@main
def main(): Unit =
  print(toStr(zero() + one() * 2 - one()))
