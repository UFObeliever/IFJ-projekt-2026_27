import ifj26.*

extension (n: Int)
  def isPositive: Boolean = n > 0

@main
def main(): Unit =
  val a = 5
  val r = a.isPositive
  print(r)
