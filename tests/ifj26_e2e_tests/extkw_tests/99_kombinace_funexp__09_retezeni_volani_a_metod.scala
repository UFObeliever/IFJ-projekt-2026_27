import ifj26.*

extension (value: Int)
  def twice: Int = value * 2
  def foo(): Int = value + 1
  def plus(other: Int): Int = value + other

def dbl(x: Int): Int = x * 2

@main
def main(): Unit =
  print(toStr(dbl(2).plus(dbl(1)).twice))
