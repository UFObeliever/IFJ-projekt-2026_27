import ifj26.*

extension (self: Int)
  def inc: Int = self + 1

@main
def main(): Unit =
  val x = 4
  val r = x.inc
  print(r)
