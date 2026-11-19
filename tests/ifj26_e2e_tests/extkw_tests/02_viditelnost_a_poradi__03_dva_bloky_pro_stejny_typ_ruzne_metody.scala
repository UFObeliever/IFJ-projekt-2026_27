import ifj26.*

extension (v: Int)
  def twice: Int = v * 2

extension (v: Int)
  def thrice: Int = v * 3

@main
def main(): Unit =
  val x = 2
  val a = x.twice
  val b = x.thrice
  print(a)
  print(b)
