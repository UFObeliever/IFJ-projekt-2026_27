import ifj26.*

def answer: Int = 42

def dbl(x: Int): Int = x * 2

@main
def main(): Unit =
  print(toStr(dbl(answer) + dbl(answer)))
