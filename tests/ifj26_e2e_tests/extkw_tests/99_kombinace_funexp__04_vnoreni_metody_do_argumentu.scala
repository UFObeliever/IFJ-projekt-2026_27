import ifj26.*

extension (value: Int)
  def twice: Int = value * 2
  def foo(): Int = value + 1
  def plus(other: Int): Int = value + other

@main
def main(): Unit =
  val x = 6
  print(toStr(x.plus(x.twice)))
