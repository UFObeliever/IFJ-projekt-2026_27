import ifj26.*

extension (value: Int)
  def twice: Int = value * 2
  def foo(): Int = value + 1
  def plus(other: Int): Int = value + other

@main
def main(): Unit =
  val x = 6
  val w = x.plus(12)
  print(w)
